#include "geometry_analytic.hpp"
#include "geometry_sampled_field.hpp"
#include "logger.hpp"
#include "sbml_math.hpp"
#include "sbml_utils.hpp"
#include "utils.hpp"
#include <QImage>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace model {

static const libsbml::AnalyticGeometry *
getAnalyticGeometry(const libsbml::Geometry *geom) {
  if (geom == nullptr) {
    return nullptr;
  }
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    if (auto *def = geom->getGeometryDefinition(i);
        def->getIsActive() && def->isAnalyticGeometry()) {
      return static_cast<const libsbml::AnalyticGeometry *>(def);
    }
  }
  return nullptr;
}

static std::vector<
    std::pair<const libsbml::Compartment *, const libsbml::AnalyticVolume *>>
getCompartmentsAndAnalyticVolumes(
    const libsbml::AnalyticGeometry *analyticGeom) {
  std::vector<
      std::pair<const libsbml::Compartment *, const libsbml::AnalyticVolume *>>
      v;
  const auto *model = analyticGeom->getModel();
  v.reserve(static_cast<std::size_t>(model->getNumCompartments()));
  for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
    const auto *comp = model->getCompartment(i);
    if (const auto *scp =
            static_cast<const libsbml::SpatialCompartmentPlugin *>(
                comp->getPlugin("spatial"));
        scp->isSetCompartmentMapping()) {
      const std::string &domainTypeId =
          scp->getCompartmentMapping()->getDomainType();
      if (const auto *analyticVol =
              analyticGeom->getAnalyticVolumeByDomainType(domainTypeId);
          analyticVol != nullptr) {
        SPDLOG_INFO("Compartment: {}", comp->getId());
        SPDLOG_INFO("  - DomainType: {}", domainTypeId);
        SPDLOG_INFO("  - AnalyticVolume: {}", analyticVol->getId());
        SPDLOG_INFO("  - Ordinal: {}", analyticVol->getOrdinal());
        v.emplace_back(comp, analyticVol);
      }
    }
  }
  // sort in descending order of ordinal
  // this gives them in the order in which they should be processed
  // when a pixel is contained in multiple volumes, the highest ordinal takes
  // precedence
  std::sort(v.begin(), v.end(), [](const auto &a, const auto &b) {
    return a.second->getOrdinal() > b.second->getOrdinal();
  });
  return v;
}

static QPointF toPhysicalPoint(int ix, int iy, const QSize &imgSize,
                               const QPointF &origin, const QSizeF &size) {
  QPointF p;
  double dx = static_cast<double>(ix) / static_cast<double>(imgSize.width());
  double dy = static_cast<double>(iy) / static_cast<double>(imgSize.height());
  p.setX(origin.x() + dx * size.width());
  p.setY(origin.y() + dy * size.height());
  return p;
}

static QPoint getMatchingColourPixel(const QImage &image, QRgb col) {
  QPoint p;
  for (int ix = 0; ix < image.width(); ++ix) {
    for (int iy = 0; iy < image.height(); ++iy) {
      if (image.pixel(ix, iy) == col) {
        p = QPoint(ix, iy);
      }
    }
  }
  return p;
}

static void improvePixelY(QPoint &p, const QImage &image) {
  SPDLOG_TRACE("({},{})", p.x(), p.y());
  QRgb col = image.pixel(p);
  // go as far as possible in each direction without changing colour
  int yp = 0;
  while (p.y() + yp < image.height() && image.pixel(p.x(), p.y() + yp) == col) {
    ++yp;
  }
  int ym = 0;
  while (p.y() - ym >= 0 && image.pixel(p.x(), p.y() - ym) == col) {
    ++ym;
  }
  // move to midpoint along this axis
  p.ry() += (yp - ym) / 2;
  SPDLOG_TRACE("  -> ({},{})", p.x(), p.y());
}

static void improvePixelX(QPoint &p, const QImage &image) {
  SPDLOG_TRACE("({},{})", p.x(), p.y());
  // go as far as possible in each direction without changing colour
  QRgb col = image.pixel(p);
  int xp = 0;
  while (p.x() + xp < image.width() && image.pixel(p.x() + xp, p.y()) == col) {
    ++xp;
  }
  int xm = 0;
  while (p.x() - xm >= 0 && image.pixel(p.x() - xm, p.y()) == col) {
    ++xm;
  }
  // move to midpoint along this axis
  p.rx() += (xp - xm) / 2;
  SPDLOG_TRACE("  -> ({},{})", p.x(), p.y());
}

static QPoint findInteriorPixel(const QImage &image, QRgb col) {
  auto p = getMatchingColourPixel(image, col);
  improvePixelX(p, image);
  improvePixelY(p, image);
  return p;
}

