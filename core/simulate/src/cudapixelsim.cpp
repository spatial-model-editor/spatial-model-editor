#include "cudapixelsim.hpp"
#include "pixelsim_impl.hpp"
#include "sme/geometry.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include "sme/simple_symbolic.hpp"
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

#ifdef SME_HAS_CUDA_PIXEL
#include <cuda.h>
#include <nvrtc.h>
#endif

namespace sme::simulate {

namespace {

class CudaPixelSimError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

static double calculateMaxStableTimestep(
    const std::array<double, 3> &dimensionlessDiffusion) {
  return 1.0 / (2.0 * (dimensionlessDiffusion[0] + dimensionlessDiffusion[1] +
                       dimensionlessDiffusion[2]));
}

static bool hasAnyMembraneReactions(const model::Model &doc) {
  for (const auto &membrane : doc.getMembranes().getMembranes()) {
    if (auto reacsInMembrane =
            doc.getReactions().getIds(membrane.getId().c_str());
        !reacsInMembrane.isEmpty()) {
      return true;
    }
  }
  return false;
}

static bool hasAnyCrossDiffusion(
    const model::Model &doc,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds) {
  for (const auto &speciesIds : compartmentSpeciesIds) {
    for (const auto &targetSpeciesId : speciesIds) {
      for (const auto &sourceSpeciesId : speciesIds) {
        if (targetSpeciesId == sourceSpeciesId) {
          continue;
        }
        if (!doc.getSpecies()
                 .getCrossDiffusionConstant(targetSpeciesId.c_str(),
                                            sourceSpeciesId.c_str())
                 .isEmpty()) {
          return true;
        }
      }
    }
  }
  return false;
}

static std::string sanitizeCudaExpression(std::string expr) {
  // PoC path: assume the supplied expression is already valid CUDA C.
  // This hook makes it easy to replace the implementation with a dedicated
  // SymEngine CUDA printer later.
  return expr;
}

static void validatePocCompartmentInputs(
    const model::Model &doc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds) {
  for (std::size_t compIndex = 0; compIndex < compartmentIds.size();
       ++compIndex) {
    const auto *compartment{doc.getCompartments().getCompartment(
        compartmentIds[compIndex].c_str())};
    if (compartment == nullptr) {
      throw CudaPixelSimError(
          fmt::format("CUDA pixel backend PoC could not find compartment '{}'",
                      compartmentIds[compIndex]));
    }
    for (const auto &speciesId : compartmentSpeciesIds[compIndex]) {
      const auto *field = doc.getSpecies().getField(speciesId.c_str());
      if (field == nullptr) {
        throw CudaPixelSimError(fmt::format(
            "CUDA pixel backend PoC could not find field for species '{}'",
            speciesId));
      }
      if (!field->getIsSpatial()) {
        throw CudaPixelSimError(
            "CUDA pixel backend PoC does not yet support non-spatial species");
      }
      if (!field->getIsUniformDiffusionConstant()) {
        throw CudaPixelSimError(
            "CUDA pixel backend PoC does not yet support non-uniform diffusion "
            "constants");
      }
      if (doc.getSpecies().getStorage(speciesId.c_str()) != 1.0) {
        throw CudaPixelSimError(
            "CUDA pixel backend PoC currently requires unit storage for all "
            "species");
      }
    }
  }
}

#ifdef SME_HAS_CUDA_PIXEL
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
#endif

} // namespace

struct CudaKernelBundle {
#ifdef SME_HAS_CUDA_PIXEL
  CUmodule module{};
  CUfunction reaction{};
  CUfunction diffusionUniform{};
  CUfunction rk101Update{};
  CUfunction clampNegative{};
#endif
};

struct CudaPixelSim::Impl {
  struct CompartmentState {
    std::string compartmentId{};
    std::size_t nPixels{};
    std::size_t nSpecies{};
    std::vector<double> concHost{};
    std::vector<double> dcdtHost{};
    std::vector<std::uint32_t> nnHost{};
    std::vector<double> diffusionHost{};
    double maxStableTimestep{std::numeric_limits<double>::max()};
    CudaKernelBundle kernels{};
#ifdef SME_HAS_CUDA_PIXEL
    CUdeviceptr dConc{};
    CUdeviceptr dDcdt{};
    CUdeviceptr dNn{};
    CUdeviceptr dDiffusion{};
#endif
  };

  std::vector<CompartmentState> compartments{};
#ifdef SME_HAS_CUDA_PIXEL
  CUdevice device{};
  CUcontext context{};
  CUstream stream{};

