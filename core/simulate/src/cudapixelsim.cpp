#include "cudapixelsim.hpp"
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

#include "sme/cuda_stubs.hpp"
#include <cuda.h>
#include <nvrtc.h>

namespace sme::simulate {

namespace {

class CudaPixelSimError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

using detail::allMembraneFaceDirections;
using detail::calculateMaxStableTimestep;
using detail::getFaceFluxLength;
using detail::hasAnyCrossDiffusion;
using detail::invalidCompartmentIndex;

constexpr unsigned int cudaKernelBlockSize{128};

static std::string getCudaErrorMessage(CUresult result) {
  const char *name{nullptr};
  const char *msg{nullptr};
  cuGetErrorName(result, &name);
  cuGetErrorString(result, &msg);
  return fmt::format("{} ({})", name != nullptr ? name : "CUDA_ERROR_UNKNOWN",
                     msg != nullptr ? msg : "unknown CUDA error");
}

static void throwIfCudaFailed(CUresult result, const std::string &context) {
  if (result != CUDA_SUCCESS) {
    throw CudaPixelSimError(
        fmt::format("{}: {}", context, getCudaErrorMessage(result)));
  }
}

static void throwIfNvrtcFailed(nvrtcResult result, const std::string &context) {
  if (result != NVRTC_SUCCESS) {
    throw CudaPixelSimError(
        fmt::format("{}: {}", context, nvrtcGetErrorString(result)));
  }
}

class ScopedCudaContext {
public:
  explicit ScopedCudaContext(CUcontext context) : targetContext{context} {
    if (targetContext == nullptr) {
      return;
    }
    throwIfCudaFailed(cuCtxGetCurrent(&previousContext),
                      "Failed to query current CUDA context");
    if (previousContext != targetContext) {
      throwIfCudaFailed(cuCtxSetCurrent(targetContext),
                        "Failed to set current CUDA context");
      restorePreviousContext = true;
    }
  }

  ScopedCudaContext(const ScopedCudaContext &) = delete;
  ScopedCudaContext &operator=(const ScopedCudaContext &) = delete;

  ~ScopedCudaContext() {
    if (!restorePreviousContext) {
      return;
    }
    if (const auto result = cuCtxSetCurrent(previousContext);
        result != CUDA_SUCCESS) {
      SPDLOG_ERROR("Failed to restore CUDA context: {}",
                   getCudaErrorMessage(result));
    }
  }

private:
  CUcontext targetContext{};
  CUcontext previousContext{};
  bool restorePreviousContext{false};
};

} // namespace

struct CudaGeneratedExpressionBundle {
  std::vector<std::string> safeVariables;
  std::vector<std::string> cudaExpressions;
};

static CudaGeneratedExpressionBundle
makeCudaGeneratedExpressionBundle(const std::vector<std::string> &variables,
                                  const std::vector<std::string> &expressions,
                                  const std::string &expressionContext,
                                  bool useFloat = false) {
  CudaGeneratedExpressionBundle bundle;
  bundle.safeVariables.reserve(variables.size());
  bundle.cudaExpressions.reserve(expressions.size());
  for (std::size_t i = 0; i < variables.size(); ++i) {
    bundle.safeVariables.push_back(fmt::format("sme_var_{}", i));
  }

  common::Symbolic symbolic{expressions, variables};
  if (!symbolic.isValid()) {
    throw CudaPixelSimError(fmt::format("Failed to convert {} to CUDA code: {}",
                                        expressionContext,
                                        symbolic.getErrorMessage()));
  }
  symbolic.relabel(bundle.safeVariables);
  for (std::size_t i = 0; i < expressions.size(); ++i) {
    try {
      bundle.cudaExpressions.push_back(symbolic.cudaCode(i, useFloat));
    } catch (const std::exception &e) {
      throw CudaPixelSimError(
          fmt::format("Failed to convert {} '{}' to CUDA code: {}",
                      expressionContext, expressions[i], e.what()));
    }
  }

  return bundle;
}

std::string
detail::makeCudaKernelSource(const std::vector<std::string> &variables,
                             const std::vector<std::string> &expressions,
                             bool useFloat) {
  const auto bundle = makeCudaGeneratedExpressionBundle(
      variables, expressions, "reaction expression", useFloat);

  const char *T = useFloat ? "float" : "double";
  const char *suffix = useFloat ? "f" : "";

  std::ostringstream src;
  src << "extern \"C\" __device__ __forceinline__ void reaction_eval(" << T
      << "* sme_out, const " << T << "* sme_values) {\n";
  for (std::size_t i = 0; i < bundle.safeVariables.size(); ++i) {
    src << "  const " << T << " " << bundle.safeVariables[i] << " = sme_values["
        << i << "];\n";
  }
  for (std::size_t i = 0; i < bundle.cudaExpressions.size(); ++i) {
    src << "  sme_out[" << i << "] = " << bundle.cudaExpressions[i] << ";\n";
  }
  src << "}\n\n";
  src << "constexpr unsigned int N_SPECIES = " << bundle.safeVariables.size()
      << ";\n\n";
  src << "constexpr unsigned int RK_ERROR_BLOCK_SIZE = " << cudaKernelBlockSize
      << ";\n\n";
  src << fmt::format(R"(
extern "C" __global__ void reaction_kernel(const {0}* conc, {0}* dcdt,
                                           unsigned int nPixels) {{
  const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
  if (ix >= nPixels) {{
    return;
  }}
  reaction_eval(dcdt + ix * N_SPECIES, conc + ix * N_SPECIES);
}}

extern "C" __global__ void diffusion_uniform_kernel(
    const {0}* conc, {0}* dcdt, const unsigned int* nn,
    const {0}* diffusion, unsigned int nPixels) {{
  const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
  if (ix >= nPixels) {{
    return;
  }}
  const unsigned int iupx = nn[6 * ix];
  const unsigned int idnx = nn[6 * ix + 1];
  const unsigned int iupy = nn[6 * ix + 2];
  const unsigned int idny = nn[6 * ix + 3];
  const unsigned int iupz = nn[6 * ix + 4];
  const unsigned int idnz = nn[6 * ix + 5];
  const unsigned int iCenter = ix * N_SPECIES;
  const unsigned int iUpx = iupx * N_SPECIES;
  const unsigned int iDnx = idnx * N_SPECIES;
  const unsigned int iUpy = iupy * N_SPECIES;
  const unsigned int iDny = idny * N_SPECIES;
  const unsigned int iUpz = iupz * N_SPECIES;
  const unsigned int iDnz = idnz * N_SPECIES;
  for (unsigned int is = 0; is < N_SPECIES; ++is) {{
    const {0} c0 = conc[iCenter + is];
    dcdt[iCenter + is] +=
        diffusion[3 * is] *
            (conc[iUpx + is] + conc[iDnx + is] - 2.0{1} * c0) +
        diffusion[3 * is + 1] *
            (conc[iUpy + is] + conc[iDny + is] - 2.0{1} * c0) +
        diffusion[3 * is + 2] *
            (conc[iUpz + is] + conc[iDnz + is] - 2.0{1} * c0);
  }}
}}

extern "C" __global__ void rk101_update_kernel({0}* conc, const {0}* dcdt,
                                                {0} dt,
                                                unsigned int nValues) {{
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nValues) {{
    return;
  }}
  conc[i] += dt * dcdt[i];
}}

extern "C" __global__ void rk212_substep1_kernel(
    {0}* conc, const {0}* dcdt, {0}* sme_lower_order,
    {0}* sme_old_conc, {0} dt, unsigned int nValues) {{
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nValues) {{
    return;
  }}
  sme_old_conc[i] = conc[i];
  conc[i] += dt * dcdt[i];
  (void)sme_lower_order;
}}

extern "C" __global__ void rk212_substep2_kernel(
    {0}* conc, const {0}* dcdt, {0}* sme_lower_order,
    const {0}* sme_old_conc, {0} dt, unsigned int nValues) {{
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nValues) {{
    return;
  }}
  sme_lower_order[i] = conc[i];
  conc[i] = 0.5{1} * sme_old_conc[i] + 0.5{1} * conc[i] + 0.5{1} * dt * dcdt[i];
}}

