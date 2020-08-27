//  - DuneSim: wrapper around dune-copasi library

#pragma once

#include "basesim.hpp"
#include "simulate_options.hpp"
#include "utils.hpp"
#include <QPointF>
#include <QSize>
#include <array>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace model {
class Model;
}

namespace geometry {
class Compartment;
}

namespace dune {
class DuneConverter;
}

namespace simulate {

using PixelLocalPair = std::pair<std::size_t, std::array<double, 2>>;

class DuneImpl;

class DuneSim : public BaseSim {
private:
  std::unique_ptr<DuneImpl> pDuneImpl;
  std::vector<std::string> compartmentDuneNames;
  // DUNE species index for each species
  std::vector<std::vector<std::size_t>> compartmentSpeciesIndex;
  // dimensions of model
  QSize geometryImageSize;
  double pixelSize;
  QPointF pixelOrigin;
  // map from pixel QPoint to ix index for each compartment
  std::vector<utils::QPointIndexer> compartmentPointIndex;
  std::vector<const geometry::Compartment *> compartmentGeometry;
  // pixels+dune local coords for each triangle
  // compartment::triangle::pixels+local-coord
  std::vector<std::vector<std::vector<PixelLocalPair>>> pixels;
  // missing pixels: use concentration from neighbouring pixel
  std::vector<std::vector<std::pair<std::size_t, std::size_t>>> missingPixels;
  // concentrations for each pixel in compartments
  std::vector<std::vector<double>> concentration;
  void initCompartmentNames();
  void initSpeciesIndices();
  void updatePixels();
  void updateSpeciesConcentrations();
  std::string currentErrorMessage;
  DuneIntegratorType integrator;
  double dt;

public:
  explicit DuneSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      const DuneOptions &options = {});
  ~DuneSim() override;
  std::size_t run(double time) override;
  const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  virtual const std::string& errorMessage() const override;
};

} // namespace simulate
