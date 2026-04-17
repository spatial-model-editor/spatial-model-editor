#include "metalpixelsim.hpp"
#include "pixelsim_common.hpp"
#include "pixelsim_impl.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/symbolic.hpp"
#include "sme/utils.hpp"
#include <QElapsedTimer>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fmt/core.h>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace sme::simulate {

namespace {

class MetalPixelSimError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

using detail::allMembraneFaceDirections;
using detail::calculateMaxStableTimestep;
using detail::getFaceFluxLength;
using detail::hasAnyCrossDiffusion;
using detail::invalidCompartmentIndex;

constexpr unsigned int metalKernelThreadgroupSize{128};

[[nodiscard]] static std::string toStdString(const NS::String *str) {
  if (str == nullptr || str->utf8String() == nullptr) {
    return {};
  }
  return str->utf8String();
}

[[nodiscard]] static std::string toStdString(const NS::Error *error) {
  if (error == nullptr) {
    return "unknown Metal error";
  }
  return toStdString(error->localizedDescription());
}

static void throwIfMetalFailed(bool ok, const std::string &context,
                               const NS::Error *error = nullptr) {
  if (!ok) {
    throw MetalPixelSimError(
        detail::makeMetalCompileFailureMessage(context, toStdString(error)));
  }
}

[[nodiscard]] static unsigned int
threadgroupSizeForPipeline(const MTL::ComputePipelineState *pipeline) {
  return std::min<unsigned int>(
      metalKernelThreadgroupSize,
      static_cast<unsigned int>(pipeline->maxTotalThreadsPerThreadgroup()));
}

[[nodiscard]] static MTL::Size makeThreadgroupCount(std::size_t nItems,
                                                    unsigned int groupSize) {
  return MTL::Size::Make(
      static_cast<NS::UInteger>((nItems + groupSize - 1) / groupSize), 1, 1);
}

[[nodiscard]] static MTL::Size makeThreadgroupSize(unsigned int groupSize) {
  return MTL::Size::Make(static_cast<NS::UInteger>(groupSize), 1, 1);
}

static void waitForCommandBuffer(MTL::CommandBuffer *commandBuffer,
                                 const std::string &context) {
  commandBuffer->commit();
  commandBuffer->waitUntilCompleted();
  if (const auto *error = commandBuffer->error(); error != nullptr) {
    const auto message =
        detail::makeMetalCompileFailureMessage(context, toStdString(error));
    throw MetalPixelSimError(message);
  }
}

template <typename T>
[[nodiscard]] static constexpr std::size_t bufferByteCount(std::size_t count) {
  return sizeof(T) * count;
}

template <typename T>
[[nodiscard]] static MTL::Buffer *
newBuffer(MTL::Device *device, std::size_t count, MTL::ResourceOptions options,
          const std::string &context) {
  auto *buffer = device->newBuffer(bufferByteCount<T>(count), options);
  if (buffer == nullptr) {
    throw MetalPixelSimError(context);
  }
  return buffer;
}

template <typename T>
[[nodiscard]] static MTL::Buffer *newSharedBuffer(MTL::Device *device,
                                                  std::size_t count,
                                                  const std::string &context) {
  return newBuffer<T>(device, count, MTL::ResourceStorageModeShared, context);
}

template <typename T>
[[nodiscard]] static MTL::Buffer *newPrivateBuffer(MTL::Device *device,
                                                   std::size_t count,
                                                   const std::string &context) {
  return newBuffer<T>(device, count, MTL::ResourceStorageModePrivate, context);
}

template <typename EncodeFn>
static void runBlitCommand(MTL::CommandQueue *queue, const std::string &context,
                           EncodeFn &&encode) {
  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
  auto *commandBuffer = queue->commandBuffer();
  if (commandBuffer == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal command buffer");
  }
  auto *encoder = commandBuffer->blitCommandEncoder();
  if (encoder == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal blit encoder");
  }
  encode(encoder);
  encoder->endEncoding();
  waitForCommandBuffer(commandBuffer, context);
  pool->release();
}

static void copyBuffer(MTL::CommandQueue *queue, MTL::Buffer *src,
                       MTL::Buffer *dst, std::size_t nBytes,
                       const std::string &context) {
  if (nBytes == 0) {
    return;
  }
  runBlitCommand(queue, context, [&](MTL::BlitCommandEncoder *encoder) {
    encoder->copyFromBuffer(src, 0, dst, 0, static_cast<NS::UInteger>(nBytes));
  });
}

template <typename T> [[nodiscard]] static T *bufferData(MTL::Buffer *buffer) {
  return static_cast<T *>(buffer->contents());
}

static void uploadDoublesToBuffer(MTL::Buffer *buffer,
                                  const std::vector<double> &src,
                                  const std::string &context) {
  auto *dst = bufferData<float>(buffer);
  if (dst == nullptr) {
    throw MetalPixelSimError(context);
  }
  std::transform(src.begin(), src.end(), dst,
                 [](double value) { return static_cast<float>(value); });
}

static void downloadBufferToDoubles(std::vector<double> &dst,
                                    MTL::Buffer *buffer,
                                    const std::string &context) {
  const auto *src = bufferData<const float>(buffer);
  if (src == nullptr) {
    throw MetalPixelSimError(context);
  }
  if (dst.empty()) {
    return;
  }
  std::transform(src, src + dst.size(), dst.begin(),
                 [](float value) { return static_cast<double>(value); });
}

template <typename T>
static void uploadVectorToBuffer(MTL::Buffer *buffer, const std::vector<T> &src,
                                 const std::string &context) {
  auto *dst = bufferData<T>(buffer);
  if (dst == nullptr) {
    throw MetalPixelSimError(context);
  }
  std::copy(src.begin(), src.end(), dst);
}

struct MetalGeneratedExpressionBundle {
  std::vector<std::string> safeVariables;
  std::vector<std::string> metalExpressions;
};

static MetalGeneratedExpressionBundle
makeMetalGeneratedExpressionBundle(const std::vector<std::string> &variables,
                                   const std::vector<std::string> &expressions,
                                   const std::string &expressionContext) {
  MetalGeneratedExpressionBundle bundle;
  bundle.safeVariables.reserve(variables.size());
  bundle.metalExpressions.reserve(expressions.size());
  for (std::size_t i = 0; i < variables.size(); ++i) {
    bundle.safeVariables.push_back(fmt::format("sme_var_{}", i));
  }

  common::Symbolic symbolic{expressions, variables};
  if (!symbolic.isValid()) {
    throw MetalPixelSimError(
        fmt::format("Failed to convert {} to Metal code: {}", expressionContext,
                    symbolic.getErrorMessage()));
  }
  symbolic.relabel(bundle.safeVariables);
  for (std::size_t i = 0; i < expressions.size(); ++i) {
    try {
      bundle.metalExpressions.push_back(symbolic.metalCode(i));
    } catch (const std::exception &e) {
      throw MetalPixelSimError(
          fmt::format("Failed to convert {} '{}' to Metal code: {}",
                      expressionContext, expressions[i], e.what()));
    }
  }

  return bundle;
}

} // namespace

