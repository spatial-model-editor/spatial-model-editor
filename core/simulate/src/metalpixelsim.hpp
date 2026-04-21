// Metal pixel simulator PoC

#pragma once

#include "pixelsim_base.hpp"
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace MTL {
class ComputeCommandEncoder;
}

namespace sme {

namespace model {
class Model;
}

namespace simulate {

namespace detail {
[[nodiscard]] std::string
makeMetalKernelSource(const std::vector<std::string> &variables,
                      const std::vector<std::string> &expressions);
[[nodiscard]] std::string
makeMetalCompileFailureMessage(std::string_view context,
                               std::string_view error);
} // namespace detail

class MetalPixelSim : public PixelSimBase {
private:
  struct Impl;
  std::unique_ptr<Impl> impl;
  const model::Model &doc;
  void encodeEvaluateDcdt(MTL::ComputeCommandEncoder *encoder);
  void encodeRk101Update(MTL::ComputeCommandEncoder *encoder, double dt);
  void encodeClampNegative(MTL::ComputeCommandEncoder *encoder);
  void encodeRk212Substep1(MTL::ComputeCommandEncoder *encoder, double dt);
  void encodeRk212Substep2(MTL::ComputeCommandEncoder *encoder, double dt);
  void encodeRkInit(MTL::ComputeCommandEncoder *encoder);
  void encodeRkSubstep(MTL::ComputeCommandEncoder *encoder, double dt,
                       double g1, double g2, double g3, double beta,
                       double delta);
  void encodeRkFinalise(MTL::ComputeCommandEncoder *encoder, double cFactor,
                        double s2Factor, double s3Factor);
  void encodeRk212Error(MTL::ComputeCommandEncoder *encoder);
  void encodeRk212Timestep(MTL::ComputeCommandEncoder *encoder, double dt);
  void encodeRk323Timestep(MTL::ComputeCommandEncoder *encoder, double dt);
  void encodeRk435Timestep(MTL::ComputeCommandEncoder *encoder, double dt);
  [[nodiscard]] PixelIntegratorError readRk212ErrorResult() const;
  void restorePreStepConcentrations();
  void downloadStateToHost();
  void waitForDownload();
  void ensureDownloadComplete();
  [[nodiscard]] double doRK101(double dt);
  void doRK212(double dt);
  void doRK323(double dt);
  void doRK435(double dt);
  [[nodiscard]] double doRKAdaptive(double dtMax);

public:
  explicit MetalPixelSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  ~MetalPixelSim() override;

  std::size_t run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;

  [[nodiscard]] const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  [[nodiscard]] std::size_t getConcentrationPadding() const override;
  [[nodiscard]] const std::vector<double> &
  getDcdt(std::size_t compartmentIndex) const;
  [[nodiscard]] double getLowerOrderConcentration(std::size_t compartmentIndex,
                                                  std::size_t speciesIndex,
                                                  std::size_t pixelIndex) const;
};

} // namespace simulate

} // namespace sme
