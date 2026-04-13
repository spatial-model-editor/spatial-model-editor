// CUDA Driver API stub implementations.
//
// These functions have the same signatures as the real CUDA driver functions
// (after cuda.h macro expansion). They forward calls through function pointers
// resolved at runtime via cuGetProcAddress (NVIDIA's recommended approach),
// so the binary can run without NVIDIA drivers installed.
//
// NVRTC is statically linked, so only the CUDA driver library is loaded here.

#include "sme/cuda_stubs.hpp"
#include "sme/logger.hpp"
#include <algorithm>
#include <cuda.h>
#include <optional>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

// Platform-specific library loading
#ifdef _WIN32
using LibHandle = HMODULE;
LibHandle loadLib(const char *name) { return LoadLibraryA(name); }
void *loadSym(LibHandle lib, const char *name) {
  return reinterpret_cast<void *>(GetProcAddress(lib, name));
}
#else
using LibHandle = void *;
LibHandle loadLib(const char *name) { return dlopen(name, RTLD_LAZY); }
void *loadSym(LibHandle lib, const char *name) { return dlsym(lib, name); }
#endif

// cuGetProcAddress (v1) signature — resolved via dlsym
using CuGetProcAddressFn = CUresult(CUDAAPI *)(const char *, void **, int,
                                               cuuint64_t);

// X(Name, Params, Args):
//   Name   - CUDA driver API base name without the leading "cu", used for both
//            the cu.Name function-pointer member and the exported cu##Name
//            stub.
//   Params - full function parameter list for declaration/definition.
//   Args   - forwarded argument list passed through to the resolved function.
//
// These entries match the CUDA 13 driver headers we currently build against.
// If a future CUDA toolkit changes the macro-mapped ABI variants or function
// signatures in cuda.h, this list may need to be updated accordingly.
#define CUDA_DRIVER_APIS(X)                                                    \
  X(Init, (unsigned int Flags), (Flags))                                       \
  X(DeviceGet, (CUdevice * device, int ordinal), (device, ordinal))            \
  X(DeviceGetCount, (int *count), (count))                                     \
  X(DeviceGetAttribute, (int *pi, CUdevice_attribute attrib, CUdevice dev),    \
    (pi, attrib, dev))                                                         \
  X(DeviceTotalMem, (size_t *bytes, CUdevice dev), (bytes, dev))               \
  X(CtxCreate,                                                                 \
    (CUcontext * pctx, CUctxCreateParams * params, unsigned int flags,         \
     CUdevice dev),                                                            \
    (pctx, params, flags, dev))                                                \
  X(CtxDestroy, (CUcontext ctx), (ctx))                                        \
  X(CtxSetCurrent, (CUcontext ctx), (ctx))                                     \
  X(CtxGetCurrent, (CUcontext * pctx), (pctx))                                 \
  X(ModuleLoadDataEx,                                                          \
    (CUmodule * module, const void *image, unsigned int numOptions,            \
     CUjit_option *options, void **optionValues),                              \
    (module, image, numOptions, options, optionValues))                        \
  X(ModuleUnload, (CUmodule hmod), (hmod))                                     \
  X(ModuleGetFunction, (CUfunction * hfunc, CUmodule hmod, const char *name),  \
    (hfunc, hmod, name))                                                       \
  X(MemAlloc, (CUdeviceptr * dptr, size_t bytesize), (dptr, bytesize))         \
  X(MemFree, (CUdeviceptr dptr), (dptr))                                       \
  X(MemGetInfo, (size_t *free, size_t *total), (free, total))                  \
  X(MemsetD8, (CUdeviceptr dstDevice, unsigned char uc, size_t N),             \
    (dstDevice, uc, N))                                                        \
  X(MemcpyHtoD,                                                                \
    (CUdeviceptr dstDevice, const void *srcHost, size_t ByteCount),            \
    (dstDevice, srcHost, ByteCount))                                           \
  X(MemcpyDtoH, (void *dstHost, CUdeviceptr srcDevice, size_t ByteCount),      \
    (dstHost, srcDevice, ByteCount))                                           \
  X(MemcpyDtoD,                                                                \
    (CUdeviceptr dstDevice, CUdeviceptr srcDevice, size_t ByteCount),          \
    (dstDevice, srcDevice, ByteCount))                                         \
  X(MemcpyDtoHAsync,                                                           \
    (void *dstHost, CUdeviceptr srcDevice, size_t ByteCount,                   \
     CUstream hStream),                                                        \
    (dstHost, srcDevice, ByteCount, hStream))                                  \
  X(MemcpyDtoDAsync,                                                           \
    (CUdeviceptr dstDevice, CUdeviceptr srcDevice, size_t ByteCount,           \
     CUstream hStream),                                                        \
    (dstDevice, srcDevice, ByteCount, hStream))                                \
  X(StreamCreate, (CUstream * phStream, unsigned int Flags),                   \
    (phStream, Flags))                                                         \
  X(StreamDestroy, (CUstream hStream), (hStream))                              \
  X(StreamSynchronize, (CUstream hStream), (hStream))                          \
  X(StreamWaitEvent, (CUstream hStream, CUevent hEvent, unsigned int Flags),   \
    (hStream, hEvent, Flags))                                                  \
  X(EventCreate, (CUevent * phEvent, unsigned int Flags), (phEvent, Flags))    \
  X(EventDestroy, (CUevent hEvent), (hEvent))                                  \
  X(EventRecord, (CUevent hEvent, CUstream hStream), (hEvent, hStream))        \
  X(EventSynchronize, (CUevent hEvent), (hEvent))                              \
  X(LaunchKernel,                                                              \
    (CUfunction f, unsigned int gridDimX, unsigned int gridDimY,               \
     unsigned int gridDimZ, unsigned int blockDimX, unsigned int blockDimY,    \
     unsigned int blockDimZ, unsigned int sharedMemBytes, CUstream hStream,    \
     void **kernelParams, void **extra),                                       \
    (f, gridDimX, gridDimY, gridDimZ, blockDimX, blockDimY, blockDimZ,         \
     sharedMemBytes, hStream, kernelParams, extra))                            \
  X(GetErrorName, (CUresult error, const char **pStr), (error, pStr))          \
  X(GetErrorString, (CUresult error, const char **pStr), (error, pStr))