std::string
detail::makeMetalKernelSource(const std::vector<std::string> &variables,
                              const std::vector<std::string> &expressions) {
  const auto bundle = makeMetalGeneratedExpressionBundle(variables, expressions,
                                                         "reaction expression");

  std::ostringstream src;
  src << "#include <metal_stdlib>\n";
  src << "using namespace metal;\n\n";
  src << "#define N_SPECIES " << bundle.safeVariables.size() << "\n";
  src << "#define RK_ERROR_BLOCK_SIZE " << metalKernelThreadgroupSize << "\n\n";
  src << "inline void reaction_eval(device float* sme_out, "
         "const device float* sme_values) {\n";
  for (std::size_t i = 0; i < bundle.safeVariables.size(); ++i) {
    src << "  const float " << bundle.safeVariables[i] << " = sme_values[" << i
        << "];\n";
  }
  for (std::size_t i = 0; i < bundle.metalExpressions.size(); ++i) {
    src << "  sme_out[" << i << "] = " << bundle.metalExpressions[i] << ";\n";
  }
  src << "}\n\n";
  src << R"(
kernel void reaction_kernel(const device float* conc [[buffer(0)]],
                            device float* dcdt [[buffer(1)]],
                            constant uint& nPixels [[buffer(2)]],
                            uint gid [[thread_position_in_grid]]) {
  if (gid >= nPixels) {
    return;
  }
  reaction_eval(dcdt + gid * N_SPECIES, conc + gid * N_SPECIES);
}

kernel void diffusion_uniform_kernel(const device float* conc [[buffer(0)]],
                                     device float* dcdt [[buffer(1)]],
                                     const device uint* nn [[buffer(2)]],
                                     const device float* diffusion [[buffer(3)]],
                                     constant uint& nPixels [[buffer(4)]],
                                     uint gid [[thread_position_in_grid]]) {
  if (gid >= nPixels) {
    return;
  }
  const uint iupx = nn[6 * gid];
  const uint idnx = nn[6 * gid + 1];
  const uint iupy = nn[6 * gid + 2];
  const uint idny = nn[6 * gid + 3];
  const uint iupz = nn[6 * gid + 4];
  const uint idnz = nn[6 * gid + 5];
  const uint iCenter = gid * N_SPECIES;
  const uint iUpx = iupx * N_SPECIES;
  const uint iDnx = idnx * N_SPECIES;
  const uint iUpy = iupy * N_SPECIES;
  const uint iDny = idny * N_SPECIES;
  const uint iUpz = iupz * N_SPECIES;
  const uint iDnz = idnz * N_SPECIES;
  for (uint is = 0; is < N_SPECIES; ++is) {
    const float c0 = conc[iCenter + is];
    dcdt[iCenter + is] +=
        diffusion[3 * is] *
            (conc[iUpx + is] + conc[iDnx + is] - 2.0f * c0) +
        diffusion[3 * is + 1] *
            (conc[iUpy + is] + conc[iDny + is] - 2.0f * c0) +
        diffusion[3 * is + 2] *
            (conc[iUpz + is] + conc[iDnz + is] - 2.0f * c0);
  }
}

kernel void rk101_update_kernel(device float* conc [[buffer(0)]],
                                const device float* dcdt [[buffer(1)]],
                                constant float& dt [[buffer(2)]],
                                constant uint& nValues [[buffer(3)]],
                                uint gid [[thread_position_in_grid]]) {
  if (gid >= nValues) {
    return;
  }
  conc[gid] += dt * dcdt[gid];
}

kernel void rk212_substep1_kernel(device float* conc [[buffer(0)]],
                                  const device float* dcdt [[buffer(1)]],
                                  device float* sme_lower_order [[buffer(2)]],
                                  device float* sme_old_conc [[buffer(3)]],
                                  constant float& dt [[buffer(4)]],
                                  constant uint& nValues [[buffer(5)]],
                                  uint gid [[thread_position_in_grid]]) {
  if (gid >= nValues) {
    return;
  }
  sme_old_conc[gid] = conc[gid];
  conc[gid] += dt * dcdt[gid];
  (void)sme_lower_order;
}

kernel void rk212_substep2_kernel(device float* conc [[buffer(0)]],
                                  const device float* dcdt [[buffer(1)]],
                                  device float* sme_lower_order [[buffer(2)]],
                                  const device float* sme_old_conc [[buffer(3)]],
                                  constant float& dt [[buffer(4)]],
                                  constant uint& nValues [[buffer(5)]],
                                  uint gid [[thread_position_in_grid]]) {
  if (gid >= nValues) {
    return;
  }
  sme_lower_order[gid] = conc[gid];
  conc[gid] = 0.5f * sme_old_conc[gid] + 0.5f * conc[gid] + 0.5f * dt * dcdt[gid];
}

kernel void rk212_error_kernel(const device float* conc [[buffer(0)]],
                               const device float* sme_lower_order [[buffer(1)]],
                               const device float* sme_old_conc [[buffer(2)]],
                               constant float& epsilon [[buffer(3)]],
                               device float* abs_error_blocks [[buffer(4)]],
                               device float* rel_error_blocks [[buffer(5)]],
                               constant uint& nValues [[buffer(6)]],
                               uint3 gid [[thread_position_in_grid]],
                               uint3 tid [[thread_position_in_threadgroup]],
                               uint3 tg_pos [[threadgroup_position_in_grid]],
                               uint3 tg_size [[threads_per_threadgroup]]) {
  threadgroup float abs_error_shared[RK_ERROR_BLOCK_SIZE];
  threadgroup float rel_error_shared[RK_ERROR_BLOCK_SIZE];
  float localAbsError = 0.0f;
  float localRelError = 0.0f;
  if (gid.x < nValues) {
    localAbsError = fabs(conc[gid.x] - sme_lower_order[gid.x]);
    const float localNorm =
        0.5f * (conc[gid.x] + sme_old_conc[gid.x] + epsilon);
    localRelError = localAbsError / localNorm;
  }
  abs_error_shared[tid.x] = localAbsError;
  rel_error_shared[tid.x] = localRelError;
  threadgroup_barrier(mem_flags::mem_threadgroup);
  for (uint stride = tg_size.x / 2; stride > 0; stride /= 2) {
    if (tid.x < stride) {
      abs_error_shared[tid.x] =
          fmax(abs_error_shared[tid.x], abs_error_shared[tid.x + stride]);
      rel_error_shared[tid.x] =
          fmax(rel_error_shared[tid.x], rel_error_shared[tid.x + stride]);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
  }
  if (tid.x == 0) {
    abs_error_blocks[tg_pos.x] = abs_error_shared[0];
    rel_error_blocks[tg_pos.x] = rel_error_shared[0];
  }
}

kernel void rk212_error_reduce_kernel(const device float* abs_error_blocks [[buffer(0)]],
                                      const device float* rel_error_blocks [[buffer(1)]],
                                      device float* result [[buffer(2)]],
                                      constant uint& nBlocks [[buffer(3)]],
                                      uint3 tid [[thread_position_in_threadgroup]],
                                      uint3 tg_size [[threads_per_threadgroup]]) {
  threadgroup float abs_shared[RK_ERROR_BLOCK_SIZE];
  threadgroup float rel_shared[RK_ERROR_BLOCK_SIZE];
  float localAbs = 0.0f;
  float localRel = 0.0f;
  for (uint i = tid.x; i < nBlocks; i += tg_size.x) {
    localAbs = fmax(localAbs, abs_error_blocks[i]);
    localRel = fmax(localRel, rel_error_blocks[i]);
  }
  abs_shared[tid.x] = localAbs;
  rel_shared[tid.x] = localRel;
  threadgroup_barrier(mem_flags::mem_threadgroup);
  for (uint stride = tg_size.x / 2; stride > 0; stride /= 2) {
    if (tid.x < stride) {
      abs_shared[tid.x] = fmax(abs_shared[tid.x], abs_shared[tid.x + stride]);
      rel_shared[tid.x] = fmax(rel_shared[tid.x], rel_shared[tid.x + stride]);
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
  }
  if (tid.x == 0) {
    result[0] = abs_shared[0];
    result[1] = rel_shared[0];
  }
}

kernel void rk_init_kernel(const device float* conc [[buffer(0)]],
                           device float* s2 [[buffer(1)]],
                           device float* s3 [[buffer(2)]],
                           constant uint& nValues [[buffer(3)]],
                           uint gid [[thread_position_in_grid]]) {
  if (gid >= nValues) {
    return;
  }
  s3[gid] = conc[gid];
  s2[gid] = 0.0f;
}

kernel void rk_substep_kernel(device float* conc [[buffer(0)]],
                              const device float* dcdt [[buffer(1)]],
                              device float* s2 [[buffer(2)]],
                              const device float* s3 [[buffer(3)]],
                              constant float& dt [[buffer(4)]],
                              constant float& g1 [[buffer(5)]],
                              constant float& g2 [[buffer(6)]],
                              constant float& g3 [[buffer(7)]],
                              constant float& beta [[buffer(8)]],
                              constant float& delta [[buffer(9)]],
                              constant uint& nValues [[buffer(10)]],
                              uint gid [[thread_position_in_grid]]) {
  if (gid >= nValues) {
    return;
  }
  s2[gid] += delta * conc[gid];
  conc[gid] = g1 * conc[gid] + g2 * s2[gid] + g3 * s3[gid] + beta * dt * dcdt[gid];
}

kernel void rk_finalise_kernel(const device float* conc [[buffer(0)]],
                               device float* s2 [[buffer(1)]],
                               const device float* s3 [[buffer(2)]],
                               constant float& cFactor [[buffer(3)]],
                               constant float& s2Factor [[buffer(4)]],
                               constant float& s3Factor [[buffer(5)]],
                               constant uint& nValues [[buffer(6)]],
                               uint gid [[thread_position_in_grid]]) {
  if (gid >= nValues) {
    return;
  }
  s2[gid] = cFactor * conc[gid] + s2Factor * s2[gid] + s3Factor * s3[gid];
}

kernel void clamp_negative_kernel(device float* conc [[buffer(0)]],
                                  constant uint& nPixels [[buffer(1)]],
                                  uint gid [[thread_position_in_grid]]) {
  if (gid >= nPixels) {
    return;
  }
  const uint offset = gid * N_SPECIES;
  for (uint is = 0; is < N_SPECIES; ++is) {
    if (conc[offset + is] < 0.0f) {
      conc[offset + is] = 0.0f;
    }
  }
}
)";
  return src.str();
}

std::string detail::makeMetalCompileFailureMessage(std::string_view context,
                                                   std::string_view error) {
  return fmt::format("{}: {}", context, error);
}

namespace {

static std::string
makeMetalMembraneKernelSource(const std::vector<std::string> &variables,
                              const std::vector<std::string> &expressions,
                              unsigned int nSpeciesA, unsigned int nSpeciesB) {
  const auto bundle = makeMetalGeneratedExpressionBundle(
      variables, expressions, "membrane reaction expression");

  std::ostringstream src;
  src << "#include <metal_stdlib>\n";
  src << "using namespace metal;\n\n";
  src << "#define N_MEMBRANE_INPUTS " << bundle.safeVariables.size() << "\n";
  src << "#define N_SPECIES_A " << nSpeciesA << "\n";
  src << "#define N_SPECIES_B " << nSpeciesB << "\n\n";
  src << "inline void membrane_reaction_eval(thread float* sme_out, "
         "const thread float* sme_values) {\n";
  for (std::size_t i = 0; i < bundle.safeVariables.size(); ++i) {
    src << "  const float " << bundle.safeVariables[i] << " = sme_values[" << i
        << "];\n";
  }
  for (std::size_t i = 0; i < bundle.metalExpressions.size(); ++i) {
    src << "  sme_out[" << i << "] = " << bundle.metalExpressions[i] << ";\n";
  }
  src << "}\n\n";
  src << R"(
kernel void membrane_reaction_kernel(const device uint* indexPairs [[buffer(0)]],
                                     constant uint& nPairs [[buffer(1)]],
                                     const device float* concA [[buffer(2)]],
                                     device atomic_float* dcdtA [[buffer(3)]],
                                     const device float* concB [[buffer(4)]],
                                     device atomic_float* dcdtB [[buffer(5)]],
                                     constant float& invFluxLength [[buffer(6)]],
                                     uint gid [[thread_position_in_grid]]) {
  if (gid >= nPairs) {
    return;
  }
  const uint ixA = indexPairs[2 * gid];
  const uint ixB = indexPairs[2 * gid + 1];
  thread float sme_inputs[N_MEMBRANE_INPUTS] {};
  thread float sme_result[N_MEMBRANE_INPUTS] {};
  const uint offsetA = ixA * N_SPECIES_A;
  const uint offsetB = ixB * N_SPECIES_B;
  for (uint is = 0; is < N_SPECIES_A; ++is) {
    sme_inputs[is] = concA[offsetA + is];
  }
  for (uint is = 0; is < N_SPECIES_B; ++is) {
    sme_inputs[N_SPECIES_A + is] = concB[offsetB + is];
  }
  membrane_reaction_eval(sme_result, sme_inputs);
  for (uint is = 0; is < N_SPECIES_A; ++is) {
    atomic_fetch_add_explicit(&(dcdtA[offsetA + is]),
                              sme_result[is] * invFluxLength,
                              memory_order_relaxed);
  }
  for (uint is = 0; is < N_SPECIES_B; ++is) {
    atomic_fetch_add_explicit(&(dcdtB[offsetB + is]),
                              sme_result[N_SPECIES_A + is] * invFluxLength,
                              memory_order_relaxed);
  }
}
)";
  return src.str();
}

struct MetalKernelBundle {
  MTL::ComputePipelineState *reaction{};
  MTL::ComputePipelineState *diffusionUniform{};
  MTL::ComputePipelineState *rk101Update{};
  MTL::ComputePipelineState *rk212Substep1{};
  MTL::ComputePipelineState *rk212Substep2{};
  MTL::ComputePipelineState *rk212Error{};
  MTL::ComputePipelineState *rk212ErrorReduce{};
  MTL::ComputePipelineState *rkInit{};
  MTL::ComputePipelineState *rkSubstep{};
  MTL::ComputePipelineState *rkFinalise{};
  MTL::ComputePipelineState *clampNegative{};
};

struct MetalMembraneKernelBundle {
  MTL::ComputePipelineState *reaction{};
};

static MTL::ComputePipelineState *makePipeline(MTL::Device *device,
                                               MTL::Library *library,
                                               const char *functionName,
                                               const std::string &context) {
  auto *function = library->newFunction(
      NS::String::string(functionName, NS::UTF8StringEncoding));
  if (function == nullptr) {
    throw MetalPixelSimError(
        fmt::format("Failed to load Metal function '{}'", functionName));
  }
  NS::Error *error{};
  auto *pipeline = device->newComputePipelineState(function, &error);
  function->release();
  throwIfMetalFailed(pipeline != nullptr, context, error);
  return pipeline;
}

static MetalKernelBundle compileMetalKernelBundle(MTL::Device *device,
                                                  const ReacExpr &reacExpr) {
  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
  auto source =
      detail::makeMetalKernelSource(reacExpr.variables, reacExpr.expressions);
  NS::Error *error{};
  auto *library = device->newLibrary(
      NS::String::string(source.c_str(), NS::UTF8StringEncoding), nullptr,
      &error);
  throwIfMetalFailed(library != nullptr, "Failed to compile Metal pixel kernel",
                     error);

  MetalKernelBundle bundle;
  bundle.reaction = makePipeline(device, library, "reaction_kernel",
                                 "Failed to create Metal reaction pipeline");
  bundle.diffusionUniform =
      makePipeline(device, library, "diffusion_uniform_kernel",
                   "Failed to create Metal diffusion pipeline");
  bundle.rk101Update =
      makePipeline(device, library, "rk101_update_kernel",
                   "Failed to create Metal RK101 update pipeline");
  bundle.rk212Substep1 =
      makePipeline(device, library, "rk212_substep1_kernel",
                   "Failed to create Metal RK212 substep 1 pipeline");
  bundle.rk212Substep2 =
      makePipeline(device, library, "rk212_substep2_kernel",
                   "Failed to create Metal RK212 substep 2 pipeline");
  bundle.rk212Error =
      makePipeline(device, library, "rk212_error_kernel",
                   "Failed to create Metal RK212 error pipeline");
  bundle.rk212ErrorReduce =
      makePipeline(device, library, "rk212_error_reduce_kernel",
                   "Failed to create Metal RK212 error reduction pipeline");
  bundle.rkInit = makePipeline(device, library, "rk_init_kernel",
                               "Failed to create Metal RK init pipeline");
  bundle.rkSubstep = makePipeline(device, library, "rk_substep_kernel",
                                  "Failed to create Metal RK substep pipeline");
  bundle.rkFinalise =
      makePipeline(device, library, "rk_finalise_kernel",
                   "Failed to create Metal RK finalise pipeline");
  bundle.clampNegative = makePipeline(device, library, "clamp_negative_kernel",
                                      "Failed to create Metal clamp pipeline");
  library->release();
  pool->release();
  return bundle;
}

static MetalMembraneKernelBundle
compileMetalMembraneKernelBundle(MTL::Device *device, const ReacExpr &reacExpr,
                                 unsigned int nSpeciesA,
                                 unsigned int nSpeciesB) {
  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
  auto source = makeMetalMembraneKernelSource(
      reacExpr.variables, reacExpr.expressions, nSpeciesA, nSpeciesB);
  NS::Error *error{};
  auto *library = device->newLibrary(
      NS::String::string(source.c_str(), NS::UTF8StringEncoding), nullptr,
      &error);
  throwIfMetalFailed(library != nullptr,
                     "Failed to compile Metal membrane pixel kernel", error);

  MetalMembraneKernelBundle bundle;
  bundle.reaction = makePipeline(device, library, "membrane_reaction_kernel",
                                 "Failed to create Metal membrane pipeline");
  library->release();
  pool->release();
  return bundle;
}

} // namespace

