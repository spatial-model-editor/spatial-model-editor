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

namespace model {
class Model;
}

namespace geometry {
class Compartment;
}

namespace dune {
class DuneConverter;
}

namespace sim {

using PixelLocalPair = std::pair<std::size_t, std::array<double, 2>>;

class DuneImpl;

class DuneSim : public BaseSim {
private:
  // Dune objects via pimpl to hide DUNE headers
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
  IntegratorError errMax;
  double maxTimestep = std::numeric_limits<double>::max();
  std::string currentErrorMessage;
  std::size_t integratorOrder = 1;

public:
  explicit DuneSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
      std::size_t order = 1);
  ~DuneSim() override;
  virtual void setIntegrationOrder(std::size_t order) override;
  virtual std::size_t getIntegrationOrder() const override;
  virtual void setIntegratorError(const IntegratorError &err) override;
  virtual IntegratorError getIntegratorError() const override;
  virtual void setMaxDt(double maxDt) override;
  virtual double getMaxDt() const override;
  virtual void setMaxThreads([[maybe_unused]] std::size_t maxThreads) override;
  virtual std::size_t getMaxThreads() const override;
  std::size_t run(double time) override;
  const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  virtual std::string errorMessage() const override;
};

} // namespace sim
