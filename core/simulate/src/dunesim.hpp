//  - DuneSim: wrapper around dune-copasi library

#pragma once

#include "basesim.hpp"
#include "sme/geometry_utils.hpp"
#include "sme/image_stack.hpp"
#include "sme/simulate_options.hpp"
#include "sme/utils.hpp"
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

namespace dune {
class DuneConverter;
}

namespace sme {

namespace model {
class Model;
}

namespace geometry {
class Compartment;
}

namespace simulate {

using PixelLocalPair = std::pair<std::size_t, std::array<double, 2>>;

class DuneImpl;

struct DuneSimCompartment {
  std::string name;
  std::size_t index;
  std::vector<std::size_t> speciesIndices;
  geometry::VoxelIndexer voxelIndexer;
  const geometry::Compartment *geometry;
  // pixels+dune local coords for each triangle
  std::vector<std::vector<PixelLocalPair>> pixels;
  // index of nearest valid pixel for any missing pixels
  std::vector<std::pair<std::size_t, std::size_t>> missingPixels;
  std::vector<double> concentration;
};

class DuneSim : public BaseSim {
private:
  std::unique_ptr<DuneImpl> pDuneImpl;
  std::vector<DuneSimCompartment> duneCompartments;
  // dimensions of model
  QSize geometryImageSize;
  double pixelSize;
  QPointF pixelOrigin;
  void initDuneSimCompartments(
      const std::vector<const geometry::Compartment *> &comps);
  void updatePixels();
  void updateSpeciesConcentrations();
  std::string currentErrorMessage{};
  common::ImageStack currentErrorImages{};
  double volOverL3;

public:
  explicit DuneSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  ~DuneSim() override;
  std::size_t run(double time, double timeout_ms,
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
