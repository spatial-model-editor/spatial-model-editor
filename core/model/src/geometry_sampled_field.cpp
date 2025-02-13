#include "geometry_sampled_field.hpp"
#include "sbml_utils.hpp"
#include "sme/logger.hpp"
#include "sme/utils.hpp"
#include <QImage>
#include <memory>
#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

namespace sme::model {

libsbml::SampledFieldGeometry *
getOrCreateSampledFieldGeometry(libsbml::Geometry *geom) {
  auto *sfgeom{getSampledFieldGeometry(geom)};
  if (sfgeom == nullptr) {
    sfgeom = geom->createSampledFieldGeometry();
    sfgeom->setId("sampledFieldGeometry");
    sfgeom->setIsActive(true);
  }
  return sfgeom;
}

static std::pair<std::vector<const libsbml::Compartment *>,
                 std::vector<const libsbml::SampledVolume *>>
getCompartmentsAndSampledVolumes(const libsbml::SampledFieldGeometry *sfgeom) {
  std::pair<std::vector<const libsbml::Compartment *>,
            std::vector<const libsbml::SampledVolume *>>
      compartmentsAndSampledVolumes;
  auto &[compartments, sampledVolumes] = compartmentsAndSampledVolumes;
  const auto *model = sfgeom->getModel();
  sampledVolumes.reserve(static_cast<std::size_t>(model->getNumCompartments()));
  compartments.reserve(sampledVolumes.size());
  for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
    const auto *comp = model->getCompartment(i);
    if (const auto *scp =
            static_cast<const libsbml::SpatialCompartmentPlugin *>(
                comp->getPlugin("spatial"));
        scp->isSetCompartmentMapping()) {
      const std::string &domainTypeId =
          scp->getCompartmentMapping()->getDomainType();
      if (const auto *sfvol =
              sfgeom->getSampledVolumeByDomainType(domainTypeId);
          sfvol != nullptr) {
        SPDLOG_INFO("Compartment: {}", comp->getId());
        SPDLOG_INFO("  - DomainType: {}", domainTypeId);
        SPDLOG_INFO("  - SampledFieldVolume: {}", sfvol->getId());
        compartments.push_back(comp);
        sampledVolumes.push_back(sfvol);
      }
    }
  }
  return compartmentsAndSampledVolumes;
}

static std::vector<QImage>
makeEmptyImages(const libsbml::SampledField *sampledField) {
  std::vector<QImage> images;
  int xVals{sampledField->getNumSamples1()};
  int yVals{sampledField->getNumSamples2()};
  // todo: do we need to check this exists?
  int zVals{1};
  if (sampledField->isSetNumSamples3()) {
    zVals = sampledField->getNumSamples3();
  }
  SPDLOG_DEBUG("Found {}x{}x{} [xyz] sampled field", xVals, yVals, zVals);
  return {static_cast<std::size_t>(zVals),
          {xVals, yVals, QImage::Format_RGB32}};
}

static bool allSampledValuesSet(
    const std::vector<const libsbml::SampledVolume *> &sampledVolumes) {
  return std::ranges::all_of(sampledVolumes,
                             [](auto *s) { return s->isSetSampledValue(); });
}

static bool valuesAreAllQRgb(const std::vector<QRgb> &values) {
  return qAlpha(common::min(values)) == 255;
}

static bool isNativeSampledFieldFormat(
    const libsbml::SampledField *sampledField,
    const std::vector<const libsbml::SampledVolume *> &sampledVolumes) {
  if (sampledField->getDataType() !=
      libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32) {
    return false;
  }
  if (!allSampledValuesSet(sampledVolumes)) {
    return false;
  }
  if (auto values = common::stringToVector<QRgb>(sampledField->getSamples());
      !valuesAreAllQRgb(values)) {
    return false;
  }
  return true;
}

template <typename T>
static void
setImportedColorsToImages(std::vector<QImage> &imgs,
                          const std::vector<T> &values,
                          const std::vector<QRgb> &importedSampledFieldColors) {
  auto iter = values.begin();
  for (auto &img : imgs) {
    for (int y = 0; y < img.height(); ++y) {
      for (int x = 0; x < img.width(); ++x) {
        img.setPixel(
            x, img.height() - 1 - y,
            importedSampledFieldColors[static_cast<std::size_t>(*iter)]);
        ++iter;
      }
    }
  }
}

static void setPixelsToValues(std::vector<QImage> &imgs,
                              const std::vector<QRgb> &values) {
  auto iter = values.begin();
  for (auto &img : imgs) {
    for (int y = 0; y < img.height(); ++y) {
      for (int x = 0; x < img.width(); ++x) {
        img.setPixel(x, img.height() - 1 - y, *iter);
        ++iter;
      }
    }
  }
}