// Loaded function pointers
struct {
#define DECLARE_DRIVER_API(Name, Params, Args) CUresult(CUDAAPI *Name) Params{};
  CUDA_DRIVER_APIS(DECLARE_DRIVER_API)
#undef DECLARE_DRIVER_API
} cu;

bool cudaLoaded{false};
std::string unavailableReason{"CUDA driver has not been loaded yet"};

template <typename FPtr>
bool resolveDriver(FPtr &ptr, CuGetProcAddressFn getProcAddress,
                   const char *sym) {
  void *rawPtr{};
  auto result =
      getProcAddress(sym, &rawPtr, CUDA_VERSION, CU_GET_PROC_ADDRESS_DEFAULT);
  ptr = reinterpret_cast<FPtr>(rawPtr);
  return result == CUDA_SUCCESS && ptr != nullptr;
}

void loadCudaDriver() {
  // Load CUDA driver library
  LibHandle cudaLib{};
#ifdef _WIN32
  cudaLib = loadLib("nvcuda.dll");
#else
  cudaLib = loadLib("libcuda.so.1");
  if (cudaLib == nullptr) {
    cudaLib = loadLib("libcuda.so");
  }
#endif
  if (cudaLib == nullptr) {
    unavailableReason = "NVIDIA driver library not found";
    SPDLOG_INFO("CUDA unavailable: {}", unavailableReason);
    return;
  }

  // Resolve cuGetProcAddress itself via dlsym (the only function we dlsym)
  auto getProcAddress = reinterpret_cast<CuGetProcAddressFn>(
      loadSym(cudaLib, "cuGetProcAddress"));
  if (getProcAddress == nullptr) {
    unavailableReason =
        "cuGetProcAddress not found (NVIDIA driver may be too old)";
    SPDLOG_INFO("CUDA unavailable: {}", unavailableReason);
    return;
  }

  // Resolve all driver API functions via cuGetProcAddress
  bool ok = true;
#define RESOLVE_DRIVER_API(Name, Params, Args)                                 \
  ok = ok && resolveDriver(cu.Name, getProcAddress, "cu" #Name);
  CUDA_DRIVER_APIS(RESOLVE_DRIVER_API)
#undef RESOLVE_DRIVER_API

  if (!ok) {
    unavailableReason = "Failed to resolve CUDA driver API symbols";
    SPDLOG_INFO("CUDA unavailable: {}", unavailableReason);
    return;
  }

  cudaLoaded = true;
  unavailableReason.clear();
  SPDLOG_INFO("CUDA driver loaded successfully");
}

std::once_flag loadFlag;

void ensureLoaded() { std::call_once(loadFlag, loadCudaDriver); }

} // namespace