extern "C" __global__ void rk212_error_kernel(
    const {0}* conc, const {0}* sme_lower_order,
    const {0}* sme_old_conc, {0} epsilon, {0}* abs_error_blocks,
    {0}* rel_error_blocks, unsigned int nValues) {{
  __shared__ {0} abs_error_shared[RK_ERROR_BLOCK_SIZE];
  __shared__ {0} rel_error_shared[RK_ERROR_BLOCK_SIZE];
  const unsigned int localIndex = threadIdx.x;
  const unsigned int globalIndex = blockIdx.x * blockDim.x + localIndex;
  {0} localAbsError = 0.0{1};
  {0} localRelError = 0.0{1};
  if (globalIndex < nValues) {{
    localAbsError = fabs(conc[globalIndex] - sme_lower_order[globalIndex]);
    const {0} localNorm =
        0.5{1} * (conc[globalIndex] + sme_old_conc[globalIndex] + epsilon);
    localRelError = localAbsError / localNorm;
  }}
  abs_error_shared[localIndex] = localAbsError;
  rel_error_shared[localIndex] = localRelError;
  __syncthreads();
  for (unsigned int stride = blockDim.x / 2; stride > 0; stride /= 2) {{
    if (localIndex < stride) {{
      abs_error_shared[localIndex] =
          fmax(abs_error_shared[localIndex],
               abs_error_shared[localIndex + stride]);
      rel_error_shared[localIndex] =
          fmax(rel_error_shared[localIndex],
               rel_error_shared[localIndex + stride]);
    }}
    __syncthreads();
  }}
  if (localIndex == 0) {{
    abs_error_blocks[blockIdx.x] = abs_error_shared[0];
    rel_error_blocks[blockIdx.x] = rel_error_shared[0];
  }}
}}

extern "C" __global__ void rk212_error_reduce_kernel(
    const {0}* abs_error_blocks, const {0}* rel_error_blocks,
    {0}* result, unsigned int nBlocks) {{
  __shared__ {0} abs_shared[RK_ERROR_BLOCK_SIZE];
  __shared__ {0} rel_shared[RK_ERROR_BLOCK_SIZE];
  const unsigned int tid = threadIdx.x;
  {0} localAbs = 0.0{1};
  {0} localRel = 0.0{1};
  for (unsigned int i = tid; i < nBlocks; i += blockDim.x) {{
    localAbs = fmax(localAbs, abs_error_blocks[i]);
    localRel = fmax(localRel, rel_error_blocks[i]);
  }}
  abs_shared[tid] = localAbs;
  rel_shared[tid] = localRel;
  __syncthreads();
  for (unsigned int stride = blockDim.x / 2; stride > 0; stride /= 2) {{
    if (tid < stride) {{
      abs_shared[tid] = fmax(abs_shared[tid], abs_shared[tid + stride]);
      rel_shared[tid] = fmax(rel_shared[tid], rel_shared[tid + stride]);
    }}
    __syncthreads();
  }}
  if (tid == 0) {{
    result[0] = abs_shared[0];
    result[1] = rel_shared[0];
  }}
}}

extern "C" __global__ void rk_init_kernel(
    const {0}* conc, {0}* s2, {0}* s3, unsigned int nValues) {{
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nValues) {{
    return;
  }}
  s3[i] = conc[i];
  s2[i] = 0.0{1};
}}

extern "C" __global__ void rk_substep_kernel(
    {0}* conc, const {0}* dcdt, {0}* s2, const {0}* s3,
    {0} dt, {0} g1, {0} g2, {0} g3, {0} beta, {0} delta,
    unsigned int nValues) {{
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nValues) {{
    return;
  }}
  s2[i] += delta * conc[i];
  conc[i] = g1 * conc[i] + g2 * s2[i] + g3 * s3[i] + beta * dt * dcdt[i];
}}

extern "C" __global__ void rk_finalise_kernel(
    const {0}* conc, {0}* s2, const {0}* s3,
    {0} cFactor, {0} s2Factor, {0} s3Factor,
    unsigned int nValues) {{
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nValues) {{
    return;
  }}
  s2[i] = cFactor * conc[i] + s2Factor * s2[i] + s3Factor * s3[i];
}}

extern "C" __global__ void clamp_negative_kernel({0}* conc,
                                                 unsigned int nPixels) {{
  const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
  if (ix >= nPixels) {{
    return;
  }}
  const unsigned int offset = ix * N_SPECIES;
  for (unsigned int is = 0; is < N_SPECIES; ++is) {{
    if (conc[offset + is] < 0.0{1}) {{
      conc[offset + is] = 0.0{1};
    }}
  }}
}}
)",
                     T, suffix);
  return src.str();
}

std::string
detail::makeNvrtcCompileFailureMessage(std::string_view context,
                                       std::string_view error,
                                       std::string_view compileLog) {
  std::string message{fmt::format("{}: {}", context, error)};
  std::string log{compileLog};
  while (!log.empty() &&
         (log.back() == '\0' || log.back() == '\n' || log.back() == '\r')) {
    log.pop_back();
  }
  if (!log.empty()) {
    message += "\nNVRTC compile log:\n";
    message += log;
  }
  return message;
}

static std::vector<char> compileNvrtcProgramToCubin(nvrtcProgram program,
                                                    int computeCapabilityMajor,
                                                    int computeCapabilityMinor,
                                                    std::string_view context);

static std::string
makeCudaMembraneKernelSource(const std::vector<std::string> &variables,
                             const std::vector<std::string> &expressions,
                             unsigned int nSpeciesA, unsigned int nSpeciesB,
                             bool useFloat = false) {
  const auto bundle = makeCudaGeneratedExpressionBundle(
      variables, expressions, "membrane reaction expression", useFloat);

  const char *T = useFloat ? "float" : "double";

  std::ostringstream src;
  src << "extern \"C\" __device__ __forceinline__ void membrane_reaction_eval("
      << T << "* sme_out, const " << T << "* sme_values) {\n";
  for (std::size_t i = 0; i < bundle.safeVariables.size(); ++i) {
    src << "  const " << T << " " << bundle.safeVariables[i] << " = sme_values["
        << i << "];\n";
  }
  for (std::size_t i = 0; i < bundle.cudaExpressions.size(); ++i) {
    src << "  sme_out[" << i << "] = " << bundle.cudaExpressions[i] << ";\n";
  }
  src << "}\n\n";
  src << "constexpr unsigned int N_MEMBRANE_INPUTS = "
      << bundle.safeVariables.size() << ";\n";
  src << "constexpr unsigned int N_SPECIES_A = " << nSpeciesA << ";\n";
  src << "constexpr unsigned int N_SPECIES_B = " << nSpeciesB << ";\n\n";
  src << fmt::format(R"(
extern "C" __global__ void membrane_reaction_kernel(
    const unsigned int* indexPairs, unsigned int nPairs, const {0}* concA,
    {0}* dcdtA, const {0}* concB, {0}* dcdtB,
    {0} invFluxLength) {{
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nPairs) {{
    return;
  }}
  const unsigned int ixA = indexPairs[2 * i];
  const unsigned int ixB = indexPairs[2 * i + 1];
  {0} sme_inputs[N_MEMBRANE_INPUTS]{{}};
  {0} sme_result[N_MEMBRANE_INPUTS]{{}};
  const unsigned int offsetA = ixA * N_SPECIES_A;
  const unsigned int offsetB = ixB * N_SPECIES_B;
  for (unsigned int is = 0; is < N_SPECIES_A; ++is) {{
    sme_inputs[is] = concA[offsetA + is];
  }}
  for (unsigned int is = 0; is < N_SPECIES_B; ++is) {{
    sme_inputs[N_SPECIES_A + is] = concB[offsetB + is];
  }}
  membrane_reaction_eval(sme_result, sme_inputs);
  for (unsigned int is = 0; is < N_SPECIES_A; ++is) {{
    atomicAdd(&dcdtA[offsetA + is], sme_result[is] * invFluxLength);
  }}
  for (unsigned int is = 0; is < N_SPECIES_B; ++is) {{
    atomicAdd(&dcdtB[offsetB + is], sme_result[N_SPECIES_A + is] * invFluxLength);
  }}
}}
)",
                     T);
  return src.str();
}

