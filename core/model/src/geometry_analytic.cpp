#include "geometry_analytic.hpp"
#include "geometry_sampled_field.hpp"
#include "sbml_math.hpp"
#include "sbml_utils.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <QImage>
#include <algorithm>
#include <array>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

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

static geometry::VoxelF toPhysicalPoint(const geometry::Voxel &voxel,
                                        const geometry::VSize &imageSize,
                                        const geometry::VoxelF &physicalOrigin,
                                        const geometry::VSizeF &physicalSize) {
  auto physicalPoint{physicalOrigin};
  physicalPoint.p.rx() += physicalSize.width() *
                          static_cast<double>(voxel.p.x()) /
                          static_cast<double>(imageSize.width());
  physicalPoint.p.ry() += physicalSize.height() *
                          static_cast<double>(voxel.p.y()) /
                          static_cast<double>(imageSize.height());
  physicalPoint.z += physicalSize.depth() * static_cast<double>(voxel.z) /
                     static_cast<double>(imageSize.depth());
  return physicalPoint;
}

GeometrySampledField
importGeometryFromAnalyticGeometry(const libsbml::Model *model,
                                   const geometry::VoxelF &physicalOrigin,
                                   const geometry::VSizeF &physicalSize) {
  int nMax{100};
  double norm{common::max(std::array<double, 3>{
      physicalSize.width(), physicalSize.height(), physicalSize.depth()})};
  int nx{std::clamp(nMax * static_cast<int>(physicalSize.width() / norm), 1,
                    nMax)};
  int ny{std::clamp(nMax * static_cast<int>(physicalSize.height() / norm), 1,
                    nMax)};
  int nz{std::clamp(nMax * static_cast<int>(physicalSize.depth() / norm), 1,
                    nMax)};
  geometry::VSize imageSize{nx, ny, static_cast<std::size_t>(nz)};
  GeometrySampledField gsf;
  QRgb nullColour{qRgb(0, 0, 0)};
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
  const std::string xCoord{xparam->getId()};
  varsMap[xCoord] = {0, false};
  const auto *yparam = getSpatialCoordinateParam(
      model, libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  if (yparam == nullptr) {
    SPDLOG_ERROR("No parameter for y coordinate in model");
    return {};
  }
  const std::string yCoord{yparam->getId()};
  varsMap[yCoord] = {0, false};
  std::size_t nZ{1};
  std::string zCoord{};
  if (const auto *zparam{getSpatialCoordinateParam(
          model,
          libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Z)};
      zparam != nullptr) {
    zCoord = zparam->getId();
    varsMap[zCoord] = {0, false};
  }
  gsf.images = {nZ,
                {imageSize.width(), imageSize.height(), QImage::Format_RGB32}};
  for (auto &image : gsf.images) {
    image.fill(nullColour);
  }
  for (const auto &[comp, analyticVol] : compVols) {
    SPDLOG_INFO("Compartment: {}", comp->getId());
    SPDLOG_INFO("  - AnalyticVolume: {}", analyticVol->getId());
    SPDLOG_INFO("  - Ordinal: {}", analyticVol->getOrdinal());
    const auto *math = analyticVol->getMath();
    SPDLOG_INFO("  - Math: {}", mathASTtoString(math));
    auto col = common::indexedColours()[iComp].rgb();
    ++iComp;
    SPDLOG_INFO("  - Colour: {:x}", col);
    std::size_t nVoxels{0};
    for (std::size_t iz = 0; iz < nZ; ++iz) {
      auto &image{gsf.images[iz]};
      for (int ix = 0; ix < image.width(); ++ix) {
        for (int iy = 0; iy < image.height(); ++iy) {
          geometry::Voxel v{ix, iy, iz};
          // we want y=0 in bottom of image, Qt puts it in top:
          int invertedYIndex = image.height() - 1 - iy;
          if (image.pixel(ix, invertedYIndex) == nullColour) {
            auto physical{
                toPhysicalPoint(v, imageSize, physicalOrigin, physicalSize)};
            varsMap[xCoord].first = physical.p.x();
            varsMap[yCoord].first = physical.p.y();
            if (nZ > 1) {
              varsMap[zCoord].first = physical.z;
            }
            if (static_cast<int>(
                    evaluateMathAST(math, varsMap, geom->getModel())) != 0) {
              image.setPixel(ix, invertedYIndex, col);
              ++nVoxels;
            }
          }
        }
      }
    }
    SPDLOG_INFO("  - Voxels: {}", nVoxels);
    if (nVoxels > 0) {
      gsf.compartmentIdColourPairs.push_back({comp->getId(), col});
    }
  }
  return gsf;
}

} // namespace sme::model