void setInteriorPoint(libsbml::Model *model, const std::string &domainType,
                      const QPointF &point) {
  auto *geom = getOrCreateGeometry(model);
  auto *domain = geom->getDomainByDomainType(domainType);
  SPDLOG_INFO("  - domain: {}", domain->getId());
  auto *interiorPoint = domain->getInteriorPoint(0);
  if (interiorPoint == nullptr) {
    SPDLOG_INFO("  - creating new interior point");
    interiorPoint = domain->createInteriorPoint();
  }
  interiorPoint->setCoord1(point.x());
  interiorPoint->setCoord2(point.y());
}

GeometrySampledField importGeometryFromAnalyticGeometry(libsbml::Model *model,
                                                        const QPointF &origin,
                                                        const QSizeF &size) {
  GeometrySampledField gsf;
  QRgb nullColour{qRgb(0, 0, 0)};
  QSize imageSize(200, 200);
  if (size.width() > size.height()) {
    imageSize.setHeight(static_cast<int>(
        static_cast<double>(imageSize.width()) * size.height() / size.width()));
  } else {
    imageSize.setWidth(
        static_cast<int>(static_cast<double>(imageSize.height()) *
                         size.width() / size.height()));
  }
  gsf.image = QImage(imageSize, QImage::Format_RGB32);
  gsf.image.fill(nullColour);
  const auto *geom = getGeometry(model);
  if (geom == nullptr) {
    return {};
  }
  const auto *analyticGeometry = getAnalyticGeometry(geom);
  if (analyticGeometry == nullptr) {
    return {};
  }
  auto compVols = getCompartmentsAndAnalyticVolumes(analyticGeometry);
  std::size_t iComp = 0;
  std::map<const std::string, std::pair<double, bool>> varsMap;
  const auto *xparam = getSpatialCoordinateParam(
      model, libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  if (xparam == nullptr) {
    SPDLOG_ERROR("No parameter for x coordinate in model");
    return {};
  }
  std::string xCoord{xparam->getId()};
  varsMap[xCoord] = {0, false};
  const auto *yparam = getSpatialCoordinateParam(
      model, libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  if (yparam == nullptr) {
    SPDLOG_ERROR("No parameter for y coordinate in model");
    return {};
  }
  std::string yCoord{yparam->getId()};
  varsMap[yCoord] = {0, false};
  if (const auto *zparam = getSpatialCoordinateParam(
          model, libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z);
      zparam != nullptr) {
    varsMap[zparam->getId()] = {0, false};
  }
  for (const auto &[comp, analyticVol] : compVols) {
    SPDLOG_INFO("Compartment: {}", comp->getId());
    SPDLOG_INFO("  - AnalyticVolume: {}", analyticVol->getId());
    SPDLOG_INFO("  - Ordinal: {}", analyticVol->getOrdinal());
    const auto *math = analyticVol->getMath();
    SPDLOG_INFO("  - Math: {}", mathASTtoString(math));
    auto col = utils::indexedColours()[iComp].rgb();
    ++iComp;
    SPDLOG_INFO("  - Colour: {:x}", col);
    std::size_t nPixels = 0;
    for (int ix = 0; ix < gsf.image.width(); ++ix) {
      for (int iy = 0; iy < gsf.image.height(); ++iy) {
        // we want y=0 in bottom of image, Qt puts it in top:
        int invertedYIndex = gsf.image.height() - 1 - iy;
        if (gsf.image.pixel(ix, invertedYIndex) == nullColour) {
          auto p = toPhysicalPoint(ix, iy, gsf.image.size(), origin, size);
          varsMap[xCoord].first = p.x();
          varsMap[yCoord].first = p.y();
          if (static_cast<int>(
                  evaluateMathAST(math, varsMap, geom->getModel())) != 0) {
            gsf.image.setPixel(ix, invertedYIndex, col);
            ++nPixels;
          }
        }
      }
    }
    SPDLOG_INFO("  - Pixels: {}", nPixels);
    if (nPixels > 0) {
      gsf.compartmentIdColourPairs.push_back({comp->getId(), col});
      auto interiorPixel = findInteriorPixel(gsf.image, col);
      auto interiorPoint = toPhysicalPoint(
          interiorPixel.x(), gsf.image.height() - 1 - interiorPixel.y(),
          gsf.image.size(), origin, size);
      SPDLOG_INFO("  - Interior point: ({},{})", interiorPoint.x(),
                  interiorPoint.y());
      setInteriorPoint(model, analyticVol->getDomainType(), interiorPoint);
    }
  }
  return gsf;
}

} // namespace model