struct CudaKernelBundle {
  CUmodule module{};
  CUfunction reaction{};
  CUfunction diffusionUniform{};
  CUfunction rk101Update{};
  CUfunction rk212Substep1{};
  CUfunction rk212Substep2{};
  CUfunction rk212Error{};
  CUfunction rk212ErrorReduce{};
  CUfunction rkInit{};
  CUfunction rkSubstep{};
  CUfunction rkFinalise{};
  CUfunction clampNegative{};
};

struct CudaMembraneKernelBundle {
  CUmodule module{};
  CUfunction reaction{};
};

struct CudaPixelSim::Impl {
  struct CompartmentState {
    std::string compartmentId{};
    std::size_t nPixels{};
    std::size_t nSpecies{};
    std::vector<double> concHost{};
    std::vector<float> concFloatHost{};
    std::vector<double> dcdtHost{};
    std::vector<double> lowerOrderHost{};
    std::vector<std::uint32_t> nnHost{};
    std::vector<double> diffusionHost{};
    std::size_t nErrorBlocks{};
    double maxStableTimestep{std::numeric_limits<double>::max()};
    CudaKernelBundle kernels{};
    CUdeviceptr dConc{};
    CUdeviceptr dDcdt{};
    CUdeviceptr dLowerOrder{};
    CUdeviceptr dOldConc{};
    CUdeviceptr dNn{};
    CUdeviceptr dDiffusion{};
    CUdeviceptr dErrorAbs{};
    CUdeviceptr dErrorRel{};
    CUdeviceptr dErrorResult{};
    CUstream stream{};
    CUevent dcdtReady{};
  };

  struct MembraneState {
    std::string membraneId{};
    std::size_t compartmentIndexA{invalidCompartmentIndex};
    std::size_t compartmentIndexB{invalidCompartmentIndex};
    std::size_t nSpeciesA{};
    std::size_t nSpeciesB{};
    std::array<std::uint32_t, 6> nFacePairs{};
    std::array<double, 6> faceInvFluxLengths{};
    CudaMembraneKernelBundle kernels{};
    std::array<CUdeviceptr, 6> dFaceIndexPairs{};
  };

  // Precomputed membrane kernel launch assignment for parallel execution.
  // Each entry represents one non-empty face-direction kernel for one membrane,
  // assigned round-robin to a compartment stream.
  struct MembraneLaunch {
    std::size_t membraneIndex{};
    std::size_t faceIndex{};
    std::size_t streamIndex{};
  };

  std::vector<CompartmentState> compartments{};
  std::vector<MembraneState> membranes{};
  std::vector<MembraneLaunch> membraneLaunches{};
  // Which compartment dcdtReady events each stream must wait for before
  // running its assigned membrane kernels (indexed by compartment/stream index)
  std::vector<std::vector<std::size_t>> membraneStreamDeps{};
  bool useFloat{false};
  std::size_t gpuElementSize{sizeof(double)};
  std::vector<float> floatConversionBuffer{};
  bool downloadPending{false};
  CUdevice device{};
  CUcontext context{};
  CUstream stream{};
  CUstream transferStream{};
  CUevent downloadComplete{};
  CUevent mainStreamReady{};

  ~Impl() {
    if (context != nullptr) {
      if (const auto result = cuCtxSetCurrent(context);
          result != CUDA_SUCCESS) {
        SPDLOG_ERROR("Failed to set current CUDA context for cleanup: {}",
                     getCudaErrorMessage(result));
        return;
      }
    }
    for (auto &compartment : compartments) {
      if (compartment.kernels.module != nullptr) {
        cuModuleUnload(compartment.kernels.module);
      }
      if (compartment.dConc != 0) {
        cuMemFree(compartment.dConc);
      }
      if (compartment.dDcdt != 0) {
        cuMemFree(compartment.dDcdt);
      }
      if (compartment.dLowerOrder != 0) {
        cuMemFree(compartment.dLowerOrder);
      }
      if (compartment.dOldConc != 0) {
        cuMemFree(compartment.dOldConc);
      }
      if (compartment.dNn != 0) {
        cuMemFree(compartment.dNn);
      }
      if (compartment.dDiffusion != 0) {
        cuMemFree(compartment.dDiffusion);
      }
      if (compartment.dErrorAbs != 0) {
        cuMemFree(compartment.dErrorAbs);
      }
      if (compartment.dErrorRel != 0) {
        cuMemFree(compartment.dErrorRel);
      }
      if (compartment.dErrorResult != 0) {
        cuMemFree(compartment.dErrorResult);
      }
      if (compartment.dcdtReady != nullptr) {
        cuEventDestroy(compartment.dcdtReady);
      }
      if (compartment.stream != nullptr) {
        cuStreamDestroy(compartment.stream);
      }
    }
    for (auto &membrane : membranes) {
      if (membrane.kernels.module != nullptr) {
        cuModuleUnload(membrane.kernels.module);
      }
      for (auto &dFaceIndexPairs : membrane.dFaceIndexPairs) {
        if (dFaceIndexPairs != 0) {
          cuMemFree(dFaceIndexPairs);
        }
      }
    }
    if (mainStreamReady != nullptr) {
      cuEventDestroy(mainStreamReady);
    }
    if (downloadComplete != nullptr) {
      cuEventDestroy(downloadComplete);
    }
    if (transferStream != nullptr) {
      cuStreamDestroy(transferStream);
    }
    if (stream != nullptr) {
      cuStreamDestroy(stream);
    }
    if (context != nullptr) {
      cuCtxDestroy(context);
    }
  }
};

static CudaKernelBundle compileCudaKernelBundle(const ReacExpr &reacExpr,
                                                int computeCapabilityMajor,
                                                int computeCapabilityMinor,
                                                bool useFloat = false) {
  auto source = detail::makeCudaKernelSource(reacExpr.variables,
                                             reacExpr.expressions, useFloat);
  nvrtcProgram program{};
  throwIfNvrtcFailed(nvrtcCreateProgram(&program, source.c_str(),
                                        "sme_pixel_poc.cu", 0, nullptr,
                                        nullptr),
                     "Failed to create NVRTC program");
  try {
    const auto cubin =
        compileNvrtcProgramToCubin(program, computeCapabilityMajor,
                                   computeCapabilityMinor, "CUDA pixel kernel");

    CudaKernelBundle bundle;
    throwIfCudaFailed(
        cuModuleLoadDataEx(&bundle.module, cubin.data(), 0, nullptr, nullptr),
        "Failed to load CUDA module");
    throwIfCudaFailed(
        cuModuleGetFunction(&bundle.reaction, bundle.module, "reaction_kernel"),
        "Failed to load reaction kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.diffusionUniform,
                                          bundle.module,
                                          "diffusion_uniform_kernel"),
                      "Failed to load diffusion kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.rk101Update, bundle.module,
                                          "rk101_update_kernel"),
                      "Failed to load RK101 update kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.rk212Substep1, bundle.module,
                                          "rk212_substep1_kernel"),
                      "Failed to load RK212 substep 1 kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.rk212Substep2, bundle.module,
                                          "rk212_substep2_kernel"),
                      "Failed to load RK212 substep 2 kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.rk212Error, bundle.module,
                                          "rk212_error_kernel"),
                      "Failed to load RK212 error kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.rk212ErrorReduce,
                                          bundle.module,
                                          "rk212_error_reduce_kernel"),
                      "Failed to load RK212 error reduce kernel");
    throwIfCudaFailed(
        cuModuleGetFunction(&bundle.rkInit, bundle.module, "rk_init_kernel"),
        "Failed to load RK init kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.rkSubstep, bundle.module,
                                          "rk_substep_kernel"),
                      "Failed to load RK substep kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.rkFinalise, bundle.module,
                                          "rk_finalise_kernel"),
                      "Failed to load RK finalise kernel");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.clampNegative, bundle.module,
                                          "clamp_negative_kernel"),
                      "Failed to load concentration clamp kernel");
    if (program != nullptr) {
      nvrtcDestroyProgram(&program);
    }
    return bundle;
  } catch (...) {
    if (program != nullptr) {
      nvrtcDestroyProgram(&program);
    }
    throw;
  }
}