struct MetalPixelSim::Impl {
  struct CompartmentState {
    std::string compartmentId{};
    std::size_t nPixels{};
    std::size_t nSpecies{};
    std::vector<double> concHost{};
    std::vector<double> dcdtHost{};
    std::vector<double> lowerOrderHost{};
    std::vector<std::uint32_t> nnHost{};
    std::vector<double> diffusionHost{};
    std::size_t nErrorBlocks{};
    double maxStableTimestep{std::numeric_limits<double>::max()};
    MetalKernelBundle kernels{};
    MTL::Buffer *stagingFloat{};
    MTL::Buffer *dConc{};
    MTL::Buffer *dDcdt{};
    MTL::Buffer *dLowerOrder{};
    MTL::Buffer *dOldConc{};
    MTL::Buffer *dNn{};
    MTL::Buffer *dDiffusion{};
    MTL::Buffer *dErrorAbs{};
    MTL::Buffer *dErrorRel{};
    MTL::Buffer *dErrorResult{};
  };

  struct MembraneState {
    std::string membraneId{};
    std::size_t compartmentIndexA{invalidCompartmentIndex};
    std::size_t compartmentIndexB{invalidCompartmentIndex};
    std::size_t nSpeciesA{};
    std::size_t nSpeciesB{};
    std::array<std::uint32_t, 6> nFacePairs{};
    std::array<float, 6> faceInvFluxLengths{};
    MetalMembraneKernelBundle kernels{};
    std::array<MTL::Buffer *, 6> dFaceIndexPairs{};
  };