  ~Impl() {
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
      if (compartment.dNn != 0) {
        cuMemFree(compartment.dNn);
      }
      if (compartment.dDiffusion != 0) {
        cuMemFree(compartment.dDiffusion);
      }
    }
    if (stream != nullptr) {
      cuStreamDestroy(stream);
    }
    if (context != nullptr) {
      cuCtxDestroy(context);
    }
  }
#endif
};

#ifdef SME_HAS_CUDA_PIXEL
static std::string makeCudaKernelSource(const ReacExpr &reacExpr) {
  std::ostringstream src;
  src << "extern \"C\" __device__ __forceinline__ void reaction_eval(double* "
         "out, const double* c) {\n";
  for (std::size_t i = 0; i < reacExpr.variables.size(); ++i) {
    src << "  const double " << reacExpr.variables[i] << " = c[" << i << "];\n";
  }
  for (std::size_t i = 0; i < reacExpr.expressions.size(); ++i) {
    src << "  out[" << i
        << "] = " << sanitizeCudaExpression(reacExpr.expressions[i]) << ";\n";
  }
  src << "}\n\n";
  src << "constexpr unsigned int N_SPECIES = " << reacExpr.variables.size()
      << ";\n\n";
  src << R"(
extern "C" __global__ void reaction_kernel(const double* conc, double* dcdt,
                                           unsigned int nPixels) {
  const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
  if (ix >= nPixels) {
    return;
  }
  reaction_eval(dcdt + ix * N_SPECIES, conc + ix * N_SPECIES);
}

extern "C" __global__ void diffusion_uniform_kernel(
    const double* conc, double* dcdt, const unsigned int* nn,
    const double* diffusion, unsigned int nPixels) {
  const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
  if (ix >= nPixels) {
    return;
  }
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
  for (unsigned int is = 0; is < N_SPECIES; ++is) {
    const double c0 = conc[iCenter + is];
    dcdt[iCenter + is] +=
        diffusion[3 * is] *
            (conc[iUpx + is] + conc[iDnx + is] - 2.0 * c0) +
        diffusion[3 * is + 1] *
            (conc[iUpy + is] + conc[iDny + is] - 2.0 * c0) +
        diffusion[3 * is + 2] *
            (conc[iUpz + is] + conc[iDnz + is] - 2.0 * c0);
  }
}

extern "C" __global__ void rk101_update_kernel(double* conc, const double* dcdt,
                                                double dt,
                                                unsigned int nValues) {
  const unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
  if (i >= nValues) {
    return;
  }
  conc[i] += dt * dcdt[i];
}

extern "C" __global__ void clamp_negative_kernel(double* conc,
                                                 unsigned int nPixels) {
  const unsigned int ix = blockIdx.x * blockDim.x + threadIdx.x;
  if (ix >= nPixels) {
    return;
  }
  const unsigned int offset = ix * N_SPECIES;
  for (unsigned int is = 0; is < N_SPECIES; ++is) {
    if (conc[offset + is] < 0.0) {
      conc[offset + is] = 0.0;
    }
  }
}
)";
  return src.str();
}