// Public API
namespace sme::simulate {

bool isCudaAvailable() {
  ensureLoaded();
  return cudaLoaded;
}

const std::string &cudaUnavailableReason() {
  ensureLoaded();
  return unavailableReason;
}

std::optional<CudaMemoryInfo> getCudaMemoryInfo() {
  ensureLoaded();
  if (!cudaLoaded) {
    return std::nullopt;
  }
  if (cu.Init(0) != CUDA_SUCCESS) {
    return std::nullopt;
  }

  int nDevices{};
  if (cu.DeviceGetCount(&nDevices) != CUDA_SUCCESS || nDevices < 1) {
    return std::nullopt;
  }

  CUdevice device{};
  if (cu.DeviceGet(&device, 0) != CUDA_SUCCESS) {
    return std::nullopt;
  }

  std::size_t totalBytes{};
  if (cu.DeviceTotalMem(&totalBytes, device) != CUDA_SUCCESS ||
      totalBytes == 0) {
    return std::nullopt;
  }

  CUcontext currentContext{};
  if (cu.CtxGetCurrent(&currentContext) != CUDA_SUCCESS) {
    return std::nullopt;
  }

  CUcontext temporaryContext{};
  const bool createdTemporaryContext = currentContext == nullptr;
  if (createdTemporaryContext &&
      cu.CtxCreate(&temporaryContext, nullptr, 0, device) != CUDA_SUCCESS) {
    return std::nullopt;
  }

  std::size_t availableBytes{};
  std::size_t contextTotalBytes{};
  const auto memInfoResult = cu.MemGetInfo(&availableBytes, &contextTotalBytes);

  if (createdTemporaryContext) {
    cu.CtxDestroy(temporaryContext);
  }

  if (memInfoResult != CUDA_SUCCESS) {
    return std::nullopt;
  }

  if (contextTotalBytes != 0) {
    totalBytes = std::min(totalBytes, contextTotalBytes);
  }
  availableBytes = std::min(availableBytes, totalBytes);
  return CudaMemoryInfo{totalBytes, availableBytes};
}

} // namespace sme::simulate

// ---------------------------------------------------------------------------
// CUDA Driver API stub functions
// These use the public, unversioned API spellings; cuda.h macro expansion maps
// them to the ABI-specific exported symbols for the active toolkit.
// Each stub calls ensureLoaded() and guards against null function pointers,
// so they are safe to call even if CUDA is unavailable (e.g. if the
// statically-linked NVRTC calls driver functions internally).
// ---------------------------------------------------------------------------

// clang-format off
#define GUARD(ptr) do { ensureLoaded(); if ((ptr) == nullptr) return CUDA_ERROR_NOT_INITIALIZED; } while(0)
// clang-format on

#define DEFINE_DRIVER_STUB(Name, Params, Args)                                 \
  CUresult CUDAAPI cu##Name Params {                                           \
    GUARD(cu.Name);                                                            \
    return cu.Name Args;                                                       \
  }

CUDA_DRIVER_APIS(DEFINE_DRIVER_STUB)

#undef DEFINE_DRIVER_STUB
#undef CUDA_DRIVER_APIS

#undef GUARD