  std::vector<CompartmentState> compartments{};
  std::vector<MembraneState> membranes{};
  MTL::Device *device{};
  MTL::CommandQueue *queue{};
  MTL::Buffer *dummyBuffer{};

  ~Impl() {
    for (auto &compartment : compartments) {
      if (compartment.kernels.reaction != nullptr) {
        compartment.kernels.reaction->release();
      }
      if (compartment.kernels.diffusionUniform != nullptr) {
        compartment.kernels.diffusionUniform->release();
      }
      if (compartment.kernels.rk101Update != nullptr) {
        compartment.kernels.rk101Update->release();
      }
      if (compartment.kernels.rk212Substep1 != nullptr) {
        compartment.kernels.rk212Substep1->release();
      }
      if (compartment.kernels.rk212Substep2 != nullptr) {
        compartment.kernels.rk212Substep2->release();
      }
      if (compartment.kernels.rk212Error != nullptr) {
        compartment.kernels.rk212Error->release();
      }
      if (compartment.kernels.rk212ErrorReduce != nullptr) {
        compartment.kernels.rk212ErrorReduce->release();
      }
      if (compartment.kernels.rkInit != nullptr) {
        compartment.kernels.rkInit->release();
      }
      if (compartment.kernels.rkSubstep != nullptr) {
        compartment.kernels.rkSubstep->release();
      }
      if (compartment.kernels.rkFinalise != nullptr) {
        compartment.kernels.rkFinalise->release();
      }
      if (compartment.kernels.clampNegative != nullptr) {
        compartment.kernels.clampNegative->release();
      }
      if (compartment.stagingFloat != nullptr) {
        compartment.stagingFloat->release();
      }
      if (compartment.dConc != nullptr) {
        compartment.dConc->release();
      }
      if (compartment.dDcdt != nullptr) {
        compartment.dDcdt->release();
      }
      if (compartment.dLowerOrder != nullptr) {
        compartment.dLowerOrder->release();
      }
      if (compartment.dOldConc != nullptr) {
        compartment.dOldConc->release();
      }
      if (compartment.dNn != nullptr) {
        compartment.dNn->release();
      }
      if (compartment.dDiffusion != nullptr) {
        compartment.dDiffusion->release();
      }
      if (compartment.dErrorAbs != nullptr) {
        compartment.dErrorAbs->release();
      }
      if (compartment.dErrorRel != nullptr) {
        compartment.dErrorRel->release();
      }
      if (compartment.dErrorResult != nullptr) {
        compartment.dErrorResult->release();
      }
    }
    for (auto &membrane : membranes) {
      if (membrane.kernels.reaction != nullptr) {
        membrane.kernels.reaction->release();
      }
      for (auto *buffer : membrane.dFaceIndexPairs) {
        if (buffer != nullptr) {
          buffer->release();
        }
      }
    }
    if (dummyBuffer != nullptr) {
      dummyBuffer->release();
    }
    if (queue != nullptr) {
      queue->release();
    }
    if (device != nullptr) {
      device->release();
    }
  }
};