static CudaKernelBundle compileCudaKernelBundle(const ReacExpr &reacExpr,
                                                int computeCapabilityMajor,
                                                int computeCapabilityMinor) {
  auto source = makeCudaKernelSource(reacExpr);
  nvrtcProgram program{};
  throwIfNvrtcFailed(nvrtcCreateProgram(&program, source.c_str(),
                                        "sme_pixel_poc.cu", 0, nullptr,
                                        nullptr),
                     "Failed to create NVRTC program");
  try {
    std::string arch =
        fmt::format("--gpu-architecture=sm_{}{}", computeCapabilityMajor,
                    computeCapabilityMinor);
    std::array<const char *, 2> options{"--std=c++17", arch.c_str()};
    const auto compileResult = nvrtcCompileProgram(
        program, static_cast<int>(options.size()), options.data());
    std::size_t logSize{};
    throwIfNvrtcFailed(nvrtcGetProgramLogSize(program, &logSize),
                       "Failed to get NVRTC log size");
    std::string compileLog(logSize, '\0');
    if (logSize > 1) {
      throwIfNvrtcFailed(nvrtcGetProgramLog(program, compileLog.data()),
                         "Failed to get NVRTC compile log");
      SPDLOG_DEBUG("NVRTC compile log:\n{}", compileLog);
    }
    throwIfNvrtcFailed(compileResult, "Failed to compile CUDA pixel kernel");

    std::size_t cubinSize{};
    throwIfNvrtcFailed(nvrtcGetCUBINSize(program, &cubinSize),
                       "Failed to get NVRTC cubin size");
    std::vector<char> cubin(cubinSize);
    throwIfNvrtcFailed(nvrtcGetCUBIN(program, cubin.data()),
                       "Failed to extract NVRTC cubin");

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

static void launchKernel(CUfunction kernel, unsigned int gridX,
                         unsigned int blockX, CUstream stream, void **args,
                         const std::string &name) {
  throwIfCudaFailed(cuLaunchKernel(kernel, gridX, 1, 1, blockX, 1, 1, 0, stream,
                                   args, nullptr),
                    fmt::format("Failed to launch CUDA kernel '{}'", name));
}
#endif

CudaPixelSim::CudaPixelSim(
    const model::Model &sbmlDoc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    const std::map<std::string, double, std::less<>> &substitutions)
    : impl(std::make_unique<Impl>()), doc{sbmlDoc},
      integrator{sbmlDoc.getSimulationSettings().options.pixel.integrator},
      maxTimestep{sbmlDoc.getSimulationSettings().options.pixel.maxTimestep} {
  try {
    if (sbmlDoc.getSimulationSettings().options.pixel.backend !=
        PixelBackendType::CUDA) {
      throw CudaPixelSimError("CUDA pixel backend was not selected");
    }
    if (integrator != PixelIntegratorType::RK101) {
      throw CudaPixelSimError(
          "CUDA pixel backend PoC only supports the RK101 integrator");
    }
    if (hasAnyMembraneReactions(doc)) {
      throw CudaPixelSimError(
          "CUDA pixel backend PoC does not yet support membrane reactions");
    }
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
    validatePocCompartmentInputs(doc, compartmentIds, compartmentSpeciesIds);
#ifndef SME_HAS_CUDA_PIXEL
    throw CudaPixelSimError(
        "CUDA pixel backend was requested, but SME was built without "
        "SME_ENABLE_CUDA_PIXEL");
#else
    throwIfCudaFailed(cuInit(0), "Failed to initialize the CUDA driver");
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
    throwIfCudaFailed(cuStreamCreate(&impl->stream, CU_STREAM_DEFAULT),
                      "Failed to create the CUDA stream");
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
      state.kernels = compileCudaKernelBundle(reacExpr, computeCapabilityMajor,
                                              computeCapabilityMinor);

      throwIfCudaFailed(
          cuMemAlloc(&state.dConc,
                     state.concHost.size() * sizeof(state.concHost.front())),
          "Failed to allocate CUDA concentration buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dDcdt,
                     state.dcdtHost.size() * sizeof(state.dcdtHost.front())),
          "Failed to allocate CUDA dcdt buffer");
      throwIfCudaFailed(
          cuMemAlloc(&state.dNn,
                     state.nnHost.size() * sizeof(state.nnHost.front())),
          "Failed to allocate CUDA neighbor buffer");
      throwIfCudaFailed(cuMemAlloc(&state.dDiffusion,
                                   state.diffusionHost.size() * sizeof(double)),
                        "Failed to allocate CUDA diffusion buffer");

      throwIfCudaFailed(
          cuMemcpyHtoD(state.dConc, state.concHost.data(),
                       state.concHost.size() * sizeof(state.concHost.front())),
          "Failed to upload initial CUDA concentrations");
      throwIfCudaFailed(
          cuMemcpyHtoD(state.dNn, state.nnHost.data(),
                       state.nnHost.size() * sizeof(state.nnHost.front())),
          "Failed to upload CUDA neighbor topology");
      throwIfCudaFailed(cuMemcpyHtoD(state.dDiffusion,
                                     state.diffusionHost.data(),
                                     state.diffusionHost.size() *
                                         sizeof(state.diffusionHost.front())),
                        "Failed to upload CUDA diffusion coefficients");
      impl->compartments.push_back(std::move(state));
    }

    const auto &data{sbmlDoc.getSimulationData()};
    if (data.concentration.size() > 1 && !data.concentration.back().empty() &&
        data.concentration.back().size() == impl->compartments.size()) {
      SPDLOG_INFO("Applying supplied initial concentrations to CUDA backend");
      for (std::size_t i = 0; i < impl->compartments.size(); ++i) {
        auto &state = impl->compartments[i];
        if (data.concentration.back()[i].size() == state.concHost.size()) {
          state.concHost = data.concentration.back()[i];
          throwIfCudaFailed(cuMemcpyHtoD(state.dConc, state.concHost.data(),
                                         state.concHost.size() *
                                             sizeof(state.concHost.front())),
                            "Failed to upload supplied CUDA concentrations");
        }
      }
    }
#endif
  } catch (const std::exception &e) {
    SPDLOG_ERROR("CUDA pixel backend setup failed: {}", e.what());
    currentErrorMessage = e.what();
  }
}