static CudaMembraneKernelBundle compileCudaMembraneKernelBundle(
    const ReacExpr &reacExpr, unsigned int nSpeciesA, unsigned int nSpeciesB,
    int computeCapabilityMajor, int computeCapabilityMinor,
    bool useFloat = false) {
  auto source = makeCudaMembraneKernelSource(
      reacExpr.variables, reacExpr.expressions, nSpeciesA, nSpeciesB, useFloat);
  nvrtcProgram program{};
  throwIfNvrtcFailed(nvrtcCreateProgram(&program, source.c_str(),
                                        "sme_pixel_membrane_poc.cu", 0, nullptr,
                                        nullptr),
                     "Failed to create NVRTC membrane program");
  try {
    const auto cubin = compileNvrtcProgramToCubin(
        program, computeCapabilityMajor, computeCapabilityMinor,
        "CUDA membrane pixel kernel");

    CudaMembraneKernelBundle bundle;
    throwIfCudaFailed(
        cuModuleLoadDataEx(&bundle.module, cubin.data(), 0, nullptr, nullptr),
        "Failed to load CUDA membrane module");
    throwIfCudaFailed(cuModuleGetFunction(&bundle.reaction, bundle.module,
                                          "membrane_reaction_kernel"),
                      "Failed to load membrane reaction kernel");
    if (program != nullptr) {
      nvrtcDestroyProgram(&program);
    }
    return bundle;
  } catch (...) {
    if (program != nullptr) {
      nvrtcDestroyProgram(&program);
    }
    throw;
  }
}

static void launchKernel(CUfunction kernel, unsigned int gridX,
                         unsigned int blockX, CUstream stream, void **args,
                         const std::string &name) {
  throwIfCudaFailed(cuLaunchKernel(kernel, gridX, 1, 1, blockX, 1, 1, 0, stream,
                                   args, nullptr),
                    fmt::format("Failed to launch CUDA kernel '{}'", name));
}

static void uploadDoublesToGpu(CUdeviceptr dst, const std::vector<double> &src,
                               bool useFloat, const std::string &context) {
  if (useFloat) {
    std::vector<float> tmp(src.begin(), src.end());
    throwIfCudaFailed(cuMemcpyHtoD(dst, tmp.data(), tmp.size() * sizeof(float)),
                      context);
  } else {
    throwIfCudaFailed(
        cuMemcpyHtoD(dst, src.data(), src.size() * sizeof(double)), context);
  }
}

static void downloadGpuToDoubles(std::vector<double> &dst, CUdeviceptr src,
                                 bool useFloat, std::vector<float> &floatBuffer,
                                 const std::string &context) {
  if (useFloat) {
    floatBuffer.resize(dst.size());
    throwIfCudaFailed(
        cuMemcpyDtoH(floatBuffer.data(), src, dst.size() * sizeof(float)),
        context);
    std::copy(floatBuffer.begin(), floatBuffer.end(), dst.begin());
  } else {
    throwIfCudaFailed(
        cuMemcpyDtoH(dst.data(), src, dst.size() * sizeof(double)), context);
  }
}

static std::vector<char> compileNvrtcProgramToCubin(nvrtcProgram program,
                                                    int computeCapabilityMajor,
                                                    int computeCapabilityMinor,
                                                    std::string_view context) {
  std::string arch =
      fmt::format("--gpu-architecture=sm_{}{}", computeCapabilityMajor,
                  computeCapabilityMinor);
  std::array<const char *, 2> options{"--std=c++17", arch.c_str()};
  const auto compileResult = nvrtcCompileProgram(
      program, static_cast<int>(options.size()), options.data());
  std::size_t logSize{};
  throwIfNvrtcFailed(
      nvrtcGetProgramLogSize(program, &logSize),
      fmt::format("Failed to get NVRTC log size for {}", context));
  std::string compileLog(logSize, '\0');
  if (logSize > 1) {
    throwIfNvrtcFailed(
        nvrtcGetProgramLog(program, compileLog.data()),
        fmt::format("Failed to get NVRTC compile log for {}", context));
    SPDLOG_DEBUG("NVRTC {} compile log:\n{}", context, compileLog);
  }
  if (compileResult != NVRTC_SUCCESS) {
    throw CudaPixelSimError(detail::makeNvrtcCompileFailureMessage(
        fmt::format("Failed to compile {}", context),
        nvrtcGetErrorString(compileResult), compileLog));
  }

  std::size_t cubinSize{};
  throwIfNvrtcFailed(
      nvrtcGetCUBINSize(program, &cubinSize),
      fmt::format("Failed to get NVRTC cubin size for {}", context));
  std::vector<char> cubin(cubinSize);
  throwIfNvrtcFailed(
      nvrtcGetCUBIN(program, cubin.data()),
      fmt::format("Failed to extract NVRTC cubin for {}", context));
  return cubin;
}