MetalPixelSim::MetalPixelSim(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    const std::map<std::string, double, std::less<>> &substitutions)
    : PixelSimBase{sbmlDoc.getSimulationSettings().options.pixel.integrator,
                   sbmlDoc.getSimulationSettings().options.pixel.maxErr,
                   sbmlDoc.getSimulationSettings().options.pixel.maxTimestep},
      impl(std::make_unique<Impl>()), doc{sbmlDoc} {
  try {
    if (sbmlDoc.getSimulationSettings().options.pixel.backend !=
        PixelBackendType::GPU) {
      throw MetalPixelSimError("Metal pixel backend was not selected");
    }
    if (hasAnyCrossDiffusion(doc, compartmentSpeciesIds)) {
      throw MetalPixelSimError(
          "Metal pixel backend PoC does not yet support cross-diffusion");
    }
    const auto xId{doc.getParameters().getSpatialCoordinates().x.id};
    const auto yId{doc.getParameters().getSpatialCoordinates().y.id};
    const auto zId{doc.getParameters().getSpatialCoordinates().z.id};
    if (doc.getReactions().dependOnVariable("time")) {
      throw MetalPixelSimError(
          "Metal pixel backend PoC does not yet support time-dependent "
          "reaction terms");
    }
    if (doc.getReactions().dependOnVariable(xId.c_str()) ||
        doc.getReactions().dependOnVariable(yId.c_str()) ||
        doc.getReactions().dependOnVariable(zId.c_str())) {
      throw MetalPixelSimError(
          "Metal pixel backend PoC does not yet support spatially dependent "
          "reaction terms");
    }
    detail::validatePocCompartmentInputs<MetalPixelSimError>(
        doc, compartmentIds, compartmentSpeciesIds, "Metal");

    NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
    impl->device = MTL::CreateSystemDefaultDevice();
    throwIfMetalFailed(impl->device != nullptr,
                       "Failed to create the Metal device");
    impl->queue = impl->device->newCommandQueue();
    throwIfMetalFailed(impl->queue != nullptr,
                       "Failed to create the Metal command queue");
    impl->dummyBuffer =
        impl->device->newBuffer(sizeof(float), MTL::ResourceStorageModeShared);
    throwIfMetalFailed(impl->dummyBuffer != nullptr,
                       "Failed to allocate Metal dummy buffer");
    *bufferData<float>(impl->dummyBuffer) = 0.0f;
    pool->release();

    const auto voxelSize{doc.getGeometry().getVoxelSize()};
    const double dx2 = voxelSize.width() * voxelSize.width();
    const double dy2 = voxelSize.height() * voxelSize.height();
    const double dz2 = voxelSize.depth() * voxelSize.depth();
    std::map<std::string, std::size_t, std::less<>> compartmentIndexById;
    for (std::size_t compIndex = 0; compIndex < compartmentIds.size();
         ++compIndex) {
      const auto &speciesIds{compartmentSpeciesIds[compIndex]};
      const auto *compartment{doc.getCompartments().getCompartment(
          compartmentIds[compIndex].c_str())};
      Impl::CompartmentState state;
      state.compartmentId = compartment->getId();
      state.nPixels = compartment->nVoxels();
      state.nSpecies = speciesIds.size();
      state.concHost.resize(state.nPixels * state.nSpecies);
      state.dcdtHost.assign(state.concHost.size(), 0.0);
      state.nErrorBlocks =
          (state.concHost.size() + metalKernelThreadgroupSize - 1) /
          metalKernelThreadgroupSize;
      state.nnHost.reserve(6 * state.nPixels);
      state.diffusionHost.reserve(3 * state.nSpecies);
      for (std::size_t i = 0; i < state.nPixels; ++i) {
        state.nnHost.push_back(
            static_cast<std::uint32_t>(compartment->up_x(i)));
        state.nnHost.push_back(
            static_cast<std::uint32_t>(compartment->dn_x(i)));
        state.nnHost.push_back(
            static_cast<std::uint32_t>(compartment->up_y(i)));
        state.nnHost.push_back(
            static_cast<std::uint32_t>(compartment->dn_y(i)));
        state.nnHost.push_back(
            static_cast<std::uint32_t>(compartment->up_z(i)));
        state.nnHost.push_back(
            static_cast<std::uint32_t>(compartment->dn_z(i)));
      }

      std::vector<const geometry::Field *> fields;
      fields.reserve(state.nSpecies);
      for (const auto &speciesId : speciesIds) {
        const auto *field = doc.getSpecies().getField(speciesId.c_str());
        if (field == nullptr) {
          throw MetalPixelSimError(fmt::format(
              "Metal pixel backend PoC could not find field for species '{}'",
              speciesId));
        }
        if (!field->getIsSpatial()) {
          throw MetalPixelSimError(
              "Metal pixel backend PoC does not yet support non-spatial "
              "species");
        }
        if (!field->getIsUniformDiffusionConstant()) {
          throw MetalPixelSimError(
              "Metal pixel backend PoC does not yet support non-uniform "
              "diffusion constants");
        }
        if (doc.getSpecies().getStorage(speciesId.c_str()) != 1.0) {
          throw MetalPixelSimError(
              "Metal pixel backend PoC currently requires unit storage for all "
              "species");
        }
        fields.push_back(field);
        const auto &diffArray = field->getDiffusionConstant();
        const double d = diffArray.empty() ? 0.0 : diffArray.front();
        state.diffusionHost.push_back(d / dx2);
        state.diffusionHost.push_back(d / dy2);
        state.diffusionHost.push_back(d / dz2);
        if (d > 0.0) {
          state.maxStableTimestep =
              std::min(state.maxStableTimestep,
                       calculateMaxStableTimestep({d / dx2, d / dy2, d / dz2}));
        }
      }
      if (state.maxStableTimestep == std::numeric_limits<double>::max()) {
        state.maxStableTimestep = std::numeric_limits<double>::max();
      }
      maxStableTimestep = std::min(maxStableTimestep, state.maxStableTimestep);

      auto concIter{state.concHost.begin()};
      for (std::size_t ix = 0; ix < state.nPixels; ++ix) {
        for (const auto *field : fields) {
          *concIter = field->getConcentration()[ix];
          ++concIter;
        }
      }
      assert(concIter == state.concHost.end());

      std::vector<std::string> reactionIDs;
      if (auto reacsInCompartment =
              doc.getReactions().getIds(state.compartmentId.c_str());
          !reacsInCompartment.isEmpty()) {
        reactionIDs = common::toStdString(reacsInCompartment);
      }
      const ReacExpr reacExpr(doc, speciesIds, reactionIDs, 1.0, false, false,
                              substitutions);
      state.kernels = compileMetalKernelBundle(impl->device, reacExpr);

      state.stagingFloat =
          newSharedBuffer<float>(impl->device, state.concHost.size(),
                                 "Failed to allocate Metal staging buffer");
      state.dConc = newPrivateBuffer<float>(
          impl->device, state.concHost.size(),
          "Failed to allocate Metal concentration buffer");
      state.dDcdt =
          newPrivateBuffer<float>(impl->device, state.dcdtHost.size(),
                                  "Failed to allocate Metal dcdt buffer");
      state.dLowerOrder = newPrivateBuffer<float>(
          impl->device, state.concHost.size(),
          "Failed to allocate Metal RK lower-order buffer");
      state.dOldConc = newPrivateBuffer<float>(
          impl->device, state.concHost.size(),
          "Failed to allocate Metal RK old concentration buffer");
      state.dNn = newPrivateBuffer<std::uint32_t>(
          impl->device, state.nnHost.size(),
          "Failed to allocate Metal neighbor buffer");
      state.dDiffusion =
          newPrivateBuffer<float>(impl->device, state.diffusionHost.size(),
                                  "Failed to allocate Metal diffusion buffer");
      state.dErrorAbs = newPrivateBuffer<float>(
          impl->device, state.nErrorBlocks,
          "Failed to allocate Metal RK absolute error buffer");
      state.dErrorRel = newPrivateBuffer<float>(
          impl->device, state.nErrorBlocks,
          "Failed to allocate Metal RK relative error buffer");
      state.dErrorResult = newSharedBuffer<float>(
          impl->device, 2, "Failed to allocate Metal RK error result buffer");

      uploadDoublesToBuffer(state.stagingFloat, state.concHost,
                            "Failed to upload initial Metal concentrations");
      MTL::Buffer *nnUpload{};
      MTL::Buffer *diffusionUpload{};
      try {
        if (!state.nnHost.empty()) {
          nnUpload = newSharedBuffer<std::uint32_t>(
              impl->device, state.nnHost.size(),
              "Failed to allocate Metal neighbor upload buffer");
          uploadVectorToBuffer(nnUpload, state.nnHost,
                               "Failed to upload Metal neighbor topology");
        }
        if (!state.diffusionHost.empty()) {
          diffusionUpload =
              newSharedBuffer<float>(impl->device, state.diffusionHost.size(),
                                     "Failed to allocate Metal diffusion "
                                     "upload buffer");
          uploadDoublesToBuffer(diffusionUpload, state.diffusionHost,
                                "Failed to upload Metal diffusion "
                                "coefficients");
        }
        runBlitCommand(
            impl->queue,
            fmt::format("Failed to initialise Metal buffers for '{}'",
                        state.compartmentId),
            [&](MTL::BlitCommandEncoder *encoder) {
              const auto concBytes =
                  bufferByteCount<float>(state.concHost.size());
              const auto dcdtBytes =
                  bufferByteCount<float>(state.dcdtHost.size());
              const auto nnBytes =
                  bufferByteCount<std::uint32_t>(state.nnHost.size());
              const auto diffusionBytes =
                  bufferByteCount<float>(state.diffusionHost.size());
              const auto errorBytes =
                  bufferByteCount<float>(state.nErrorBlocks);
              if (concBytes != 0) {
                encoder->copyFromBuffer(state.stagingFloat, 0, state.dConc, 0,
                                        static_cast<NS::UInteger>(concBytes));
                encoder->fillBuffer(
                    state.dLowerOrder,
                    NS::Range::Make(0, static_cast<NS::UInteger>(concBytes)),
                    0);
                encoder->copyFromBuffer(state.dConc, 0, state.dOldConc, 0,
                                        static_cast<NS::UInteger>(concBytes));
              }
              if (dcdtBytes != 0) {
                encoder->fillBuffer(
                    state.dDcdt,
                    NS::Range::Make(0, static_cast<NS::UInteger>(dcdtBytes)),
                    0);
              }
              if (nnUpload != nullptr && nnBytes != 0) {
                encoder->copyFromBuffer(nnUpload, 0, state.dNn, 0,
                                        static_cast<NS::UInteger>(nnBytes));
              }
              if (diffusionUpload != nullptr && diffusionBytes != 0) {
                encoder->copyFromBuffer(
                    diffusionUpload, 0, state.dDiffusion, 0,
                    static_cast<NS::UInteger>(diffusionBytes));
              }
              if (errorBytes != 0) {
                encoder->fillBuffer(
                    state.dErrorAbs,
                    NS::Range::Make(0, static_cast<NS::UInteger>(errorBytes)),
                    0);
                encoder->fillBuffer(
                    state.dErrorRel,
                    NS::Range::Make(0, static_cast<NS::UInteger>(errorBytes)),
                    0);
              }
            });
      } catch (...) {
        if (nnUpload != nullptr) {
          nnUpload->release();
        }
        if (diffusionUpload != nullptr) {
          diffusionUpload->release();
        }
        throw;
      }
      if (nnUpload != nullptr) {
        nnUpload->release();
      }
      if (diffusionUpload != nullptr) {
        diffusionUpload->release();
      }
      std::fill_n(bufferData<float>(state.dErrorResult), 2, 0.0f);

      compartmentIndexById.emplace(state.compartmentId,
                                   impl->compartments.size());
      impl->compartments.push_back(std::move(state));
    }

    const auto &lengthUnit = doc.getUnits().getLength();
    const auto &volumeUnit = doc.getUnits().getVolume();
    const double volOverL3 = model::getVolOverL3(lengthUnit, volumeUnit);
    for (const auto &membrane : doc.getMembranes().getMembranes()) {
      if (auto reacsInMembrane =
              doc.getReactions().getIds(membrane.getId().c_str());
          !reacsInMembrane.isEmpty()) {
        Impl::MembraneState state;
        state.membraneId = membrane.getId();

        if (auto iter =
                compartmentIndexById.find(membrane.getCompartmentA()->getId());
            iter != compartmentIndexById.end()) {
          state.compartmentIndexA = iter->second;
          state.nSpeciesA = impl->compartments[iter->second].nSpecies;
        }
        if (auto iter =
                compartmentIndexById.find(membrane.getCompartmentB()->getId());
            iter != compartmentIndexById.end()) {
          state.compartmentIndexB = iter->second;
          state.nSpeciesB = impl->compartments[iter->second].nSpecies;
        }

        std::vector<std::string> membraneSpeciesIds;
        if (state.compartmentIndexA != invalidCompartmentIndex) {
          const auto &speciesIds =
              compartmentSpeciesIds[state.compartmentIndexA];
          membraneSpeciesIds.insert(membraneSpeciesIds.end(),
                                    speciesIds.begin(), speciesIds.end());
        }
        if (state.compartmentIndexB != invalidCompartmentIndex) {
          const auto &speciesIds =
              compartmentSpeciesIds[state.compartmentIndexB];
          membraneSpeciesIds.insert(membraneSpeciesIds.end(),
                                    speciesIds.begin(), speciesIds.end());
        }
        if (membraneSpeciesIds.empty()) {
          continue;
        }

        const ReacExpr reacExpr(doc, membraneSpeciesIds,
                                common::toStdString(reacsInMembrane), volOverL3,
                                false, false, substitutions);
        state.kernels = compileMetalMembraneKernelBundle(
            impl->device, reacExpr, static_cast<unsigned int>(state.nSpeciesA),
            static_cast<unsigned int>(state.nSpeciesB));

        for (const auto faceDirection : allMembraneFaceDirections) {
          const auto faceIndex = static_cast<std::size_t>(faceDirection);
          const auto &facePairs{membrane.getFaceIndexPairs(faceDirection)};
          state.nFacePairs[faceIndex] =
              static_cast<std::uint32_t>(facePairs.size());
          if (facePairs.empty()) {
            continue;
          }
          state.faceInvFluxLengths[faceIndex] = static_cast<float>(
              1.0 / getFaceFluxLength(faceDirection, voxelSize));
          std::vector<std::uint32_t> flatPairs;
          flatPairs.reserve(2 * facePairs.size());
          for (const auto &[ixA, ixB] : facePairs) {
            flatPairs.push_back(static_cast<std::uint32_t>(ixA));
            flatPairs.push_back(static_cast<std::uint32_t>(ixB));
          }
          state.dFaceIndexPairs[faceIndex] = newPrivateBuffer<std::uint32_t>(
              impl->device, flatPairs.size(),
              fmt::format("Failed to allocate Metal membrane pair buffer for "
                          "'{}'",
                          state.membraneId));
          auto *faceUpload = newSharedBuffer<std::uint32_t>(
              impl->device, flatPairs.size(),
              fmt::format("Failed to allocate Metal membrane upload buffer for "
                          "'{}'",
                          state.membraneId));
          try {
            uploadVectorToBuffer(
                faceUpload, flatPairs,
                fmt::format("Failed to upload Metal membrane topology "
                            "for '{}'",
                            state.membraneId));
            copyBuffer(
                impl->queue, faceUpload, state.dFaceIndexPairs[faceIndex],
                bufferByteCount<std::uint32_t>(flatPairs.size()),
                fmt::format("Failed to copy Metal membrane topology for '{}'",
                            state.membraneId));
          } catch (...) {
            faceUpload->release();
            throw;
          }
          faceUpload->release();
        }
        impl->membranes.push_back(std::move(state));
      }
    }

    const auto &data{sbmlDoc.getSimulationData()};
    if (data.concentration.size() > 1 && !data.concentration.back().empty() &&
        data.concentration.back().size() == impl->compartments.size()) {
      SPDLOG_INFO("Applying supplied initial concentrations to Metal backend");
      for (std::size_t i = 0; i < impl->compartments.size(); ++i) {
        auto &state = impl->compartments[i];
        if (data.concentration.back()[i].size() == state.concHost.size()) {
          state.concHost = data.concentration.back()[i];
          uploadDoublesToBuffer(
              state.stagingFloat, state.concHost,
              "Failed to upload supplied Metal concentrations");
          runBlitCommand(
              impl->queue,
              fmt::format("Failed to apply supplied Metal concentrations for "
                          "'{}'",
                          state.compartmentId),
              [&](MTL::BlitCommandEncoder *encoder) {
                const auto concBytes =
                    bufferByteCount<float>(state.concHost.size());
                if (concBytes == 0) {
                  return;
                }
                encoder->copyFromBuffer(state.stagingFloat, 0, state.dConc, 0,
                                        static_cast<NS::UInteger>(concBytes));
                encoder->fillBuffer(
                    state.dLowerOrder,
                    NS::Range::Make(0, static_cast<NS::UInteger>(concBytes)),
                    0);
                encoder->copyFromBuffer(state.dConc, 0, state.dOldConc, 0,
                                        static_cast<NS::UInteger>(concBytes));
              });
        }
      }
    }
  } catch (const std::exception &e) {
    SPDLOG_ERROR("Metal pixel backend setup failed: {}", e.what());
    currentErrorMessage = e.what();
  }
}

