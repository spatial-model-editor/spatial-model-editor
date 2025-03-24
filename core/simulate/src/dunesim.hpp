//  - DuneSim: wrapper around dune-copasi library

#pragma once

#include "basesim.hpp"
#include "sme/image_stack.hpp"
#include "sme/simulate_options.hpp"
#include "sme/voxel.hpp"
#include <QPointF>
#include <QSize>
#include <array>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace geometry {
class Compartment;
}

namespace simulate {

class DuneConverter;

template <int DuneDimensions> class DuneImpl;

class DuneSim : public BaseSim {
protected:
  std::unique_ptr<DuneImpl<2>> pDuneImpl2d;
  std::unique_ptr<DuneImpl<3>> pDuneImpl3d;
  std::string currentErrorMessage{};
  common::ImageStack currentErrorImages{};

public:
  explicit DuneSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  virtual ~DuneSim() override;
  virtual std::size_t
  run(double time, double timeout_ms,
      const std::function<bool()> &stopRunningCallback) override;
  [[nodiscard]] const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  [[nodiscard]] std::size_t getConcentrationPadding() const override;
  [[nodiscard]] const std::string &errorMessage() const override;
  [[nodiscard]] const common::ImageStack &errorImages() const override;
  void setStopRequested(bool stop) override;
};

} // namespace simulate

} // namespace sme