CudaPixelSim::CudaPixelSim(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    const std::map<std::string, double, std::less<>> &substitutions)
    : PixelSimBase{sbmlDoc.getSimulationSettings().options.pixel.integrator,
                   sbmlDoc.getSimulationSettings().options.pixel.maxErr,
                   sbmlDoc.getSimulationSettings().options.pixel.maxTimestep},
      impl(std::make_unique<Impl>()), doc{sbmlDoc} {
  CUcontext previousContext{};
  bool restorePreviousContext{false};
  try {
    if (sbmlDoc.getSimulationSettings().options.pixel.backend !=
        PixelBackendType::GPU) {
      throw CudaPixelSimError("CUDA pixel backend was not selected");
    }
    impl->useFloat =
        sbmlDoc.getSimulationSettings().options.pixel.gpuFloatPrecision ==
        GpuFloatPrecision::Float;
    impl->gpuElementSize = impl->useFloat ? sizeof(float) : sizeof(double);
    if (hasAnyCrossDiffusion(doc, compartmentSpeciesIds)) {
      throw CudaPixelSimError(
          "CUDA pixel backend PoC does not yet support cross-diffusion");
    }
    const auto xId{doc.getParameters().getSpatialCoordinates().x.id};
    const auto yId{doc.getParameters().getSpatialCoordinates().y.id};
    const auto zId{doc.getParameters().getSpatialCoordinates().z.id};
    if (doc.getReactions().dependOnVariable("time")) {
      throw CudaPixelSimError(
          "CUDA pixel backend PoC does not yet support time-dependent "
          "reaction terms");
    }
    if (doc.getReactions().dependOnVariable(xId.c_str()) ||
        doc.getReactions().dependOnVariable(yId.c_str()) ||
        doc.getReactions().dependOnVariable(zId.c_str())) {
      throw CudaPixelSimError(
          "CUDA pixel backend PoC does not yet support spatially dependent "
          "reaction terms");
    }
    detail::validatePocCompartmentInputs<CudaPixelSimError>(
        doc, compartmentIds, compartmentSpeciesIds, "CUDA");
    if (!isCudaAvailable()) {
      throw CudaPixelSimError(
          fmt::format("CUDA is not available: {}", cudaUnavailableReason()));
    }
    throwIfCudaFailed(cuInit(0), "Failed to initialize the CUDA driver");
    throwIfCudaFailed(cuCtxGetCurrent(&previousContext),
                      "Failed to query current CUDA context");
    int nDevices{};
    throwIfCudaFailed(cuDeviceGetCount(&nDevices),
                      "Failed to query CUDA devices");
    if (nDevices < 1) {
      throw CudaPixelSimError("No CUDA devices were found");
    }
    throwIfCudaFailed(cuDeviceGet(&impl->device, 0),
                      "Failed to get the primary CUDA device");
    throwIfCudaFailed(cuCtxCreate(&impl->context, nullptr, 0, impl->device),
                      "Failed to create the CUDA context");
    restorePreviousContext = true;
    throwIfCudaFailed(cuStreamCreate(&impl->stream, CU_STREAM_DEFAULT),
                      "Failed to create the CUDA compute stream");
    throwIfCudaFailed(cuStreamCreate(&impl->transferStream, CU_STREAM_DEFAULT),
                      "Failed to create the CUDA transfer stream");
    throwIfCudaFailed(
        cuEventCreate(&impl->downloadComplete, CU_EVENT_DISABLE_TIMING),
        "Failed to create the CUDA download event");
    // Record event immediately so the first ensureDownloadComplete() is a no-op
    throwIfCudaFailed(cuEventRecord(impl->downloadComplete, impl->stream),
                      "Failed to initialize download event");
    throwIfCudaFailed(
        cuEventCreate(&impl->mainStreamReady, CU_EVENT_DISABLE_TIMING),
        "Failed to create the CUDA main-stream-ready event");
    // Record event so first evaluateDcdt() compartment streams don't wait
    throwIfCudaFailed(cuEventRecord(impl->mainStreamReady, impl->stream),
                      "Failed to initialize main-stream-ready event");
    int computeCapabilityMajor{};
    int computeCapabilityMinor{};
    throwIfCudaFailed(
        cuDeviceGetAttribute(&computeCapabilityMajor,
                             CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR,
                             impl->device),
        "Failed to query CUDA compute capability major version");
    throwIfCudaFailed(
        cuDeviceGetAttribute(&computeCapabilityMinor,
                             CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR,
                             impl->device),
        "Failed to query CUDA compute capability minor version");

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
      state.nErrorBlocks = (state.concHost.size() + cudaKernelBlockSize - 1) /
                           cudaKernelBlockSize;
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
      for (std::size_t is = 0; is < speciesIds.size(); ++is) {
        const auto &speciesId{speciesIds[is]};
        const auto *field = doc.getSpecies().getField(speciesId.c_str());
        if (field == nullptr) {
          throw CudaPixelSimError(fmt::format(
              "CUDA pixel backend PoC could not find field for species '{}'",
              speciesId));
        }
        if (!field->getIsSpatial()) {
          throw CudaPixelSimError(
              "CUDA pixel backend PoC does not yet support non-spatial "
              "species");
        }
        if (!field->getIsUniformDiffusionConstant()) {
          throw CudaPixelSimError(
              "CUDA pixel backend PoC does not yet support non-uniform "
              "diffusion constants");
        }
        if (doc.getSpecies().getStorage(speciesId.c_str()) != 1.0) {
          throw CudaPixelSimError(
              "CUDA pixel backend PoC currently requires unit storage for all "
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
      state.kernels =
          compileCudaKernelBundle(reacExpr, computeCapabilityMajor,
                                  computeCapabilityMinor, impl->useFloat);

      const auto eSize = impl->gpuElementSize;
      throwIfCudaFailed(cuMemAlloc(&state.dConc, state.concHost.size() * eSize),
                        "Failed to allocate CUDA concentration buffer");
      throwIfCudaFailed(cuMemAlloc(&state.dDcdt, state.dcdtHost.size() * eSize),
                        "Failed to allocate CUDA dcdt buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dLowerOrder, state.concHost.size() * eSize),
          "Failed to allocate CUDA RK lower-order buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dOldConc, state.concHost.size() * eSize),
          "Failed to allocate CUDA RK old concentration buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dNn,
                     state.nnHost.size() * sizeof(state.nnHost.front())),
          "Failed to allocate CUDA neighbor buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dDiffusion, state.diffusionHost.size() * eSize),
          "Failed to allocate CUDA diffusion buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dErrorAbs, state.nErrorBlocks * eSize),
          "Failed to allocate CUDA RK absolute error buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dErrorRel, state.nErrorBlocks * eSize),
          "Failed to allocate CUDA RK relative error buffer");
      throwIfCudaFailed(cuMemAlloc(&state.dErrorResult, 2 * eSize),
                        "Failed to allocate CUDA RK error result buffer");

      uploadDoublesToGpu(state.dConc, state.concHost, impl->useFloat,
                         "Failed to upload initial CUDA concentrations");
      throwIfCudaFailed(
          cuMemsetD8(state.dDcdt, 0, state.dcdtHost.size() * eSize),
          "Failed to clear CUDA dcdt buffer");
      throwIfCudaFailed(
          cuMemsetD8(state.dLowerOrder, 0, state.concHost.size() * eSize),
          "Failed to clear CUDA RK lower-order buffer");
      throwIfCudaFailed(
          cuMemcpyDtoD(state.dOldConc, state.dConc,
                       state.concHost.size() * eSize),
          "Failed to initialize CUDA RK old concentration buffer");
      throwIfCudaFailed(
          cuMemcpyHtoD(state.dNn, state.nnHost.data(),
                       state.nnHost.size() * sizeof(state.nnHost.front())),
          "Failed to upload CUDA neighbor topology");
      uploadDoublesToGpu(state.dDiffusion, state.diffusionHost, impl->useFloat,
                         "Failed to upload CUDA diffusion coefficients");
      throwIfCudaFailed(
          cuMemsetD8(state.dErrorAbs, 0, state.nErrorBlocks * eSize),
          "Failed to clear CUDA RK absolute error buffer");
      throwIfCudaFailed(
          cuMemsetD8(state.dErrorRel, 0, state.nErrorBlocks * eSize),
          "Failed to clear CUDA RK relative error buffer");
      throwIfCudaFailed(cuMemsetD8(state.dErrorResult, 0, 2 * eSize),
                        "Failed to clear CUDA RK error result buffer");
      throwIfCudaFailed(cuStreamCreate(&state.stream, CU_STREAM_NON_BLOCKING),
                        "Failed to create per-compartment CUDA stream");
      throwIfCudaFailed(
          cuEventCreate(&state.dcdtReady, CU_EVENT_DISABLE_TIMING),
          "Failed to create per-compartment CUDA event");
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
        state.kernels = compileCudaMembraneKernelBundle(
            reacExpr, static_cast<unsigned int>(state.nSpeciesA),
            static_cast<unsigned int>(state.nSpeciesB), computeCapabilityMajor,
            computeCapabilityMinor, impl->useFloat);

        for (const auto faceDirection : allMembraneFaceDirections) {
          const auto faceIndex = static_cast<std::size_t>(faceDirection);
          const auto &facePairs{membrane.getFaceIndexPairs(faceDirection)};
          state.nFacePairs[faceIndex] =
              static_cast<std::uint32_t>(facePairs.size());
          if (facePairs.empty()) {
            continue;
          }
          state.faceInvFluxLengths[faceIndex] =
              1.0 / getFaceFluxLength(faceDirection, voxelSize);
          std::vector<std::uint32_t> flatPairs;
          flatPairs.reserve(2 * facePairs.size());
          for (const auto &[ixA, ixB] : facePairs) {
            flatPairs.push_back(static_cast<std::uint32_t>(ixA));
            flatPairs.push_back(static_cast<std::uint32_t>(ixB));
          }
          throwIfCudaFailed(
              cuMemAlloc(&state.dFaceIndexPairs[faceIndex],
                         flatPairs.size() * sizeof(flatPairs.front())),
              fmt::format("Failed to allocate CUDA membrane pair buffer for "
                          "'{}'",
                          state.membraneId));
          throwIfCudaFailed(
              cuMemcpyHtoD(state.dFaceIndexPairs[faceIndex], flatPairs.data(),
                           flatPairs.size() * sizeof(flatPairs.front())),
              fmt::format("Failed to upload CUDA membrane topology for '{}'",
                          state.membraneId));
        }
        impl->membranes.push_back(std::move(state));
      }
    }

    // Precompute round-robin membrane kernel launch assignments
    {
      const auto nStreams = impl->compartments.size();
      impl->membraneStreamDeps.resize(nStreams);
      std::size_t nextStream = 0;
      for (std::size_t mi = 0; mi < impl->membranes.size(); ++mi) {
        const auto &membrane = impl->membranes[mi];
        for (std::size_t fi = 0; fi < 6; ++fi) {
          if (membrane.nFacePairs[fi] == 0) {
            continue;
          }
          auto streamIdx = nextStream % nStreams;
          nextStream++;
          impl->membraneLaunches.push_back({mi, fi, streamIdx});
          if (membrane.compartmentIndexA != invalidCompartmentIndex) {
            auto &deps = impl->membraneStreamDeps[streamIdx];
            if (std::find(deps.begin(), deps.end(),
                          membrane.compartmentIndexA) == deps.end()) {
              deps.push_back(membrane.compartmentIndexA);
            }
          }
          if (membrane.compartmentIndexB != invalidCompartmentIndex) {
            auto &deps = impl->membraneStreamDeps[streamIdx];
            if (std::find(deps.begin(), deps.end(),
                          membrane.compartmentIndexB) == deps.end()) {
              deps.push_back(membrane.compartmentIndexB);
            }
          }
        }
      }
    }

    const auto &data{sbmlDoc.getSimulationData()};
    if (data.concentration.size() > 1 && !data.concentration.back().empty() &&
        data.concentration.back().size() == impl->compartments.size()) {
      SPDLOG_INFO("Applying supplied initial concentrations to CUDA backend");
      for (std::size_t i = 0; i < impl->compartments.size(); ++i) {
        auto &state = impl->compartments[i];
        if (data.concentration.back()[i].size() == state.concHost.size()) {
          state.concHost = data.concentration.back()[i];
          uploadDoublesToGpu(state.dConc, state.concHost, impl->useFloat,
                             "Failed to upload supplied CUDA concentrations");
          const auto eSize = impl->gpuElementSize;
          throwIfCudaFailed(
              cuMemsetD8(state.dLowerOrder, 0, state.concHost.size() * eSize),
              "Failed to clear CUDA RK lower-order buffer");
          throwIfCudaFailed(
              cuMemcpyDtoD(state.dOldConc, state.dConc,
                           state.concHost.size() * eSize),
              "Failed to update CUDA RK old concentration buffer");
        }
      }
    }
    if (restorePreviousContext && previousContext != impl->context) {
      throwIfCudaFailed(cuCtxSetCurrent(previousContext),
                        "Failed to restore previous CUDA context");
      restorePreviousContext = false;
    }
  } catch (const std::exception &e) {
    if (restorePreviousContext && previousContext != impl->context) {
      if (const auto result = cuCtxSetCurrent(previousContext);
          result != CUDA_SUCCESS) {
        SPDLOG_ERROR("Failed to restore previous CUDA context after setup "
                     "failure: {}",
                     getCudaErrorMessage(result));
      }
    }
    SPDLOG_ERROR("CUDA pixel backend setup failed: {}", e.what());
    currentErrorMessage = e.what();
  }
}