MetalPixelSim::~MetalPixelSim() = default;

void MetalPixelSim::encodeEvaluateDcdt(MTL::ComputeCommandEncoder *encoder) {
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.reaction);
    encoder->setComputePipelineState(state.kernels.reaction);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dDcdt, 0, 1);
    auto nPixels = static_cast<std::uint32_t>(state.nPixels);
    encoder->setBytes(&nPixels, sizeof(nPixels), 2);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.nPixels, groupSize),
        makeThreadgroupSize(groupSize));

    groupSize = threadgroupSizeForPipeline(state.kernels.diffusionUniform);
    encoder->setComputePipelineState(state.kernels.diffusionUniform);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dDcdt, 0, 1);
    encoder->setBuffer(state.dNn, 0, 2);
    encoder->setBuffer(state.dDiffusion, 0, 3);
    encoder->setBytes(&nPixels, sizeof(nPixels), 4);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.nPixels, groupSize),
        makeThreadgroupSize(groupSize));
  }

  for (auto &membrane : impl->membranes) {
    for (std::size_t faceIndex = 0; faceIndex < 6; ++faceIndex) {
      const auto nPairs = membrane.nFacePairs[faceIndex];
      if (nPairs == 0) {
        continue;
      }
      auto groupSize = threadgroupSizeForPipeline(membrane.kernels.reaction);
      encoder->setComputePipelineState(membrane.kernels.reaction);
      encoder->setBuffer(membrane.dFaceIndexPairs[faceIndex], 0, 0);
      encoder->setBytes(&membrane.nFacePairs[faceIndex], sizeof(std::uint32_t),
                        1);
      auto *concA = membrane.compartmentIndexA != invalidCompartmentIndex
                        ? impl->compartments[membrane.compartmentIndexA].dConc
                        : impl->dummyBuffer;
      auto *dcdtA = membrane.compartmentIndexA != invalidCompartmentIndex
                        ? impl->compartments[membrane.compartmentIndexA].dDcdt
                        : impl->dummyBuffer;
      auto *concB = membrane.compartmentIndexB != invalidCompartmentIndex
                        ? impl->compartments[membrane.compartmentIndexB].dConc
                        : impl->dummyBuffer;
      auto *dcdtB = membrane.compartmentIndexB != invalidCompartmentIndex
                        ? impl->compartments[membrane.compartmentIndexB].dDcdt
                        : impl->dummyBuffer;
      encoder->setBuffer(concA, 0, 2);
      encoder->setBuffer(dcdtA, 0, 3);
      encoder->setBuffer(concB, 0, 4);
      encoder->setBuffer(dcdtB, 0, 5);
      encoder->setBytes(&membrane.faceInvFluxLengths[faceIndex], sizeof(float),
                        6);
      encoder->dispatchThreadgroups(makeThreadgroupCount(nPairs, groupSize),
                                    makeThreadgroupSize(groupSize));
    }
  }
}

void MetalPixelSim::encodeRk101Update(MTL::ComputeCommandEncoder *encoder,
                                      double dt) {
  const float dtF = static_cast<float>(dt);
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.rk101Update);
    auto nValues = static_cast<std::uint32_t>(state.concHost.size());
    encoder->setComputePipelineState(state.kernels.rk101Update);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dDcdt, 0, 1);
    encoder->setBytes(&dtF, sizeof(dtF), 2);
    encoder->setBytes(&nValues, sizeof(nValues), 3);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.concHost.size(), groupSize),
        makeThreadgroupSize(groupSize));
  }
}

void MetalPixelSim::encodeClampNegative(MTL::ComputeCommandEncoder *encoder) {
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.clampNegative);
    auto nPixels = static_cast<std::uint32_t>(state.nPixels);
    encoder->setComputePipelineState(state.kernels.clampNegative);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBytes(&nPixels, sizeof(nPixels), 1);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.nPixels, groupSize),
        makeThreadgroupSize(groupSize));
  }
}

