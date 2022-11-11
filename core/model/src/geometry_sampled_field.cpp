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

static QImage makeEmptyImage(const libsbml::SampledField *sampledField) {
  int xVals = sampledField->getNumSamples1();
  int yVals = sampledField->getNumSamples2();
  SPDLOG_DEBUG("Found {}x{} sampled field", xVals, yVals);
  return QImage(xVals, yVals, QImage::Format_RGB32);
}

static bool allSampledValuesSet(
    const std::vector<const libsbml::SampledVolume *> &sampledVolumes) {
  return std::all_of(sampledVolumes.cbegin(), sampledVolumes.cend(),
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
static void setImportedColoursToImage(
    QImage &img, const std::vector<T> &values,
    const std::vector<QRgb> &importedSampledFieldColours) {
  auto iter = values.begin();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      img.setPixel(x, img.height() - 1 - y,
                   importedSampledFieldColours[(int)*iter]);
      ++iter;
    }
  }
}

static void setPixelsToValues(QImage &img, const std::vector<QRgb> &values) {
  auto iter = values.begin();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      img.setPixel(x, img.height() - 1 - y, *iter);
      ++iter;
    }
  }
}

static std::vector<QRgb> setImagePixelsNative(
    QImage &img, const libsbml::SampledField *sampledField,
    const std::vector<const libsbml::SampledVolume *> &sampledVolumes) {
  std::vector<QRgb> colours;
  colours.reserve(sampledVolumes.size());
  auto values = common::stringToVector<QRgb>(sampledField->getSamples());
  SPDLOG_DEBUG("Importing sampled field of {} samples of type QRgb",
               values.size());
  if (static_cast<int>(values.size()) != sampledField->getSamplesLength()) {
    SPDLOG_WARN("Number of samples {} doesn't match SamplesLength {}",
                values.size(), sampledField->getSamplesLength());
  }
  setPixelsToValues(img, values);
  for (const auto *sampledVolume : sampledVolumes) {
    colours.push_back(static_cast<QRgb>(sampledVolume->getSampledValue()));
  }
  return colours;
}

template <typename T>
static std::vector<bool>
getMatchingSampledValues(const std::vector<T> &values,
                         const libsbml::SampledVolume *sfvol) {
  std::vector<bool> matches(values.size(), false);
  if (sfvol->isSetSampledValue()) {
    T sv = static_cast<T>(sfvol->getSampledValue());
    std::transform(values.cbegin(), values.cend(), matches.begin(),
                   [sv](auto v) { return v == sv; });
  } else if (sfvol->isSetMinValue() && sfvol->isSetMaxValue()) {
    double min = sfvol->getMinValue();
    double max = sfvol->getMaxValue();
    std::transform(values.cbegin(), values.cend(), matches.begin(),
                   [min, max](T v) {
                     auto vAsDouble{static_cast<double>(v)};
                     return vAsDouble >= min && vAsDouble < max;
                   });
  }
  return matches;
}

static void setMatchingPixelsToColour(QImage &img,
                                      const std::vector<bool> &matches,
                                      QRgb colour) {
  auto iter = matches.begin();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      if (*iter) {
        img.setPixel(x, img.height() - 1 - y, colour);
      }
      ++iter;
    }
  }
}

template <typename T>
static std::vector<QRgb> setImagePixels(
    QImage &img, const libsbml::SampledField *sampledField,
    const std::vector<const libsbml::SampledVolume *> &sampledVolumes,
    const std::vector<QRgb> &importedSampledFieldColours = {}) {
  std::vector<QRgb> colours(sampledVolumes.size(), 0);
  img.fill(qRgb(0, 0, 0));
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
  if (common::isItIndexes(values, importedSampledFieldColours.size())) {
    setImportedColoursToImage(img, values, importedSampledFieldColours);
    auto iter = colours.begin();
    for (const auto *sampledVolume : sampledVolumes) {
      int sampledValue = (int)sampledVolume->getSampledValue();
      auto col = importedSampledFieldColours[sampledValue];
      SPDLOG_DEBUG("importedSampledFieldColours  {}/{} -> colour {:x}",
                   sampledVolume->getId(), sampledVolume->getDomainType(), col);
      *iter = col;
      ++iter;
    }
  }

  else {
    if (static_cast<int>(values.size()) != sampledField->getSamplesLength()) {
      SPDLOG_WARN("Number of samples {} doesn't match SamplesLength {}",
                  values.size(), sampledField->getSamplesLength());
    }
    std::size_t iCol = 0;
    auto iter = colours.begin();
    for (const auto *sampledVolume : sampledVolumes) {
      auto matches = getMatchingSampledValues(values, sampledVolume);
      if (std::find(matches.cbegin(), matches.cend(), true) != matches.cend()) {
        auto col = common::indexedColours()[iCol].rgb();
        SPDLOG_WARN("Color {} is {}", iCol, col);
        // int sampledValue = (int) sampledVolume->getSampledValue();
        //  if (sampledValue < importedSampledFieldColours.size()) {
        //    col = importedSampledFieldColours[sampledValue];
        //    SPDLOG_WARN("  -> Using importedcompartmentColour {}", col);
        //  }
        SPDLOG_DEBUG("  {}/{} -> colour {:x}", sampledVolume->getId(),
                     sampledVolume->getDomainType(), col);
        setMatchingPixelsToColour(img, matches, col);
        *iter = col;
        ++iCol;
      }
      ++iter;
    }
  }
  return colours;
}

