#include "cudapixelsim.hpp"
#include <stdexcept>

namespace sme::simulate::detail {

std::string makeCudaKernelSource(const std::vector<std::string> &,
                                 const std::vector<std::string> &, bool) {
  throw std::runtime_error(
      "CUDA pixel backend is not available on this platform");
}

std::string makeNvrtcCompileFailureMessage(std::string_view context,
                                           std::string_view error,
                                           std::string_view compileLog) {
  return std::string(context) + ": " + std::string(error) +
         "\nNVRTC compile log:\n" + std::string(compileLog);
}

} // namespace sme::simulate::detail

namespace sme::simulate {

namespace {

const std::vector<double> &emptyValues() {
  static const std::vector<double> values;
  return values;
}

const common::ImageStack &emptyImages() {
  static const common::ImageStack images;
  return images;
}

} // namespace

struct CudaPixelSim::Impl {};

CudaPixelSim::CudaPixelSim(const model::Model &sbmlDoc,
                           const std::vector<std::string> &,
                           const std::vector<std::vector<std::string>> &,
                           const std::map<std::string, double, std::less<>> &)
    : doc(sbmlDoc),
      currentErrorMessage(
          "CUDA pixel backend is not available on this platform") {}

CudaPixelSim::~CudaPixelSim() = default;

std::size_t CudaPixelSim::run(double, double, const std::function<bool()> &) {
  return 0;
}

const std::vector<double> &CudaPixelSim::getConcentrations(std::size_t) const {
  return emptyValues();
}

std::size_t CudaPixelSim::getConcentrationPadding() const { return 0; }

const std::vector<double> &CudaPixelSim::getDcdt(std::size_t) const {
  return emptyValues();
}

double CudaPixelSim::getLowerOrderConcentration(std::size_t, std::size_t,
                                                std::size_t) const {
  return 0.0;
}

const std::string &CudaPixelSim::errorMessage() const {
  return currentErrorMessage;
}

const common::ImageStack &CudaPixelSim::errorImages() const {
  return emptyImages();
}

void CudaPixelSim::setStopRequested(bool stop) { stopRequested.store(stop); }

bool CudaPixelSim::getStopRequested() const { return stopRequested.load(); }

void CudaPixelSim::setCurrentErrormessage(const std::string &msg) {
  currentErrorMessage = msg;
}

} // namespace sme::simulate