void MetalPixelSim::encodeRk212Substep1(MTL::ComputeCommandEncoder *encoder,
                                        double dt) {
  const float dtF = static_cast<float>(dt);
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.rk212Substep1);
    auto nValues = static_cast<std::uint32_t>(state.concHost.size());
    encoder->setComputePipelineState(state.kernels.rk212Substep1);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dDcdt, 0, 1);
    encoder->setBuffer(state.dLowerOrder, 0, 2);
    encoder->setBuffer(state.dOldConc, 0, 3);
    encoder->setBytes(&dtF, sizeof(dtF), 4);
    encoder->setBytes(&nValues, sizeof(nValues), 5);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.concHost.size(), groupSize),
        makeThreadgroupSize(groupSize));
  }
}

void MetalPixelSim::encodeRk212Substep2(MTL::ComputeCommandEncoder *encoder,
                                        double dt) {
  const float dtF = static_cast<float>(dt);
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.rk212Substep2);
    auto nValues = static_cast<std::uint32_t>(state.concHost.size());
    encoder->setComputePipelineState(state.kernels.rk212Substep2);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dDcdt, 0, 1);
    encoder->setBuffer(state.dLowerOrder, 0, 2);
    encoder->setBuffer(state.dOldConc, 0, 3);
    encoder->setBytes(&dtF, sizeof(dtF), 4);
    encoder->setBytes(&nValues, sizeof(nValues), 5);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.concHost.size(), groupSize),
        makeThreadgroupSize(groupSize));
  }
}

void MetalPixelSim::encodeRkInit(MTL::ComputeCommandEncoder *encoder) {
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.rkInit);
    auto nValues = static_cast<std::uint32_t>(state.concHost.size());
    encoder->setComputePipelineState(state.kernels.rkInit);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dLowerOrder, 0, 1);
    encoder->setBuffer(state.dOldConc, 0, 2);
    encoder->setBytes(&nValues, sizeof(nValues), 3);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.concHost.size(), groupSize),
        makeThreadgroupSize(groupSize));
  }
}

void MetalPixelSim::encodeRkSubstep(MTL::ComputeCommandEncoder *encoder,
                                    double dt, double g1Val, double g2Val,
                                    double g3Val, double betaVal,
                                    double deltaVal) {
  const float dtF = static_cast<float>(dt);
  const float g1F = static_cast<float>(g1Val);
  const float g2F = static_cast<float>(g2Val);
  const float g3F = static_cast<float>(g3Val);
  const float betaF = static_cast<float>(betaVal);
  const float deltaF = static_cast<float>(deltaVal);
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.rkSubstep);
    auto nValues = static_cast<std::uint32_t>(state.concHost.size());
    encoder->setComputePipelineState(state.kernels.rkSubstep);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dDcdt, 0, 1);
    encoder->setBuffer(state.dLowerOrder, 0, 2);
    encoder->setBuffer(state.dOldConc, 0, 3);
    encoder->setBytes(&dtF, sizeof(dtF), 4);
    encoder->setBytes(&g1F, sizeof(g1F), 5);
    encoder->setBytes(&g2F, sizeof(g2F), 6);
    encoder->setBytes(&g3F, sizeof(g3F), 7);
    encoder->setBytes(&betaF, sizeof(betaF), 8);
    encoder->setBytes(&deltaF, sizeof(deltaF), 9);
    encoder->setBytes(&nValues, sizeof(nValues), 10);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.concHost.size(), groupSize),
        makeThreadgroupSize(groupSize));
  }
}

void MetalPixelSim::encodeRkFinalise(MTL::ComputeCommandEncoder *encoder,
                                     double cFactor, double s2Factor,
                                     double s3Factor) {
  const float cFactorF = static_cast<float>(cFactor);
  const float s2FactorF = static_cast<float>(s2Factor);
  const float s3FactorF = static_cast<float>(s3Factor);
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.rkFinalise);
    auto nValues = static_cast<std::uint32_t>(state.concHost.size());
    encoder->setComputePipelineState(state.kernels.rkFinalise);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dLowerOrder, 0, 1);
    encoder->setBuffer(state.dOldConc, 0, 2);
    encoder->setBytes(&cFactorF, sizeof(cFactorF), 3);
    encoder->setBytes(&s2FactorF, sizeof(s2FactorF), 4);
    encoder->setBytes(&s3FactorF, sizeof(s3FactorF), 5);
    encoder->setBytes(&nValues, sizeof(nValues), 6);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.concHost.size(), groupSize),
        makeThreadgroupSize(groupSize));
  }
}

void MetalPixelSim::encodeRk212Error(MTL::ComputeCommandEncoder *encoder) {
  const float epsilonF = static_cast<float>(epsilon);
  for (auto &state : impl->compartments) {
    auto groupSize = threadgroupSizeForPipeline(state.kernels.rk212Error);
    auto nValues = static_cast<std::uint32_t>(state.concHost.size());
    encoder->setComputePipelineState(state.kernels.rk212Error);
    encoder->setBuffer(state.dConc, 0, 0);
    encoder->setBuffer(state.dLowerOrder, 0, 1);
    encoder->setBuffer(state.dOldConc, 0, 2);
    encoder->setBytes(&epsilonF, sizeof(epsilonF), 3);
    encoder->setBuffer(state.dErrorAbs, 0, 4);
    encoder->setBuffer(state.dErrorRel, 0, 5);
    encoder->setBytes(&nValues, sizeof(nValues), 6);
    encoder->dispatchThreadgroups(
        makeThreadgroupCount(state.concHost.size(), groupSize),
        makeThreadgroupSize(groupSize));

    groupSize = threadgroupSizeForPipeline(state.kernels.rk212ErrorReduce);
    auto nBlocks = static_cast<std::uint32_t>(state.nErrorBlocks);
    encoder->setComputePipelineState(state.kernels.rk212ErrorReduce);
    encoder->setBuffer(state.dErrorAbs, 0, 0);
    encoder->setBuffer(state.dErrorRel, 0, 1);
    encoder->setBuffer(state.dErrorResult, 0, 2);
    encoder->setBytes(&nBlocks, sizeof(nBlocks), 3);
    encoder->dispatchThreadgroups(MTL::Size::Make(1, 1, 1),
                                  makeThreadgroupSize(groupSize));
  }
}

PixelIntegratorError MetalPixelSim::readRk212ErrorResult() const {
  PixelIntegratorError err{0.0, 0.0};
  for (auto &state : impl->compartments) {
    const auto *result = bufferData<const float>(state.dErrorResult);
    err.abs = std::max(err.abs, static_cast<double>(result[0]));
    err.rel = std::max(err.rel, static_cast<double>(result[1]));
  }
  return err;
}

void MetalPixelSim::restorePreStepConcentrations() {
  runBlitCommand(
      impl->queue, "Failed to restore Metal pre-step concentrations",
      [&](MTL::BlitCommandEncoder *encoder) {
        for (auto &state : impl->compartments) {
          const auto concBytes = bufferByteCount<float>(state.concHost.size());
          if (concBytes == 0) {
            continue;
          }
          encoder->copyFromBuffer(state.dOldConc, 0, state.dConc, 0,
                                  static_cast<NS::UInteger>(concBytes));
        }
      });
}

void MetalPixelSim::downloadStateToHost() {
  runBlitCommand(
      impl->queue, "Failed to stage Metal concentrations for download",
      [&](MTL::BlitCommandEncoder *encoder) {
        for (auto &state : impl->compartments) {
          const auto concBytes = bufferByteCount<float>(state.concHost.size());
          if (concBytes == 0) {
            continue;
          }
          encoder->copyFromBuffer(state.dConc, 0, state.stagingFloat, 0,
                                  static_cast<NS::UInteger>(concBytes));
        }
      });
  for (auto &state : impl->compartments) {
    downloadBufferToDoubles(state.concHost, state.stagingFloat,
                            "Failed to download Metal concentrations");
  }
}

void MetalPixelSim::waitForDownload() {}

void MetalPixelSim::ensureDownloadComplete() {}

double MetalPixelSim::doRK101(double dt) {
  dt = std::min(dt, maxStableTimestep);
  ensureDownloadComplete();
  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
  auto *commandBuffer = impl->queue->commandBuffer();
  if (commandBuffer == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal command buffer");
  }
  auto *encoder = commandBuffer->computeCommandEncoder();
  if (encoder == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal compute encoder");
  }
  // Keep the full RK101 timestep on-GPU to avoid paying a command-buffer
  // completion round-trip after each stage.
  encodeEvaluateDcdt(encoder);
  encodeRk101Update(encoder, dt);
  encodeClampNegative(encoder);
  encoder->endEncoding();
  waitForCommandBuffer(commandBuffer, "Metal RK101 timestep failed");
  pool->release();
  return dt;
}