CudaPixelSim::~CudaPixelSim() = default;

std::size_t
CudaPixelSim::run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) {
  if (!currentErrorMessage.empty()) {
    return 0;
  }
#ifndef SME_HAS_CUDA_PIXEL
  currentErrorMessage =
      "CUDA pixel backend was requested, but SME was built without "
      "SME_ENABLE_CUDA_PIXEL";
  return 0;
#else
  currentErrorMessage.clear();
  QElapsedTimer timer;
  timer.start();
  double tNow = 0.0;
  std::size_t steps = 0;
  constexpr double relativeTolerance = 1e-12;
  constexpr unsigned int blockSize = 128;
  try {
    while (tNow + time * relativeTolerance < time) {
      const double dt = std::min({maxTimestep, maxStableTimestep, time - tNow});
      for (auto &state : impl->compartments) {
        unsigned int nPixels = static_cast<unsigned int>(state.nPixels);
        unsigned int nValues = static_cast<unsigned int>(state.concHost.size());
        double timestep = dt;
        const unsigned int voxelGrid = (nPixels + blockSize - 1) / blockSize;
        const unsigned int valueGrid = (nValues + blockSize - 1) / blockSize;
        void *reactionArgs[] = {&state.dConc, &state.dDcdt, &nPixels};
        launchKernel(state.kernels.reaction, voxelGrid, blockSize, impl->stream,
                     reactionArgs, "reaction_kernel");
        void *diffusionArgs[] = {&state.dConc, &state.dDcdt, &state.dNn,
                                 &state.dDiffusion, &nPixels};
        launchKernel(state.kernels.diffusionUniform, voxelGrid, blockSize,
                     impl->stream, diffusionArgs, "diffusion_uniform_kernel");
        void *updateArgs[] = {&state.dConc, &state.dDcdt, &timestep, &nValues};
        launchKernel(state.kernels.rk101Update, valueGrid, blockSize,
                     impl->stream, updateArgs, "rk101_update_kernel");
        void *clampArgs[] = {&state.dConc, &nPixels};
        launchKernel(state.kernels.clampNegative, voxelGrid, blockSize,
                     impl->stream, clampArgs, "clamp_negative_kernel");
      }
      throwIfCudaFailed(cuStreamSynchronize(impl->stream),
                        "CUDA pixel timestep synchronization failed");
      tNow += dt;
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
    for (auto &state : impl->compartments) {
      throwIfCudaFailed(
          cuMemcpyDtoH(state.concHost.data(), state.dConc,
                       state.concHost.size() * sizeof(state.concHost.front())),
          "Failed to download CUDA concentrations");
      throwIfCudaFailed(
          cuMemcpyDtoH(state.dcdtHost.data(), state.dDcdt,
                       state.dcdtHost.size() * sizeof(state.dcdtHost.front())),
          "Failed to download CUDA dcdt");
    }
  } catch (const std::exception &e) {
    currentErrorMessage = e.what();
    SPDLOG_ERROR("CUDA pixel backend run failed: {}", currentErrorMessage);
  }
  return steps;
#endif
}

const std::vector<double> &
CudaPixelSim::getConcentrations(std::size_t compartmentIndex) const {
  return impl->compartments[compartmentIndex].concHost;
}

std::size_t CudaPixelSim::getConcentrationPadding() const { return 0; }

const std::vector<double> &
CudaPixelSim::getDcdt(std::size_t compartmentIndex) const {
  return impl->compartments[compartmentIndex].dcdtHost;
}

double CudaPixelSim::getLowerOrderConcentration(std::size_t compartmentIndex,
                                                std::size_t speciesIndex,
                                                std::size_t pixelIndex) const {
  (void)compartmentIndex;
  (void)speciesIndex;
  (void)pixelIndex;
  return 0.0;
}

const std::string &CudaPixelSim::errorMessage() const {
  return currentErrorMessage;
}

const common::ImageStack &CudaPixelSim::errorImages() const {
  return currentErrorImages;
}

void CudaPixelSim::setStopRequested(bool stop) { stopRequested.store(stop); }

bool CudaPixelSim::getStopRequested() const { return stopRequested.load(); }

void CudaPixelSim::setCurrentErrormessage(const std::string &msg) {
  currentErrorMessage = msg;
}

} // namespace sme::simulate