static std::vector<std::pair<std::string, QRgb>> getCompartmentIdAndColours(
    const std::vector<const libsbml::Compartment *> &compartments,
    const std::vector<QRgb> &compartmentColours) {
  std::vector<std::pair<std::string, QRgb>> compartmentIdAndColours;
  compartmentIdAndColours.reserve(compartments.size());
  for (std::size_t i = 0; i < compartments.size(); ++i) {
    if (auto colour = compartmentColours[i]; colour != 0) {
      compartmentIdAndColours.push_back({compartments[i]->getId(), colour});
    }
  }
  return compartmentIdAndColours;
}

GeometrySampledField importGeometryFromSampledField(
    const libsbml::Geometry *geom,
    const std::vector<QRgb> &importedSampledFieldColours) {
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
  gsf.image = makeEmptyImage(sampledField);
  std::vector<QRgb> compartmentColours;
  auto dataType = sampledField->getDataType();
  SPDLOG_DEBUG("importedSampledFieldColours.size() = {} ",
               importedSampledFieldColours.size());

  std::vector<QRgb>::size_type b_iter = 0;
  std::for_each(importedSampledFieldColours.cbegin(),
                importedSampledFieldColours.cend(), [b_iter](QRgb col) mutable {
                  SPDLOG_DEBUG("importedSampledFieldColours {} = {} ", b_iter,
                               col);
                });

  // for (int i = 0; i < importedSampledFieldColours.size(); ++i) {
  //   SPDLOG_DEBUG("importedSampledFieldColours {} = {} ", i,
  //                importedSampledFieldColours[i]);
  // }

  if (isNativeSampledFieldFormat(sampledField, sampledVolumes)) {
    compartmentColours =
        setImagePixelsNative(gsf.image, sampledField, sampledVolumes);
  } else if (dataType == libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE) {
    compartmentColours = setImagePixels<double>(
        gsf.image, sampledField, sampledVolumes, importedSampledFieldColours);
  } else if (dataType == libsbml::DataKind_t::SPATIAL_DATAKIND_FLOAT) {
    compartmentColours = setImagePixels<float>(
        gsf.image, sampledField, sampledVolumes, importedSampledFieldColours);
  } else if (dataType == libsbml::DataKind_t::SPATIAL_DATAKIND_INT) {
    compartmentColours = setImagePixels<int>(
        gsf.image, sampledField, sampledVolumes, importedSampledFieldColours);
  } else {
    // remaining dataTypes are all unsigned ints of various sizes
    compartmentColours = setImagePixels<std::size_t>(
        gsf.image, sampledField, sampledVolumes, importedSampledFieldColours);
  }
  gsf.compartmentIdColourPairs =
      getCompartmentIdAndColours(compartments, compartmentColours);

  return gsf;
}

void exportSampledFieldGeometry(libsbml::Geometry *geom,
                                const QImage &compartmentImage) {
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

  SPDLOG_INFO("Writing {}x{} geometry image to SampledField {}",
              compartmentImage.width(), compartmentImage.height(),
              sfgeom->getSampledField());
  sf->setNumSamples1(compartmentImage.width());
  sf->setNumSamples2(compartmentImage.height());
  sf->setNumSamples3(1);
  sf->setSamplesLength(compartmentImage.width() * compartmentImage.height());

  std::vector<QRgb> samples;
  samples.reserve(static_cast<std::size_t>(compartmentImage.width() *
                                           compartmentImage.height()));
  // convert 2d pixmap into array of uints
  // NOTE: order of samples is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  for (int y = 0; y < compartmentImage.height(); ++y) {
    for (int x = 0; x < compartmentImage.width(); ++x) {
      samples.push_back(
          compartmentImage.pixelIndex(x, compartmentImage.height() - 1 - y));
    }
  }
  sf->setSamples(common::vectorToString(samples));
  SPDLOG_INFO("SampledField '{}': assigned {}x{} array of total length {}",
              sf->getId(), sf->getNumSamples1(), sf->getNumSamples2(),
              sf->getSamplesLength());
}

} // namespace sme::model