CudaPixelSim::~CudaPixelSim() = default;

void CudaPixelSim::beginCompartmentParallelLaunches() {
  throwIfCudaFailed(cuEventRecord(impl->mainStreamReady, impl->stream),
                    "Failed to record main-stream-ready event");
  for (auto &state : impl->compartments) {
    throwIfCudaFailed(cuStreamWaitEvent(state.stream, impl->mainStreamReady, 0),
                      "Failed to sync compartment stream with main stream");
  }
}

void CudaPixelSim::endCompartmentParallelLaunches() {
  for (auto &state : impl->compartments) {
    throwIfCudaFailed(cuStreamWaitEvent(impl->stream, state.dcdtReady, 0),
                      "Failed to sync main stream with compartment");
  }
}

void CudaPixelSim::evaluateDcdt() {
  // Record event on main stream so compartment streams can wait for prior work
  beginCompartmentParallelLaunches();
  // Launch reaction and diffusion kernels on per-compartment streams
  for (auto &state : impl->compartments) {
    auto nPixels = static_cast<unsigned int>(state.nPixels);
    const auto voxelGrid =
        (nPixels + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 3> reactionArgs{&state.dConc, &state.dDcdt, &nPixels};
    launchKernel(state.kernels.reaction, voxelGrid, cudaKernelBlockSize,
                 state.stream, reactionArgs.data(), "reaction_kernel");
    std::array<void *, 5> diffusionArgs{&state.dConc, &state.dDcdt, &state.dNn,
                                        &state.dDiffusion, &nPixels};
    launchKernel(state.kernels.diffusionUniform, voxelGrid, cudaKernelBlockSize,
                 state.stream, diffusionArgs.data(),
                 "diffusion_uniform_kernel");
    // Record event so membrane kernels can wait for this compartment
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment dcdt-ready event");
  }
  // Membrane reactions use atomicAdd on dDcdt, so all can run in parallel.
  // Each compartment stream waits for the dcdtReady events of compartments
  // whose buffers it will touch, then launches its assigned membrane kernels.
  for (std::size_t si = 0; si < impl->compartments.size(); ++si) {
    for (auto depIdx : impl->membraneStreamDeps[si]) {
      throwIfCudaFailed(
          cuStreamWaitEvent(impl->compartments[si].stream,
                            impl->compartments[depIdx].dcdtReady, 0),
          "Failed to sync compartment stream with membrane dependency");
    }
  }
  for (const auto &launch : impl->membraneLaunches) {
    auto &membrane = impl->membranes[launch.membraneIndex];
    auto &launchStream = impl->compartments[launch.streamIndex].stream;
    CUdeviceptr dConcA{
        membrane.compartmentIndexA != invalidCompartmentIndex
            ? impl->compartments[membrane.compartmentIndexA].dConc
            : 0};
    CUdeviceptr dDcdtA{
        membrane.compartmentIndexA != invalidCompartmentIndex
            ? impl->compartments[membrane.compartmentIndexA].dDcdt
            : 0};
    CUdeviceptr dConcB{
        membrane.compartmentIndexB != invalidCompartmentIndex
            ? impl->compartments[membrane.compartmentIndexB].dConc
            : 0};
    CUdeviceptr dDcdtB{
        membrane.compartmentIndexB != invalidCompartmentIndex
            ? impl->compartments[membrane.compartmentIndexB].dDcdt
            : 0};
    auto nPairs = membrane.nFacePairs[launch.faceIndex];
    const auto faceGrid =
        (nPairs + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    auto invFluxLength = membrane.faceInvFluxLengths[launch.faceIndex];
    auto invFluxLengthF = static_cast<float>(invFluxLength);
    std::array<void *, 7> membraneArgs{
        &membrane.dFaceIndexPairs[launch.faceIndex],
        &nPairs,
        &dConcA,
        &dDcdtA,
        &dConcB,
        &dDcdtB,
        impl->useFloat ? static_cast<void *>(&invFluxLengthF)
                       : static_cast<void *>(&invFluxLength)};
    launchKernel(membrane.kernels.reaction, faceGrid, cudaKernelBlockSize,
                 launchStream, membraneArgs.data(), "membrane_reaction_kernel");
  }
  // Sync all compartment streams back to main stream
  for (auto &state : impl->compartments) {
    // Re-record event to capture any membrane work added to this stream
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

void CudaPixelSim::launchRk101Update(double dt) {
  float dtF = static_cast<float>(dt);
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nValues = static_cast<unsigned int>(state.concHost.size());
    const auto valueGrid =
        (nValues + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 4> updateArgs{&state.dConc, &state.dDcdt,
                                     impl->useFloat ? static_cast<void *>(&dtF)
                                                    : static_cast<void *>(&dt),
                                     &nValues};
    launchKernel(state.kernels.rk101Update, valueGrid, cudaKernelBlockSize,
                 state.stream, updateArgs.data(), "rk101_update_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

void CudaPixelSim::launchClampNegative() {
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nPixels = static_cast<unsigned int>(state.nPixels);
    const auto voxelGrid =
        (nPixels + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 2> clampArgs{&state.dConc, &nPixels};
    launchKernel(state.kernels.clampNegative, voxelGrid, cudaKernelBlockSize,
                 state.stream, clampArgs.data(), "clamp_negative_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

void CudaPixelSim::launchRk212Substep1(double dt) {
  float dtF = static_cast<float>(dt);
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nValues = static_cast<unsigned int>(state.concHost.size());
    const auto valueGrid =
        (nValues + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 6> substepArgs{&state.dConc,
                                      &state.dDcdt,
                                      &state.dLowerOrder,
                                      &state.dOldConc,
                                      impl->useFloat ? static_cast<void *>(&dtF)
                                                     : static_cast<void *>(&dt),
                                      &nValues};
    launchKernel(state.kernels.rk212Substep1, valueGrid, cudaKernelBlockSize,
                 state.stream, substepArgs.data(), "rk212_substep1_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

void CudaPixelSim::launchRk212Substep2(double dt) {
  float dtF = static_cast<float>(dt);
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nValues = static_cast<unsigned int>(state.concHost.size());
    const auto valueGrid =
        (nValues + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 6> substepArgs{&state.dConc,
                                      &state.dDcdt,
                                      &state.dLowerOrder,
                                      &state.dOldConc,
                                      impl->useFloat ? static_cast<void *>(&dtF)
                                                     : static_cast<void *>(&dt),
                                      &nValues};
    launchKernel(state.kernels.rk212Substep2, valueGrid, cudaKernelBlockSize,
                 state.stream, substepArgs.data(), "rk212_substep2_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

void CudaPixelSim::launchRkInit() {
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nValues = static_cast<unsigned int>(state.concHost.size());
    const auto valueGrid =
        (nValues + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 4> args{&state.dConc, &state.dLowerOrder,
                               &state.dOldConc, &nValues};
    launchKernel(state.kernels.rkInit, valueGrid, cudaKernelBlockSize,
                 state.stream, args.data(), "rk_init_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

void CudaPixelSim::launchRkSubstep(double dt, double g1Val, double g2Val,
                                   double g3Val, double betaVal,
                                   double deltaVal) {
  float dtF = static_cast<float>(dt);
  float g1F = static_cast<float>(g1Val);
  float g2F = static_cast<float>(g2Val);
  float g3F = static_cast<float>(g3Val);
  float betaF = static_cast<float>(betaVal);
  float deltaF = static_cast<float>(deltaVal);
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nValues = static_cast<unsigned int>(state.concHost.size());
    const auto valueGrid =
        (nValues + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 11> args{&state.dConc,
                                &state.dDcdt,
                                &state.dLowerOrder,
                                &state.dOldConc,
                                impl->useFloat ? static_cast<void *>(&dtF)
                                               : static_cast<void *>(&dt),
                                impl->useFloat ? static_cast<void *>(&g1F)
                                               : static_cast<void *>(&g1Val),
                                impl->useFloat ? static_cast<void *>(&g2F)
                                               : static_cast<void *>(&g2Val),
                                impl->useFloat ? static_cast<void *>(&g3F)
                                               : static_cast<void *>(&g3Val),
                                impl->useFloat ? static_cast<void *>(&betaF)
                                               : static_cast<void *>(&betaVal),
                                impl->useFloat ? static_cast<void *>(&deltaF)
                                               : static_cast<void *>(&deltaVal),
                                &nValues};
    launchKernel(state.kernels.rkSubstep, valueGrid, cudaKernelBlockSize,
                 state.stream, args.data(), "rk_substep_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

void CudaPixelSim::launchRkFinalise(double cFactor, double s2Factor,
                                    double s3Factor) {
  float cFactorF = static_cast<float>(cFactor);
  float s2FactorF = static_cast<float>(s2Factor);
  float s3FactorF = static_cast<float>(s3Factor);
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nValues = static_cast<unsigned int>(state.concHost.size());
    const auto valueGrid =
        (nValues + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 7> args{&state.dConc,
                               &state.dLowerOrder,
                               &state.dOldConc,
                               impl->useFloat ? static_cast<void *>(&cFactorF)
                                              : static_cast<void *>(&cFactor),
                               impl->useFloat ? static_cast<void *>(&s2FactorF)
                                              : static_cast<void *>(&s2Factor),
                               impl->useFloat ? static_cast<void *>(&s3FactorF)
                                              : static_cast<void *>(&s3Factor),
                               &nValues};
    launchKernel(state.kernels.rkFinalise, valueGrid, cudaKernelBlockSize,
                 state.stream, args.data(), "rk_finalise_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
}

PixelIntegratorError CudaPixelSim::calculateRk212Error() {
  PixelIntegratorError err{0.0, 0.0};
  float epsilonF = static_cast<float>(epsilon);
  beginCompartmentParallelLaunches();
  for (auto &state : impl->compartments) {
    auto nValues = static_cast<unsigned int>(state.concHost.size());
    const auto valueGrid =
        (nValues + cudaKernelBlockSize - 1) / cudaKernelBlockSize;
    std::array<void *, 7> errorArgs{&state.dConc,
                                    &state.dLowerOrder,
                                    &state.dOldConc,
                                    impl->useFloat
                                        ? static_cast<void *>(&epsilonF)
                                        : static_cast<void *>(&epsilon),
                                    &state.dErrorAbs,
                                    &state.dErrorRel,
                                    &nValues};
    launchKernel(state.kernels.rk212Error, valueGrid, cudaKernelBlockSize,
                 state.stream, errorArgs.data(), "rk212_error_kernel");
    auto nBlocks = static_cast<unsigned int>(state.nErrorBlocks);
    std::array<void *, 4> reduceArgs{&state.dErrorAbs, &state.dErrorRel,
                                     &state.dErrorResult, &nBlocks};
    launchKernel(state.kernels.rk212ErrorReduce, 1, cudaKernelBlockSize,
                 state.stream, reduceArgs.data(), "rk212_error_reduce_kernel");
    throwIfCudaFailed(cuEventRecord(state.dcdtReady, state.stream),
                      "Failed to record compartment completion event");
  }
  endCompartmentParallelLaunches();
  throwIfCudaFailed(cuStreamSynchronize(impl->stream),
                    "CUDA RK212 error synchronization failed");
  const auto eSize = impl->gpuElementSize;
  for (auto &state : impl->compartments) {
    if (impl->useFloat) {
      std::array<float, 2> resultF{};
      throwIfCudaFailed(
          cuMemcpyDtoH(resultF.data(), state.dErrorResult, 2 * eSize),
          "Failed to download CUDA RK error result");
      err.abs = std::max(err.abs, static_cast<double>(resultF[0]));
      err.rel = std::max(err.rel, static_cast<double>(resultF[1]));
    } else {
      std::array<double, 2> resultD{};
      throwIfCudaFailed(
          cuMemcpyDtoH(resultD.data(), state.dErrorResult, 2 * eSize),
          "Failed to download CUDA RK error result");
      err.abs = std::max(err.abs, resultD[0]);
      err.rel = std::max(err.rel, resultD[1]);
    }
  }
  return err;
}

void CudaPixelSim::restorePreStepConcentrations() {
  for (auto &state : impl->compartments) {
    throwIfCudaFailed(
        cuMemcpyDtoDAsync(state.dConc, state.dOldConc,
                          state.concHost.size() * impl->gpuElementSize,
                          impl->stream),
        "Failed to restore CUDA concentrations after rejected RK212 step");
  }
}

void CudaPixelSim::downloadStateToHost() {
  // Record an event on the compute stream so the transfer stream waits for
  // all GPU kernels to finish before starting the download
  CUevent computeDone{};
  throwIfCudaFailed(cuEventCreate(&computeDone, CU_EVENT_DISABLE_TIMING),
                    "Failed to create compute-done event");
  throwIfCudaFailed(cuEventRecord(computeDone, impl->stream),
                    "Failed to record compute-done event");
  throwIfCudaFailed(cuStreamWaitEvent(impl->transferStream, computeDone, 0),
                    "Failed to make transfer stream wait for compute");
  cuEventDestroy(computeDone);
  // Launch async copies on the transfer stream
  for (auto &state : impl->compartments) {
    const auto nBytes = state.concHost.size() * impl->gpuElementSize;
    if (impl->useFloat) {
      state.concFloatHost.resize(state.concHost.size());
      throwIfCudaFailed(cuMemcpyDtoHAsync(state.concFloatHost.data(),
                                          state.dConc, nBytes,
                                          impl->transferStream),
                        "Failed to async download CUDA concentrations");
    } else {
      throwIfCudaFailed(cuMemcpyDtoHAsync(state.concHost.data(), state.dConc,
                                          nBytes, impl->transferStream),
                        "Failed to async download CUDA concentrations");
    }
  }
  // Record event so we know when the download is complete
  throwIfCudaFailed(cuEventRecord(impl->downloadComplete, impl->transferStream),
                    "Failed to record download-complete event");
  impl->downloadPending = true;
}

void CudaPixelSim::waitForDownload() {
  if (!impl->downloadPending) {
    return;
  }
  throwIfCudaFailed(cuEventSynchronize(impl->downloadComplete),
                    "Failed to wait for CUDA download to complete");
  if (impl->useFloat) {
    for (auto &state : impl->compartments) {
      std::copy(state.concFloatHost.begin(), state.concFloatHost.end(),
                state.concHost.begin());
    }
  }
  impl->downloadPending = false;
}

void CudaPixelSim::ensureDownloadComplete() {
  // Make the compute stream wait until any in-flight download finishes,
  // so that kernels that write to dConc don't race with the transfer.
  throwIfCudaFailed(cuStreamWaitEvent(impl->stream, impl->downloadComplete, 0),
                    "Failed to sync compute stream with download");
}

double CudaPixelSim::doRK101(double dt) {
  dt = std::min(dt, maxStableTimestep);
  evaluateDcdt();
  ensureDownloadComplete();
  launchRk101Update(dt);
  launchClampNegative();
  return dt;
}

void CudaPixelSim::doRK212(double dt) {
  evaluateDcdt();
  ensureDownloadComplete();
  launchRk212Substep1(dt);
  evaluateDcdt();
  launchRk212Substep2(dt);
}

void CudaPixelSim::doRK323(double dt) {
  using namespace detail::rk323;
  launchRkInit();
  evaluateDcdt();
  ensureDownloadComplete();
  launchRkSubstep(dt, g1[0], g2[0], g3[0], beta[0], delta[0]);
  for (std::size_t i = 1; i < g1.size(); ++i) {
    evaluateDcdt();
    launchRkSubstep(dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
  }
  launchRkFinalise(finaliseFactors[0], finaliseFactors[1], finaliseFactors[2]);
}

void CudaPixelSim::doRK435(double dt) {
  using namespace detail::rk435;
  launchRkInit();
  evaluateDcdt();
  ensureDownloadComplete();
  launchRkSubstep(dt, g1[0], g2[0], g3[0], beta[0], delta[0]);
  for (std::size_t i = 1; i < g1.size(); ++i) {
    evaluateDcdt();
    launchRkSubstep(dt, g1[i], g2[i], g3[i], beta[i], delta[i]);
  }
  launchRkFinalise(deltaSumReciprocal * delta[5], deltaSumReciprocal,
                   deltaSumReciprocal * delta[6]);
}

double CudaPixelSim::doRKAdaptive(double dtMax) {
  PixelIntegratorError err{0.0, 0.0};
  double dt{};
  const double errPower = detail::getErrorPower(integrator);
  do {
    dt = std::min(nextTimestep, dtMax);
    if (integrator == PixelIntegratorType::RK323) {
      doRK323(dt);
    } else if (integrator == PixelIntegratorType::RK435) {
      doRK435(dt);
    } else {
      doRK212(dt);
    }
    err = calculateRk212Error();
    double errFactor = std::min(errMax.abs / err.abs, errMax.rel / err.rel);
    errFactor = std::pow(errFactor, errPower);
    nextTimestep = std::min(0.95 * dt * errFactor, dtMax);
    SPDLOG_TRACE("CUDA dt = {} gave rel err = {}, abs err = {} -> new dt = {}",
                 dt, err.rel, err.abs, nextTimestep);
    if (nextTimestep / dtMax < 1e-20) {
      currentErrorImages.clear();
      currentErrorMessage =
          fmt::format("Failed to solve CUDA model to the required accuracy.");
      return nextTimestep;
    }
    if (err.abs > errMax.abs || err.rel > errMax.rel) {
      ++discardedSteps;
      restorePreStepConcentrations();
    }
  } while (err.abs > errMax.abs || err.rel > errMax.rel);
  launchClampNegative();
  return dt;
}

std::size_t
CudaPixelSim::run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) {
  if (!currentErrorMessage.empty()) {
    return 0;
  }
  currentErrorMessage.clear();
  currentErrorImages.clear();
  ScopedCudaContext scopedContext{impl->context};
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
    SPDLOG_ERROR("CUDA pixel backend run failed: {}", currentErrorMessage);
  }
  return steps;
}

const std::vector<double> &
CudaPixelSim::getConcentrations(std::size_t compartmentIndex) const {
  const_cast<CudaPixelSim *>(this)->waitForDownload();
  return impl->compartments[compartmentIndex].concHost;
}

std::size_t CudaPixelSim::getConcentrationPadding() const { return 0; }

const std::vector<double> &
CudaPixelSim::getDcdt(std::size_t compartmentIndex) const {
  auto &state = impl->compartments[compartmentIndex];
  ScopedCudaContext scopedContext{impl->context};
  downloadGpuToDoubles(state.dcdtHost, state.dDcdt, impl->useFloat,
                       impl->floatConversionBuffer,
                       "Failed to download CUDA dcdt");
  return impl->compartments[compartmentIndex].dcdtHost;
}

double CudaPixelSim::getLowerOrderConcentration(std::size_t compartmentIndex,
                                                std::size_t speciesIndex,
                                                std::size_t pixelIndex) const {
  auto &state = impl->compartments[compartmentIndex];
  if (state.lowerOrderHost.size() != state.concHost.size()) {
    state.lowerOrderHost.resize(state.concHost.size());
  }
  ScopedCudaContext scopedContext{impl->context};
  downloadGpuToDoubles(state.lowerOrderHost, state.dLowerOrder, impl->useFloat,
                       impl->floatConversionBuffer,
                       "Failed to download CUDA RK lower-order "
                       "concentrations");
  const auto &stateRef = impl->compartments[compartmentIndex];
  if (stateRef.lowerOrderHost.empty()) {
    return 0.0;
  }
  return stateRef.lowerOrderHost[pixelIndex * stateRef.nSpecies + speciesIndex];
}

} // namespace sme::simulate