static std::vector<QRgb> setImagePixelsNative(
    std::vector<QImage> &imgs, const libsbml::SampledField *sampledField,
    const std::vector<const libsbml::SampledVolume *> &sampledVolumes) {
  std::vector<QRgb> colors;
  colors.reserve(sampledVolumes.size());
  auto values = common::stringToVector<QRgb>(sampledField->getSamples());
  SPDLOG_DEBUG("Importing sampled field of {} samples of type QRgb",
               values.size());
  if (static_cast<int>(values.size()) != sampledField->getSamplesLength()) {
    SPDLOG_WARN("Number of samples {} doesn't match SamplesLength {}",
                values.size(), sampledField->getSamplesLength());
  }
  setPixelsToValues(imgs, values);
  for (const auto *sampledVolume : sampledVolumes) {
    colors.push_back(static_cast<QRgb>(sampledVolume->getSampledValue()));
  }
  return colors;
}

template <typename T>
static std::vector<bool>
getMatchingSampledValues(const std::vector<T> &values,
                         const libsbml::SampledVolume *sfvol) {
  std::vector<bool> matches(values.size(), false);
  if (sfvol->isSetSampledValue()) {
    T sv = static_cast<T>(sfvol->getSampledValue());
    // cannot use c++20 ranges with vector<bool> as it doesn't satisfy
    // indirectly_writable: should be fixed in c++23
    std::transform(values.cbegin(), values.cend(), matches.begin(),
                   [sv](T v) -> bool { return v == sv; });
  } else if (sfvol->isSetMinValue() && sfvol->isSetMaxValue()) {
    double min{sfvol->getMinValue()};
    double max{sfvol->getMaxValue()};
    std::transform(values.begin(), values.end(), matches.begin(),
                   [min, max](T v) {
                     auto vAsDouble{static_cast<double>(v)};
                     return vAsDouble >= min && vAsDouble < max;
                   });
  }
  return matches;
}

static void setMatchingPixelsToColor(std::vector<QImage> &imgs,
                                     const std::vector<bool> &matches,
                                     QRgb color) {
  auto iter = matches.begin();
  for (auto &img : imgs) {
    for (int y = 0; y < img.height(); ++y) {
      for (int x = 0; x < img.width(); ++x) {
        if (*iter) {
          img.setPixel(x, img.height() - 1 - y, color);
        }
        ++iter;
      }
    }
  }
}

template <typename T>
static std::vector<QRgb> setImagePixels(
    std::vector<QImage> &imgs, const libsbml::SampledField *sampledField,
    const std::vector<const libsbml::SampledVolume *> &sampledVolumes,
    const std::vector<QRgb> &importedSampledFieldColors = {}) {
  std::vector<QRgb> colors(sampledVolumes.size(), 0);
  for (auto &img : imgs) {
    img.fill(qRgb(0, 0, 0));
  }
  std::vector<T> values;
  if (sampledField->getCompression() ==
      libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_DEFLATED) {
    // for compressed fields the string returned by getSamples() is compressed,
    // so instead we get the field as doubles and then cast to the correct type.
    std::vector<double> dbls;
    sampledField->getSamples(dbls);
    values.reserve(dbls.size());
    for (auto dbl : dbls) {
      values.push_back(static_cast<T>(dbl));
    }
  } else {
    values = common::stringToVector<T>(sampledField->getSamples());
  }
  SPDLOG_DEBUG("Importing sampled field of {} samples of type {}",
               values.size(), common::decltypeStr<T>());
  if (common::isItIndexes(values, importedSampledFieldColors.size())) {
    setImportedColorsToImages(imgs, values, importedSampledFieldColors);
  }
  if (static_cast<int>(values.size()) != sampledField->getSamplesLength()) {
    SPDLOG_WARN("Number of samples {} doesn't match SamplesLength {}",
                values.size(), sampledField->getSamplesLength());
  }
  std::size_t iCol = 0;
  auto iter = colors.begin();
  for (const auto *sampledVolume : sampledVolumes) {
    auto matches = getMatchingSampledValues(values, sampledVolume);
    if (std::ranges::find(matches, true) != matches.cend()) {
      auto col{common::indexedColors()[iCol].rgb()};
      SPDLOG_DEBUG("Color {} is {}", iCol, col);
      auto sampledValue{
          static_cast<std::size_t>(sampledVolume->getSampledValue())};
      if (sampledValue < importedSampledFieldColors.size()) {
        col = importedSampledFieldColors[sampledValue];
        SPDLOG_DEBUG("  -> Using importedSampledFieldColor {}", col);
      }
      SPDLOG_DEBUG("  {}/{} -> color {:x}", sampledVolume->getId(),
                   sampledVolume->getDomainType(), col);
      setMatchingPixelsToColor(imgs, matches, col);
      *iter = col;
      ++iCol;
    }
    ++iter;
  }
  return colors;
}