void MetalPixelSim::encodeRk212Timestep(MTL::ComputeCommandEncoder *encoder,
                                        double dt) {
  encodeEvaluateDcdt(encoder);
  encodeRk212Substep1(encoder, dt);
  encodeEvaluateDcdt(encoder);
  encodeRk212Substep2(encoder, dt);
}

void MetalPixelSim::doRK212(double dt) {
  ensureDownloadComplete();
  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
  auto *commandBuffer = impl->queue->commandBuffer();
  if (commandBuffer == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal command buffer");
  }
  auto *encoder = commandBuffer->computeCommandEncoder();
  if (encoder == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal compute encoder");
  }
  encodeRk212Timestep(encoder, dt);
  encoder->endEncoding();
  waitForCommandBuffer(commandBuffer, "Metal RK212 timestep failed");
  pool->release();
}

void MetalPixelSim::encodeRk323Timestep(MTL::ComputeCommandEncoder *encoder,
                                        double dt) {
  using namespace detail::rk323;
  encodeRkInit(encoder);
  encodeEvaluateDcdt(encoder);
  encodeRkSubstep(encoder, dt, g1[0], g2[0], g3[0], beta[0], delta[0]);
  for (std::size_t i = 1; i < g1.size(); ++i) {
    encodeEvaluateDcdt(encoder);
    encodeRkSubstep(encoder, dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
  }
  encodeRkFinalise(encoder, finaliseFactors[0], finaliseFactors[1],
                   finaliseFactors[2]);
}

void MetalPixelSim::doRK323(double dt) {
  ensureDownloadComplete();
  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
  auto *commandBuffer = impl->queue->commandBuffer();
  if (commandBuffer == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal command buffer");
  }
  auto *encoder = commandBuffer->computeCommandEncoder();
  if (encoder == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal compute encoder");
  }
  encodeRk323Timestep(encoder, dt);
  encoder->endEncoding();
  waitForCommandBuffer(commandBuffer, "Metal RK323 timestep failed");
  pool->release();
}

void MetalPixelSim::encodeRk435Timestep(MTL::ComputeCommandEncoder *encoder,
                                        double dt) {
  using namespace detail::rk435;
  encodeRkInit(encoder);
  encodeEvaluateDcdt(encoder);
  encodeRkSubstep(encoder, dt, g1[0], g2[0], g3[0], beta[0], delta[0]);
  for (std::size_t i = 1; i < g1.size(); ++i) {
    encodeEvaluateDcdt(encoder);
    encodeRkSubstep(encoder, dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
  }
  encodeRkFinalise(encoder, deltaSumReciprocal * delta[5], deltaSumReciprocal,
                   deltaSumReciprocal * delta[6]);
}

void MetalPixelSim::doRK435(double dt) {
  ensureDownloadComplete();
  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
  auto *commandBuffer = impl->queue->commandBuffer();
  if (commandBuffer == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal command buffer");
  }
  auto *encoder = commandBuffer->computeCommandEncoder();
  if (encoder == nullptr) {
    pool->release();
    throw MetalPixelSimError("Failed to create the Metal compute encoder");
  }
  encodeRk435Timestep(encoder, dt);
  encoder->endEncoding();
  waitForCommandBuffer(commandBuffer, "Metal RK435 timestep failed");
  pool->release();
}

double MetalPixelSim::doRKAdaptive(double dtMax) {
  PixelIntegratorError err{0.0, 0.0};
  double dt{};
  const double errPower = detail::getErrorPower(integrator);
  do {
    dt = std::min(nextTimestep, dtMax);
    ensureDownloadComplete();
    NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();
    auto *commandBuffer = impl->queue->commandBuffer();
    if (commandBuffer == nullptr) {
      pool->release();
      throw MetalPixelSimError("Failed to create the Metal command buffer");
    }
    auto *encoder = commandBuffer->computeCommandEncoder();
    if (encoder == nullptr) {
      pool->release();
      throw MetalPixelSimError("Failed to create the Metal compute encoder");
    }
    if (integrator == PixelIntegratorType::RK323) {
      encodeRk323Timestep(encoder, dt);
    } else if (integrator == PixelIntegratorType::RK435) {
      encodeRk435Timestep(encoder, dt);
    } else {
      encodeRk212Timestep(encoder, dt);
    }
    encodeRk212Error(encoder);
    // Clamp in the same command buffer after the error kernels. If the trial
    // step is rejected, restorePreStepConcentrations() overwrites conc.
    encodeClampNegative(encoder);
    encoder->endEncoding();
    waitForCommandBuffer(commandBuffer, "Metal adaptive RK timestep failed");
    pool->release();
    err = readRk212ErrorResult();
    double errFactor = std::min(errMax.abs / err.abs, errMax.rel / err.rel);
    errFactor = std::pow(errFactor, errPower);
    nextTimestep = std::min(0.95 * dt * errFactor, dtMax);
    SPDLOG_TRACE("Metal dt = {} gave rel err = {}, abs err = {} -> new dt = {}",
                 dt, err.rel, err.abs, nextTimestep);
    if (nextTimestep / dtMax < 1e-20) {
      currentErrorImages.clear();
      currentErrorMessage =
          fmt::format("Failed to solve Metal model to the required accuracy.");
      return nextTimestep;
    }
    if (err.abs > errMax.abs || err.rel > errMax.rel) {
      ++discardedSteps;
      restorePreStepConcentrations();
    }
  } while (err.abs > errMax.abs || err.rel > errMax.rel);
  return dt;
}

std::size_t
MetalPixelSim::run(double time, double timeout_ms,
                   const std::function<bool()> &stopRunningCallback) {
  if (!currentErrorMessage.empty()) {
    return 0;
  }
  currentErrorMessage.clear();
  currentErrorImages.clear();
  QElapsedTimer timer;
  timer.start();
  double tNow = 0.0;
  std::size_t steps = 0;
  discardedSteps = 0;
  constexpr double relativeTolerance = 1e-12;
  try {
    while (tNow + time * relativeTolerance < time) {
      const double maxDt = std::min(maxTimestep, time - tNow);
      if (integrator == PixelIntegratorType::RK101) {
        tNow += doRK101(maxDt);
      } else {
        tNow += doRKAdaptive(maxDt);
        if (!currentErrorMessage.empty()) {
          return steps;
        }
      }
      ++steps;
      if (timeout_ms >= 0.0 &&
          static_cast<double>(timer.elapsed()) >= timeout_ms) {
        setStopRequested(true);
      }
      if (stopRunningCallback && stopRunningCallback()) {
        setStopRequested(true);
      }
      if (stopRequested.load()) {
        currentErrorMessage = "Simulation stopped early";
        return steps;
      }
    }
    downloadStateToHost();
  } catch (const std::exception &e) {
    currentErrorMessage = e.what();
    SPDLOG_ERROR("Metal pixel backend run failed: {}", currentErrorMessage);
  }
  return steps;
}

const std::vector<double> &
MetalPixelSim::getConcentrations(std::size_t compartmentIndex) const {
  const_cast<MetalPixelSim *>(this)->waitForDownload();
  return impl->compartments[compartmentIndex].concHost;
}

std::size_t MetalPixelSim::getConcentrationPadding() const { return 0; }

const std::vector<double> &
MetalPixelSim::getDcdt(std::size_t compartmentIndex) const {
  auto &state = impl->compartments[compartmentIndex];
  copyBuffer(impl->queue, state.dDcdt, state.stagingFloat,
             bufferByteCount<float>(state.dcdtHost.size()),
             "Failed to stage Metal dcdt");
  downloadBufferToDoubles(state.dcdtHost, state.stagingFloat,
                          "Failed to download Metal dcdt");
  return state.dcdtHost;
}

double MetalPixelSim::getLowerOrderConcentration(std::size_t compartmentIndex,
                                                 std::size_t speciesIndex,
                                                 std::size_t pixelIndex) const {
  auto &state = impl->compartments[compartmentIndex];
  if (state.lowerOrderHost.size() != state.concHost.size()) {
    state.lowerOrderHost.resize(state.concHost.size());
  }
  copyBuffer(impl->queue, state.dLowerOrder, state.stagingFloat,
             bufferByteCount<float>(state.lowerOrderHost.size()),
             "Failed to stage Metal RK lower-order concentrations");
  downloadBufferToDoubles(state.lowerOrderHost, state.stagingFloat,
                          "Failed to download Metal RK lower-order "
                          "concentrations");
  if (state.lowerOrderHost.empty()) {
    return 0.0;
  }
  return state.lowerOrderHost[pixelIndex * state.nSpecies + speciesIndex];
}

} // namespace sme::simulate
