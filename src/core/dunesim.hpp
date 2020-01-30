//  - DuneSim: wrapper around dune-copasi library

#pragma once

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <array>
#include <memory>

#include "basesim.hpp"
#include "utils.hpp"

namespace sbml {
class SbmlDocWrapper;
}

namespace geometry {
class Compartment;
}

namespace dune {
class DuneConverter;
}

namespace sim {

using PixelLocalPair = std::pair<std::size_t, std::array<double, 2>>;

class DuneSim : public BaseSim {
 private:
  // Dune objects via pimpl to hide DUNE headers
  class DuneImpl;
  std::unique_ptr<DuneImpl> pDuneImpl;
  std::vector<std::string> compartmentDuneNames;
  // DUNE species index for each species
  std::vector<std::vector<std::size_t>> compartmentSpeciesIndex;
  // dimensions of model
  QSize geometryImageSize;
  double pixelSize;
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
  void initSpeciesIndices(const dune::DuneConverter &dc);
  void updatePixels();
  void updateSpeciesConcentrations();

 public:
  explicit DuneSim(const sbml::SbmlDocWrapper &sbmlDoc);
  ~DuneSim() override;
  void doTimestep(double t, double dt) override;
  const std::vector<double> &getConcentrations(
      std::size_t compartmentIndex) const override;
};

}  // namespace sim