static std::vector<std::pair<std::string, QRgb>> getCompartmentIdAndColors(
    const std::vector<const libsbml::Compartment *> &compartments,
    const std::vector<QRgb> &compartmentColors) {
  std::vector<std::pair<std::string, QRgb>> compartmentIdAndColors;
  compartmentIdAndColors.reserve(compartments.size());
  for (std::size_t i = 0; i < compartments.size(); ++i) {
    if (auto color = compartmentColors[i]; color != 0) {
      compartmentIdAndColors.push_back({compartments[i]->getId(), color});
    }
  }
  return compartmentIdAndColors;
}

GeometrySampledField importGeometryFromSampledField(
    const libsbml::Geometry *geom,
    const std::vector<QRgb> &importedSampledFieldColors) {
  GeometrySampledField gsf;
  if (geom == nullptr) {
    return gsf;
  }
  const auto *sfgeom = getSampledFieldGeometry(geom);
  if (sfgeom == nullptr) {
    return gsf;
  }
  auto [compartments, sampledVolumes] =
      getCompartmentsAndSampledVolumes(sfgeom);
  const auto *sampledField = geom->getSampledField(sfgeom->getSampledField());
  gsf.images = makeEmptyImages(sampledField);
  std::vector<QRgb> compartmentColors;
  auto dataType = sampledField->getDataType();
  SPDLOG_DEBUG("importedSampledFieldColors {}", importedSampledFieldColors);

  if (isNativeSampledFieldFormat(sampledField, sampledVolumes)) {
    compartmentColors =
        setImagePixelsNative(gsf.images, sampledField, sampledVolumes);
  } else if (dataType == libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE) {
    compartmentColors = setImagePixels<double>(
        gsf.images, sampledField, sampledVolumes, importedSampledFieldColors);
  } else if (dataType == libsbml::DataKind_t::SPATIAL_DATAKIND_FLOAT) {
    compartmentColors = setImagePixels<float>(
        gsf.images, sampledField, sampledVolumes, importedSampledFieldColors);
  } else if (dataType == libsbml::DataKind_t::SPATIAL_DATAKIND_INT) {
    compartmentColors = setImagePixels<int>(
        gsf.images, sampledField, sampledVolumes, importedSampledFieldColors);
  } else {
    // remaining dataTypes are all unsigned ints of various sizes
    compartmentColors = setImagePixels<std::size_t>(
        gsf.images, sampledField, sampledVolumes, importedSampledFieldColors);
  }
  gsf.compartmentIdColorPairs =
      getCompartmentIdAndColors(compartments, compartmentColors);

  return gsf;
}

void exportSampledFieldGeometry(libsbml::Geometry *geom,
                                const common::ImageStack &compartmentImages) {
  auto *sfgeom = getOrCreateSampledFieldGeometry(geom);
  libsbml::SampledField *sf = geom->getSampledField(sfgeom->getSampledField());
  if (sf == nullptr) {
    sf = geom->createSampledField();
  }
  sf->setId("geometryImage");
  sf->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32);
  sf->setInterpolationType(
      libsbml::InterpolationKind_t::SPATIAL_INTERPOLATIONKIND_NEARESTNEIGHBOR);
  sf->setCompression(
      libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
  sfgeom->setSampledField(sf->getId());

  int nx{compartmentImages[0].width()};
  int ny{compartmentImages[0].height()};
  int nz{static_cast<int>(compartmentImages.volume().depth())};
  SPDLOG_INFO("Writing {}x{}x{} geometry image to SampledField {}", nx, ny, nz,
              sfgeom->getSampledField());
  sf->setNumSamples1(nx);
  sf->setNumSamples2(ny);
  sf->setNumSamples3(nz);
  sf->setSamplesLength(nx * ny * nz);

  std::vector<QRgb> samples;
  samples.reserve(static_cast<std::size_t>(nx) * static_cast<std::size_t>(ny) *
                  static_cast<std::size_t>(nz));
  // convert 3d pixmap into array of uints
  // NOTE: order of samples is:
  // [ (x=0,y=0,z=0), (x=1,y=0,z=0), ..., (x=0,y=1,z=0), (x=1,y=1,z=0), ... ]
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  for (std::size_t z = 0; z < static_cast<std::size_t>(nz); ++z) {
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        samples.push_back(
            static_cast<QRgb>(compartmentImages[z].pixelIndex(x, ny - 1 - y)));
      }
    }
  }
  sf->setSamples(common::vectorToString(samples));
  SPDLOG_INFO("SampledField '{}': assigned {}x{}x{} array of total length {}",
              sf->getId(), sf->getNumSamples1(), sf->getNumSamples2(),
              sf->getNumSamples3(), sf->getSamplesLength());
}

} // namespace sme::model
