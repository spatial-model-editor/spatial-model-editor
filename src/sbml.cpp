#include "sbml.hpp"

#include <unordered_set>

#include "logger.hpp"
#include "reactions.hpp"
#include "symbolic.hpp"
#include "utils.hpp"

namespace sbml {

static std::string ASTtoString(const libsbml::ASTNode *node) {
  std::unique_ptr<char, decltype(&std::free)> charAST(
      libsbml::SBML_formulaToL3String(node), &std::free);
  return charAST.get();
}

void SbmlDocWrapper::clearAllModelData() {
  compartments.clear();
  membranes.clear();
  species.clear();
  reactions.clear();
  functions.clear();
  mapSpeciesIdToColour.clear();
  mapCompIdToGeometry.clear();
  mapSpeciesIdToField.clear();
  membraneVec.clear();
  mapCompartmentToColour.clear();
  mapColourToCompartment.clear();
  mapMembraneToIndex.clear();
  mapMembraneToImage.clear();
}

void SbmlDocWrapper::clearAllGeometryData() {
  membranes.clear();
  reactions.clear();
  mapCompIdToGeometry.clear();
  mapSpeciesIdToField.clear();
  membraneVec.clear();
  mapCompartmentToColour.clear();
  mapColourToCompartment.clear();
  membranePairs.clear();
  mapColPairToIndex.clear();
  mapMembraneToIndex.clear();
  mapMembraneToImage.clear();
  mesh = mesh::Mesh{};
}

void SbmlDocWrapper::importSBMLString(const std::string &xml) {
  clearAllModelData();
  SPDLOG_INFO("Importing SBML from string...");
  doc.reset(libsbml::readSBMLFromString(xml.c_str()));
  initModelData();
}

void SbmlDocWrapper::importSBMLFile(const std::string &filename) {
  clearAllModelData();
  currentFilename = filename.c_str();
  SPDLOG_INFO("Loading SBML file {}...", filename);
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  initModelData();
}

void SbmlDocWrapper::writeDefaultGeometryToSBML() {
  SPDLOG_INFO("Creating new 2d SBML model geometry");
  geom = plugin->createGeometry();
  geom->setCoordinateSystem(
      libsbml::GeometryKind_t::SPATIAL_GEOMETRYKIND_CARTESIAN);
  for (std::size_t i = 0; i < nDimensions; ++i) {
    geom->createCoordinateComponent();
  }

  // set-up xy coordinates
  auto *coord = geom->getCoordinateComponent(0);
  coord->setType(libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  coord->setId("xCoord");
  auto *param = model->createParameter();
  param->setId("x");
  param->setConstant(false);
  auto *ssr = dynamic_cast<libsbml::SpatialParameterPlugin *>(
                  param->getPlugin("spatial"))
                  ->createSpatialSymbolReference();
  ssr->setSpatialRef(coord->getId());
  SPDLOG_INFO("  - creating Parameter: {}", param->getId());
  SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
  auto *min = coord->createBoundaryMin();
  min->setId("xBoundaryMin");
  min->setValue(0);
  auto *max = coord->createBoundaryMax();
  max->setId("xBoundaryMax");
  max->setValue(pixelWidth * static_cast<double>(compartmentImage.width()));
  SPDLOG_INFO("  - x in range [{},{}]", min->getValue(), max->getValue());

  coord = geom->getCoordinateComponent(1);
  coord->setType(libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  coord->setId("yCoord");
  param = model->createParameter();
  param->setId("y");
  param->setConstant(false);
  ssr = dynamic_cast<libsbml::SpatialParameterPlugin *>(
            param->getPlugin("spatial"))
            ->createSpatialSymbolReference();
  ssr->setSpatialRef(coord->getId());
  SPDLOG_INFO("  - creating Parameter: {}", ssr->getId());
  SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
  min = coord->createBoundaryMin();
  min->setId("yBoundaryMin");
  min->setValue(0);
  max = coord->createBoundaryMax();
  max->setId("yBoundaryMax");
  max->setValue(pixelWidth * static_cast<double>(compartmentImage.height()));
  SPDLOG_INFO("  - y in range [{},{}]", min->getValue(), max->getValue());

  // set isSpatial to true for all species
  for (unsigned i = 0; i < model->getNumSpecies(); ++i) {
    auto *ssp = dynamic_cast<libsbml::SpatialSpeciesPlugin *>(
        model->getSpecies(i)->getPlugin("spatial"));
    ssp->setIsSpatial(true);
  }

  // set isLocal to true for all reactions
  for (unsigned i = 0; i < model->getNumReactions(); ++i) {
    auto *srp = dynamic_cast<libsbml::SpatialReactionPlugin *>(
        model->getReaction(i)->getPlugin("spatial"));
    srp->setIsLocal(true);
  }

  // create sampled field geometry with empty SampledField
  sfgeom = geom->createSampledFieldGeometry();
  sfgeom->setId("sampledFieldGeometry");
  sfgeom->setIsActive(true);
  auto *sf = geom->createSampledField();
  sf->setId("geometryImage");
  sf->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32);
  sf->setInterpolationType(
      libsbml::InterpolationKind_t::SPATIAL_INTERPOLATIONKIND_NEARESTNEIGHBOR);
  sf->setCompression(
      libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
  sfgeom->setSampledField(sf->getId());

  // for each compartment:
  //  - create DomainType
  //  - create CompartmentMapping from compartment to DomainType
  //  - create Domain with this DomainType
  //  - create SampledVolume with with DomainType (pixel geometry)
  for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
    auto *comp = model->getCompartment(i);
    auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        comp->getPlugin("spatial"));
    const std::string &compartmentID = comp->getId();
    SPDLOG_INFO("  - compartment {}", compartmentID);
    auto *dt = geom->createDomainType();
    dt->setId(compartmentID + "_domainType");
    dt->setSpatialDimensions(static_cast<int>(nDimensions));
    SPDLOG_INFO("    * {}", dt->getId());
    auto *cmap = scp->createCompartmentMapping();
    cmap->setId(compartmentID + "_compartmentMapping");
    cmap->setDomainType(dt->getId());
    cmap->setUnitSize(1.0);
    SPDLOG_INFO("    * {}", cmap->getId());
    auto *dom = geom->createDomain();
    dom->setId(compartmentID + "_domain");
    dom->setDomainType(dt->getId());
    SPDLOG_INFO("    * {}", dom->getId());
    auto *sfvol = sfgeom->createSampledVolume();
    sfvol->setId(compartmentID + "_sampledVolume");
    sfvol->setDomainType(dt->getId());
    SPDLOG_INFO("    * {}", sfvol->getId());
  }
}

void SbmlDocWrapper::importSpatialData() {
  importGeometryDimensions();
  importSampledFieldGeometry();
  importParametricGeometry();
}

void SbmlDocWrapper::importGeometryDimensions() {
  geom = plugin->getGeometry();
  SPDLOG_INFO("Importing existing {}d SBML model geometry",
              geom->getNumCoordinateComponents());
  if (geom->getNumCoordinateComponents() != nDimensions) {
    SPDLOG_CRITICAL("Error: Only {}d models are currently supported",
                    nDimensions);
    // todo: offer to ignore spatial part of model & import anyway
  }
  // import xy coordinates
  const auto *xcoord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  if (xcoord == nullptr) {
    SPDLOG_CRITICAL("Error: no x-coordinate found in SBML model");
  }
  SPDLOG_INFO("- found x range [{},{}]", xcoord->getBoundaryMin()->getValue(),
              xcoord->getBoundaryMax()->getValue());
  const auto *ycoord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  if (ycoord == nullptr) {
    SPDLOG_CRITICAL("Error: no y-coordinate found in SBML model");
  }
  SPDLOG_INFO("- found y range [{},{}]", ycoord->getBoundaryMin()->getValue(),
              ycoord->getBoundaryMax()->getValue());
  origin = QPointF(xcoord->getBoundaryMin()->getValue(),
                   ycoord->getBoundaryMin()->getValue());
}

void SbmlDocWrapper::importSampledFieldGeometry() {
  // get sampled field geometry
  sfgeom = nullptr;
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    auto *def = geom->getGeometryDefinition(i);
    if (def->getIsActive() && def->isSampledFieldGeometry()) {
      sfgeom = dynamic_cast<libsbml::SampledFieldGeometry *>(def);
    }
  }
  if (sfgeom == nullptr) {
    SPDLOG_CRITICAL("Error: Failed to load sampled field geometry");
    qFatal(
        "SbmlDocWrapper::initModelData :: Error: Failed to load sampled field "
        "geometry");
  }

  // import geometry image
  auto *sf = geom->getSampledField(sfgeom->getSampledField());
  int xVals = sf->getNumSamples1();
  int yVals = sf->getNumSamples2();
  int totalVals = sf->getSamplesLength();
  auto samples = utils::stringToVector<QRgb>(sf->getSamples());
  if (static_cast<std::size_t>(totalVals) != samples.size()) {
    SPDLOG_WARN("Number of ints in string {} doesn't match samplesLength {}",
                samples.size(), totalVals);
  }
  // convert values into 2d pixmap
  // NOTE: order of samples is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  QImage img(xVals, yVals, QImage::Format_RGB32);
  auto iter = samples.begin();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      img.setPixel(x, img.height() - 1 - y, *iter);
      ++iter;
    }
  }
  SPDLOG_INFO("  - found {} geometry image", img.size());
  importGeometryFromImage(img, false);

  // calculate pixel size from image dimensions
  const auto *xcoord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  double xPixels = static_cast<double>(compartmentImage.width());
  double xPixelSize = (xcoord->getBoundaryMax()->getValue() -
                       xcoord->getBoundaryMin()->getValue()) /
                      xPixels;
  const auto *ycoord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  double yPixels = static_cast<double>(compartmentImage.height());
  double yPixelSize = (ycoord->getBoundaryMax()->getValue() -
                       ycoord->getBoundaryMin()->getValue()) /
                      yPixels;
  if (std::abs((xPixelSize - yPixelSize) / xPixelSize) > 1e-12) {
    SPDLOG_WARN("Pixels are not square: {} x {}", xPixelSize, yPixelSize);
  }
  SPDLOG_INFO("  - pixel size: {}", pixelWidth);
  pixelWidth = xPixelSize;

  // assign each compartment to a colour
  for (const auto &compartmentID : compartments) {
    auto *comp = model->getCompartment(compartmentID.toStdString());
    auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        comp->getPlugin("spatial"));
    if (scp->isSetCompartmentMapping()) {
      const std::string &domainTypeID =
          scp->getCompartmentMapping()->getDomainType();
      auto *sfvol = sfgeom->getSampledVolumeByDomainType(domainTypeID);
      QRgb col = static_cast<QRgb>(sfvol->getSampledValue());
      SPDLOG_INFO("setting compartment {} colour to {:x}", compartmentID, col);
      SPDLOG_INFO("  - DomainType: {}", domainTypeID);
      SPDLOG_INFO("  - SampledFieldVolume: {}", sfvol->getId());
      setCompartmentColour(compartmentID, col, false);
    }
  }
}

void SbmlDocWrapper::importParametricGeometry() {
  // get sampled field geometry
  parageom = nullptr;
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    auto *def = geom->getGeometryDefinition(i);
    if (def->getIsActive() && def->isParametricGeometry()) {
      parageom = dynamic_cast<libsbml::ParametricGeometry *>(def);
    }
  }
  if (parageom == nullptr) {
    SPDLOG_WARN("Failed to load Parametric Field geometry");
    return;
  }

  // get interiorPoints in terms of physical location
  // & convert them to integer pixel points
  std::vector<QPointF> interiorPoints;
  for (const auto &compartmentID : compartments) {
    SPDLOG_DEBUG("Found interior point: {}");
    QPointF interiorFloatPhysical = getCompartmentInteriorPoint(compartmentID);
    QPointF interiorFloatPixel =
        (interiorFloatPhysical - origin) / pixelWidth + QPointF(0.3, 0.3);
    interiorPoints.push_back(interiorFloatPixel);
    SPDLOG_DEBUG("  - pixel location: {}", interiorPoints.back());
  }

  // get maxBoundaryPoints and maxTriangleAreas
  if (parageom->isSetAnnotation()) {
    auto *node = parageom->getAnnotation();
    for (unsigned i = 0; i < node->getNumChildren(); ++i) {
      auto &child = node->getChild(i);
      if (child.getURI() == annotationURI &&
          child.getPrefix() == annotationPrefix && child.getName() == "mesh") {
        auto maxPoints = utils::stringToVector<std::size_t>(
            child.getAttrValue("maxBoundaryPoints", annotationURI));
        SPDLOG_INFO("  - maxBoundaryPoints: {}", maxPoints);
        auto maxAreas = utils::stringToVector<std::size_t>(
            child.getAttrValue("maxTriangleAreas", annotationURI));
        SPDLOG_INFO("  - maxTriangleAreas: {}", maxAreas);
        // generate Mesh
        SPDLOG_INFO("  - re-generating mesh");
        mesh = mesh::Mesh(compartmentImage, interiorPoints, maxPoints, maxAreas,
                          pixelWidth, origin);
      }
    }
    return;
  }

  // If we didn't find mesh parameters generated by our code,
  // then the ParametricGeometry was created by some other software:
  // load it and use as a read-only mesh
  SPDLOG_INFO("loading as read-only mesh");
  // import vertices:
  int nVertices = parageom->getSpatialPoints()->getArrayDataLength();
  std::vector<double> vertices(static_cast<std::size_t>(nVertices), 0);
  parageom->getSpatialPoints()->getArrayData(vertices.data());
  SPDLOG_INFO("  - found {} vertices", nVertices / 2);
  // import triangles for each compartment
  std::vector<std::vector<int>> triangles;
  for (const auto &compartmentID : compartments) {
    auto *po = getParametricObject(compartmentID.toStdString());
    auto nPoints = static_cast<std::size_t>(po->getPointIndexLength());
    SPDLOG_INFO("  - compartment {}: found {} triangles", compartmentID,
                nPoints / 3);
    triangles.emplace_back(nPoints, 0);
    po->getPointIndex(triangles.back().data());
  }
  mesh = mesh::Mesh(vertices, triangles, interiorPoints);
}

void SbmlDocWrapper::initModelData() {
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    std::stringstream ss;
    doc->printErrors(ss);
    SPDLOG_WARN("SBML document errors:\n\n{}", ss.str());
    SPDLOG_WARN(
        "Warning - errors while reading SBML file (continuing anyway...)");
    isValid = true;
  } else {
    SPDLOG_INFO("Successfully imported SBML Level {}, Version {} model",
                doc->getLevel(), doc->getVersion());
    isValid = true;
  }

  model = doc->getModel();

  // get list of compartments
  for (unsigned int i = 0; i < model->getNumCompartments(); ++i) {
    const auto *comp = model->getCompartment(i);
    QString id = comp->getId().c_str();
    compartments << id;
    species[id] = QStringList();
  }

  // get all species, make a list for each compartment
  for (unsigned int i = 0; i < model->getNumSpecies(); ++i) {
    const auto *spec = model->getSpecies(i);
    if (spec->isSetHasOnlySubstanceUnits() &&
        spec->getHasOnlySubstanceUnits()) {
      // equations expect amount, not concentration for this species
      // for now this is not supported:
      std::string errorMessage(
          "SbmlDocWrapper::importSBMLFile :: Error: "
          "HasOnlySubstanceUnits=true "
          "is not yet supported.");
      SPDLOG_CRITICAL(errorMessage);
      qFatal("%s", errorMessage.c_str());
    }
    const auto id = spec->getId().c_str();
    species[spec->getCompartment().c_str()] << QString(id);
    // assign a default colour for displaying the species
    mapSpeciesIdToColour[id] = utils::indexedColours()[i];
  }

  // get list of functions
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    functions << QString(func->getId().c_str());
  }

  // upgrade SBML document to latest version
  if (doc->setLevelAndVersion(libsbml::SBMLDocument::getDefaultLevel(),
                              libsbml::SBMLDocument::getDefaultVersion())) {
    SPDLOG_INFO("Successfully upgraded SBML model to Level {}, Version {}",
                doc->getLevel(), doc->getVersion());
  } else {
    SPDLOG_CRITICAL(
        "Error - failed to upgrade SBML file (continuing anyway...)");
  }
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    std::stringstream ss;
    doc->printErrors(ss);
    SPDLOG_WARN("SBML document errors:\n\n{}", ss.str());
  }

  if (!doc->isPackageEnabled("spatial")) {
    doc->enablePackage(libsbml::SpatialExtension::getXmlnsL3V1V1(), "spatial",
                       true);
    doc->setPackageRequired("spatial", true);
    SPDLOG_INFO("Enabling spatial extension");
  }

  plugin =
      dynamic_cast<libsbml::SpatialModelPlugin *>(model->getPlugin("spatial"));
  if (plugin == nullptr) {
    SPDLOG_WARN("Failed to get SpatialModelPlugin from SBML document");
  }

  if (plugin->isSetGeometry()) {
    importSpatialData();
  } else {
    writeDefaultGeometryToSBML();
    // if we already had a geometry image, and we loaded a model without spatial
    // info, use this geometry image
    if (hasGeometry) {
      importGeometryFromImage(compartmentImage);
    }
  }
}

void SbmlDocWrapper::exportSBMLFile(const std::string &filename) {
  if (isValid) {
    writeGeometryMeshToSBML();
    SPDLOG_INFO("Exporting SBML model to {}", filename);
    if (!libsbml::SBMLWriter().writeSBML(doc.get(), filename)) {
      SPDLOG_ERROR("Failed to write to {}", filename);
    }
  }
}

QString SbmlDocWrapper::getXml() {
  QString xml;
  writeGeometryMeshToSBML();
  std::unique_ptr<char, decltype(&std::free)> xmlChar(
      libsbml::writeSBMLToString(doc.get()), &std::free);
  xml = QString(xmlChar.get());
  return xml;
}

void SbmlDocWrapper::importGeometryFromImage(const QImage &img,
                                             bool updateSBML) {
  clearAllGeometryData();
  compartmentImage = img;
  initMembraneColourPairs();
  if (isValid && updateSBML) {
    writeGeometryImageToSBML();
  }
  hasGeometry = true;
}

void SbmlDocWrapper::importGeometryFromImage(const QString &filename,
                                             bool updateSBML) {
  QImage img(filename);
  importGeometryFromImage(img, updateSBML);
}

void SbmlDocWrapper::writeGeometryImageToSBML() {
  // export image to SampledField
  auto *sf = geom->getSampledField(sfgeom->getSampledField());
  SPDLOG_INFO("Writing {}x{} geometry image to SampledField {}",
              compartmentImage.width(), compartmentImage.height(),
              sfgeom->getSampledField());
  sf->setNumSamples1(compartmentImage.width());
  sf->setNumSamples2(compartmentImage.height());
  sf->setSamplesLength(compartmentImage.width() * compartmentImage.height());
  std::vector<QRgb> samples;
  samples.reserve(static_cast<std::size_t>(compartmentImage.width() *
                                           compartmentImage.height()));
  // convert 2d pixmap into array of uints
  // NOTE: order of samples is [ (x=0,y=0), (x=1,y=0), ... ]
  // NOTE: (0,0) point is at bottom-left
  // NOTE: QImage has (0,0) point at top-left, so flip y-coord here
  for (int y = 0; y < compartmentImage.height(); ++y) {
    for (int x = 0; x < compartmentImage.width(); ++x) {
      samples.push_back(
          compartmentImage.pixel(x, compartmentImage.height() - 1 - y));
    }
  }
  std::string samplesString = utils::vectorToString(samples);
  sf->setSamples(samplesString);
  SPDLOG_INFO("SampledField '{}': assigned an array of length {}", sf->getId(),
              sf->getSamplesLength());
}

void SbmlDocWrapper::setFieldConcAnalytic(geometry::Field &field,
                                          const std::string &expr) {
  SPDLOG_INFO("expr: {}", expr);
  auto inlinedExpr = inlineExpr(expr);
  SPDLOG_INFO("  - inlined expr: {}", inlinedExpr);
  symbolic::Symbolic sym(inlinedExpr, {"x", "y"}, getGlobalConstants());
  SPDLOG_INFO("  - parsed expr: {}", sym.simplify());
  auto result = std::vector<double>(1, 0);
  auto vars = std::vector<double>(2, 0);
  for (std::size_t i = 0; i < field.geometry->ix.size(); ++i) {
    // position in pixels (with (0,0) in top-left of image):
    const auto &point = field.geometry->ix.at(i);
    // rescale to physical x,y point (with (0,0) in bottom-left):
    double minX =
        geom->getCoordinateComponentByKind(
                libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X)
            ->getBoundaryMin()
            ->getValue();
    vars[0] = minX + pixelWidth * static_cast<double>(point.x());
    double minY =
        geom->getCoordinateComponentByKind(
                libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y)
            ->getBoundaryMin()
            ->getValue();
    vars[1] =
        minY + pixelWidth * static_cast<double>(
                                field.geometry->getCompartmentImage().height() -
                                1 - point.y());
    sym.evalLLVM(result, vars);
    field.conc[i] = result[0];
  }
  field.init = field.conc;
  field.isUniformConcentration = false;
}

void SbmlDocWrapper::initMembraneColourPairs() {
  // convert geometry image to 8-bit indexed format:
  // each pixel points to an index in the colorTable
  // which contains an RGB value for each color in the image
  compartmentImage = compartmentImage.convertToFormat(QImage::Format_Indexed8);

  // for each pair of different colours: map to a continuous index
  // NOTE: colours ordered by ascending numerical value
  membranePairs.clear();
  mapColPairToIndex.clear();
  std::size_t i = 0;
  for (int iB = 1; iB < compartmentImage.colorCount(); ++iB) {
    for (int iA = 0; iA < iB; ++iA) {
      QRgb colA = compartmentImage.colorTable()[iA];
      QRgb colB = compartmentImage.colorTable()[iB];
      membranePairs.push_back(std::vector<std::pair<QPoint, QPoint>>());
      mapColPairToIndex[{std::min(colA, colB), std::max(colA, colB)}] = i++;
      SPDLOG_DEBUG("ColourPair iA,iB: {},{}: {:x},{:x}", iA, iB,
                   std::min(colA, colB), std::max(colA, colB));
    }
  }
  // for each pair of adjacent pixels of different colour,
  // add the pair of QPoints to the vector for this pair of colours,
  // i.e. the membrane between compartments of these two colours
  // NOTE: colour pairs are ordered in ascending order
  // x-neighbours
  for (int y = 0; y < compartmentImage.height(); ++y) {
    QRgb prevPix = compartmentImage.pixel(0, y);
    for (int x = 1; x < compartmentImage.width(); ++x) {
      QRgb currPix = compartmentImage.pixel(x, y);
      if (currPix != prevPix) {
        if (currPix < prevPix) {
          membranePairs[mapColPairToIndex.at({currPix, prevPix})].push_back(
              {QPoint(x, y), QPoint(x - 1, y)});
        } else {
          membranePairs[mapColPairToIndex.at({prevPix, currPix})].push_back(
              {QPoint(x - 1, y), QPoint(x, y)});
        }
      }
      prevPix = currPix;
    }
  }
  // y-neighbours
  for (int x = 0; x < compartmentImage.width(); ++x) {
    QRgb prevPix = compartmentImage.pixel(x, 0);
    for (int y = 1; y < compartmentImage.height(); ++y) {
      QRgb currPix = compartmentImage.pixel(x, y);
      if (currPix != prevPix) {
        if (currPix < prevPix) {
          membranePairs[mapColPairToIndex.at({currPix, prevPix})].push_back(
              {QPoint(x, y), QPoint(x, y - 1)});
        } else {
          membranePairs[mapColPairToIndex.at({prevPix, currPix})].push_back(
              {QPoint(x, y - 1), QPoint(x, y)});
        }
      }
      prevPix = currPix;
    }
  }
}

const QImage &SbmlDocWrapper::getMembraneImage(
    const QString &membraneID) const {
  if (!hasGeometry) {
    return compartmentImage;
  }
  return mapMembraneToImage.at(membraneID);
}

void SbmlDocWrapper::updateMembraneList() {
  // construct membrane list & images
  membranes.clear();
  mapMembraneToIndex.clear();
  mapMembraneToImage.clear();
  // clear vector of Membrane objects
  membraneVec.clear();
  // iterate over pairs of compartments
  for (int i = 1; i < compartments.size(); ++i) {
    for (int j = 0; j < i; ++j) {
      QRgb colA = mapCompartmentToColour[compartments[i]];
      QRgb colB = mapCompartmentToColour[compartments[j]];
      auto iter =
          mapColPairToIndex.find({std::min(colA, colB), std::max(colA, colB)});
      // if membrane for pair of colours exists, generate name and image, and
      // add to list
      if (iter != mapColPairToIndex.cend()) {
        std::size_t index = iter->second;
        if (!membranePairs[index].empty()) {
          // generate membrane name, compartments ordered by colour
          QString name = compartments[i] + "_" + compartments[j];
          if (colA > colB) {
            name = compartments[j] + "_" + compartments[i];
          }
          membranes.push_back(name);
          // map name to index of membrane location pairs
          mapMembraneToIndex[name] = index;
          // generate image
          QImage img = compartmentImage.convertToFormat(QImage::Format_ARGB32);
          for (const auto &pair : membranePairs[index]) {
            img.setPixel(pair.first, QColor(0, 155, 40, 255).rgba());
            img.setPixel(pair.second, QColor(0, 199, 40, 255).rgba());
          }
          mapMembraneToImage[name] = img;
          // create Membrane object
          geometry::Compartment *compA =
              &mapCompIdToGeometry.at(compartments[i]);
          geometry::Compartment *compB =
              &mapCompIdToGeometry.at(compartments[j]);
          if (colA > colB) {
            std::swap(compA, compB);
          }
          assert(mapCompartmentToColour.at(compA->compartmentID.c_str()) <
                 mapCompartmentToColour.at(compB->compartmentID.c_str()));
          membraneVec.emplace_back(name.toStdString(), compA, compB,
                                   membranePairs[index]);
        }
      }
    }
  }
}

void SbmlDocWrapper::updateReactionList() {
  reactions.clear();
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    auto *reac = model->getReaction(i);
    auto *srp = dynamic_cast<libsbml::SpatialReactionPlugin *>(
        reac->getPlugin("spatial"));
    QString reacID = reac->getId().c_str();
    // construct the set of compartments where reaction takes place
    std::unordered_set<std::string> comps;
    for (unsigned int k = 0; k < reac->getNumProducts(); ++k) {
      const std::string &specID = reac->getProduct(k)->getSpecies();
      comps.insert(model->getSpecies(specID)->getCompartment());
    }
    // TODO: also include modifiers here?? (and when compiling reactions??)
    for (unsigned int k = 0; k < reac->getNumReactants(); ++k) {
      const std::string &specID = reac->getReactant(k)->getSpecies();
      comps.insert(model->getSpecies(specID)->getCompartment());
    }
    // single compartment
    if (comps.size() == 1) {
      QString comp = comps.begin()->c_str();
      reactions[comp] << reacID;
      srp->setIsLocal(true);
      reac->setCompartment(comp.toStdString());
    } else if (comps.size() == 2) {
      auto iter = comps.cbegin();
      QString compA = iter->c_str();
      ++iter;
      QString compB = iter->c_str();
      // two compartments: want the membrane between them
      // membrane name is compA-compB, ordered by colour
      QRgb colA = mapCompartmentToColour[compA];
      QRgb colB = mapCompartmentToColour[compB];
      QString membraneID = compA + "_" + compB;
      if (colA > colB) {
        membraneID = compB + "_" + compA;
      }
      // check that compartments map to colours - if not do nothing
      if (colA != 0 && colB != 0) {
        reactions[membraneID] << QString(reac->getId().c_str());
      }
      srp->setIsLocal(true);
      // todo: work out what to do with reactions on a membrane
      // for now just setting the compartment to compA and ignoring it
      reac->setCompartment(compA.toStdString());
    } else {
      // invalid reaction: number of compartments for reaction must be 1 or 2
      SPDLOG_ERROR("Reaction involves {} compartments - not supported",
                   comps.size());
      exit(1);
    }
  }
}

const QImage &SbmlDocWrapper::getCompartmentImage() const {
  return compartmentImage;
}

double SbmlDocWrapper::getCompartmentSize(const QString &compartmentID) const {
  return model->getCompartment(compartmentID.toStdString())->getSize();
}

double SbmlDocWrapper::getSpeciesCompartmentSize(
    const QString &speciesID) const {
  QString compID =
      model->getSpecies(speciesID.toStdString())->getCompartment().c_str();
  return getCompartmentSize(compID);
}

QString SbmlDocWrapper::getCompartmentID(QRgb colour) const {
  auto iter = mapColourToCompartment.find(colour);
  if (iter == mapColourToCompartment.cend()) {
    return "";
  }
  return iter->second;
}

QRgb SbmlDocWrapper::getCompartmentColour(const QString &compartmentID) const {
  auto iter = mapCompartmentToColour.find(compartmentID);
  if (iter == mapCompartmentToColour.cend()) {
    return 0;
  }
  return iter->second;
}

void SbmlDocWrapper::setCompartmentColour(const QString &compartmentID,
                                          QRgb colour, bool updateSBML) {
  SPDLOG_INFO("assigning colour {} to compartment {}", colour, compartmentID);
  // todo: add check that colour exists in geometry image?
  QRgb oldColour = getCompartmentColour(compartmentID);
  if (oldColour != 0) {
    SPDLOG_INFO(
        "setting previous compartment colour {} to point to null compartment",
        oldColour);
    // if there was already a colour mapped to this compartment, map it to a
    // null compartment
    mapColourToCompartment[oldColour] = "";
  }
  auto oldCompartmentID = getCompartmentID(colour);
  if (oldCompartmentID != "") {
    SPDLOG_INFO("colour {} used to point to compartment to {}: removing",
                colour, oldCompartmentID);
    // if the new colour was already mapped to another compartment, set the
    // colour of that compartment to null
    mapCompartmentToColour[oldCompartmentID] = 0;
  }
  mapColourToCompartment[colour] = compartmentID;
  mapCompartmentToColour[compartmentID] = colour;
  // create compartment geometry for this colour
  mapCompIdToGeometry[compartmentID] = geometry::Compartment(
      compartmentID.toStdString(), getCompartmentImage(), colour);
  geometry::Compartment &comp = mapCompIdToGeometry.at(compartmentID);
  // create a field for each species in this compartment
  for (const auto &s : species[compartmentID]) {
    SPDLOG_INFO("creating field for species {}", s);
    mapSpeciesIdToField[s] = geometry::Field(&comp, s.toStdString(), 1.0,
                                             mapSpeciesIdToColour.at(s));
    geometry::Field &field = mapSpeciesIdToField.at(s);
    // set all species concentrations to their initial values
    const auto *spec = model->getSpecies(s.toStdString());
    // todo: deal with case of initialAmount, or neither being set
    field.setUniformConcentration(spec->getInitialConcentration());

    if (model->getInitialAssignmentBySymbol(s.toStdString()) != nullptr) {
      // if an initial assingment exists, it takes precedence over
      // the InitialConcentration set above
      const auto *asgn = model->getInitialAssignmentBySymbol(s.toStdString());
      std::string expr = ASTtoString(asgn->getMath());
      SPDLOG_INFO("found initialAssignment: {} = {}", s, expr);
      if (model->getParameter(expr) != nullptr) {
        // simplest case: formula is just the name of a parameter
        const auto *param = model->getParameter(expr);
        const auto *spp = dynamic_cast<const libsbml::SpatialParameterPlugin *>(
            param->getPlugin("spatial"));
        if (spp != nullptr && spp->isSetSpatialSymbolReference()) {
          // parameter is a spatialref to a sampled field
          // -> set initial concentration to this sampled field
          const std::string &sampledFieldID =
              spp->getSpatialSymbolReference()->getSpatialRef();
          SPDLOG_INFO("  - spatialSymbolReference: {} -> {}", param->getId(),
                      sampledFieldID);
          const auto *sf = geom->getSampledField(sampledFieldID);
          std::vector<double> arrayConc;
          sf->getSamples(arrayConc);
          field.importConcentration(arrayConc);
        } else {
          // normal parameter, set initialConcentration to its value
          SPDLOG_INFO("  - parameter {} = {}", param->getId(),
                      param->getValue());
          field.setUniformConcentration(param->getValue());
        }
      } else {
        setFieldConcAnalytic(field, expr);
      }
    }

    // set isSpatial flag
    const auto *ssp = dynamic_cast<const libsbml::SpatialSpeciesPlugin *>(
        spec->getPlugin("spatial"));
    if (ssp != nullptr && ssp->isSetIsSpatial() && ssp->getIsSpatial()) {
      field.isSpatial = true;
    } else {
      field.isSpatial = false;
    }
    // set diffusion constant value
    // todo: deal with potential non-isotropic case in SBML
    for (unsigned i = 0; i < model->getNumParameters(); ++i) {
      const auto *param = model->getParameter(i);
      const auto *pplugin =
          dynamic_cast<const libsbml::SpatialParameterPlugin *>(
              param->getPlugin("spatial"));
      // iterate over diff coeff params
      if (pplugin != nullptr && pplugin->isSetDiffusionCoefficient()) {
        // check if diff coeff applies to our species
        const auto *diffcoeff = pplugin->getDiffusionCoefficient();
        if (diffcoeff->getVariable() == s.toStdString()) {
          field.diffusionConstant = param->getValue();
        }
      }
    }
  }
  // update list of possible inter-compartment membranes
  updateMembraneList();
  // update list of reactions for each compartment/membrane
  updateReactionList();

  // update all compartment mappings in SBML
  // (don't want to do this when importing an SBML file)
  if (updateSBML) {
    for (unsigned int i = 0; i < model->getNumCompartments(); ++i) {
      const std::string &compID = model->getCompartment(i)->getId();
      // set SampledValue (aka colour) of SampledFieldVolume
      auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
          model->getCompartment(compID)->getPlugin("spatial"));
      const std::string &domainType =
          scp->getCompartmentMapping()->getDomainType();
      auto *sfvol = sfgeom->getSampledVolumeByDomainType(domainType);
      sfvol->setSampledValue(
          static_cast<double>(getCompartmentColour(compID.c_str())));
    }
  }
}

void SbmlDocWrapper::updateMesh() {
  std::vector<QPointF> interiorPoints;
  for (const auto &compID : compartments) {
    const QPointF interior = getCompartmentInteriorPoint(compID);
    if (interior == QPointF()) {
      SPDLOG_INFO("compartment {} missing interiorPoint: skip mesh update",
                  compID);
      return;
    }
    interiorPoints.push_back(interior);
  }
  SPDLOG_INFO("Updating mesh interior points");
  mesh =
      mesh::Mesh(compartmentImage, interiorPoints, {}, {}, pixelWidth, origin);
}

libsbml::ParametricObject *SbmlDocWrapper::getParametricObject(
    const std::string &compartmentID) const {
  // NB: parageom->getParametricObjectByDomainType returned null when compiled
  // with gcc9 - have not understood why, for now just avoiding using it
  const auto *comp = model->getCompartment(compartmentID);
  const auto *scp = dynamic_cast<const libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  const std::string &domainTypeID =
      scp->getCompartmentMapping()->getDomainType();
  for (unsigned j = 0; j < parageom->getNumParametricObjects(); ++j) {
    auto *p = parageom->getParametricObject(j);
    if (p->getDomainType() == domainTypeID) {
      return p;
    }
  }
  return nullptr;
}

void SbmlDocWrapper::writeMeshParamsAnnotation(
    libsbml::ParametricGeometry *pg) {
  // if there is already an annotation set by us, remove it
  if (pg->isSetAnnotation()) {
    auto *node = pg->getAnnotation();
    for (unsigned i = 0; i < node->getNumChildren(); ++i) {
      auto &child = node->getChild(i);
      if (child.getURI() == annotationURI &&
          child.getPrefix() == annotationPrefix && child.getName() == "mesh") {
        SPDLOG_INFO("removing annotation {} : {}", i, node->toXMLString());
        // Note: removeChild returns a pointer to the child,
        // and removes this pointer from the SBML doc,
        // but does not delete the Child,
        // so now we own it and it will be deleted when
        // this unique pointer goes out of scope
        std::unique_ptr<libsbml::XMLNode> childNode(node->removeChild(i));
        break;
      }
    }
  }
  // append annotation with mesh info
  std::string xml = "<";
  xml.append(annotationPrefix);
  xml.append(":mesh xmlns:");
  xml.append(annotationPrefix);
  xml.append("=\"");
  xml.append(annotationURI);
  xml.append("\" ");
  xml.append(annotationPrefix);
  xml.append(":maxBoundaryPoints=\"");
  xml.append(utils::vectorToString(mesh.getBoundaryMaxPoints()));
  xml.append("\" ");
  xml.append(annotationPrefix);
  xml.append(":maxTriangleAreas=\"");
  xml.append(utils::vectorToString(mesh.getCompartmentMaxTriangleArea()));
  xml.append("\"/>");
  pg->appendAnnotation(xml);
  SPDLOG_INFO("appending annotation: {}", xml);
}

void SbmlDocWrapper::writeGeometryMeshToSBML() {
  if (mesh.getVertices().empty()) {
    SPDLOG_INFO("No mesh to export to SBML");
    return;
  }
  if (parageom == nullptr) {
    SPDLOG_INFO("No ParametricGeometry found, creating...");
    parageom = geom->createParametricGeometry();
    parageom->setId("parametricGeometry");
    parageom->setIsActive(true);
    auto *sp = parageom->createSpatialPoints();
    sp->setId("spatialPoints");
    sp->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE);
    sp->setCompression(
        libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
    for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
      auto *comp = model->getCompartment(i);
      auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
          comp->getPlugin("spatial"));
      const std::string &compartmentID = comp->getId();
      SPDLOG_INFO("  - compartment {}", compartmentID);
      std::string domainTypeID = scp->getCompartmentMapping()->getDomainType();
      SPDLOG_INFO("  - domainType {}", domainTypeID);
      auto *po = parageom->createParametricObject();
      po->setId(compartmentID + "_triangles");
      po->setPolygonType(libsbml::PolygonKind_t::SPATIAL_POLYGONKIND_TRIANGLE);
      po->setDomainType(domainTypeID);
      po->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32);
      po->setCompression(
          libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
      SPDLOG_INFO("  - parametricObject {}", po->getId());
      SPDLOG_INFO("  - domainType {}", po->getDomainType());
    }
  }

  if (!mesh.isReadOnly()) {
    // if we constructed the mesh, add the parameters required
    // to reconstruct it from the geometry image as an annotation
    writeMeshParamsAnnotation(parageom);
  }

  // write vertices
  std::vector<double> vertices = mesh.getVertices();
  auto *sp = parageom->getSpatialPoints();
  int sz = static_cast<int>(vertices.size());
  sp->setArrayData(vertices.data(), sz);
  sp->setArrayDataLength(sz);
  SPDLOG_INFO("  - added {} doubles ({} vertices)", sz, sz / 2);

  SPDLOG_INFO(" Writing mesh triangles:");
  // write mesh triangles for each compartment
  for (int i = 0; i < compartments.size(); ++i) {
    auto compartmentID = compartments[i].toStdString();
    SPDLOG_INFO("  - compartment {}", compartmentID);
    auto *po = getParametricObject(compartmentID);
    if (po == nullptr) {
      SPDLOG_CRITICAL("    - no parametricObject found");
    }
    SPDLOG_INFO("    - parametricObject: {}", po->getId());
    std::vector<int> triangleInts =
        mesh.getTriangleIndices(static_cast<std::size_t>(i));
    int size = static_cast<int>(triangleInts.size());
    po->setPointIndexLength(size);
    po->setPointIndex(triangleInts.data(), size);
    SPDLOG_INFO("    - added {} uints ({} triangles)", size, size / 2);
  }
  return;
}

QPointF SbmlDocWrapper::getCompartmentInteriorPoint(
    const QString &compartmentID) const {
  SPDLOG_INFO("compartmentID: {}", compartmentID);
  auto *comp = model->getCompartment(compartmentID.toStdString());
  auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  const std::string &domainType = scp->getCompartmentMapping()->getDomainType();
  SPDLOG_INFO("  - domainType: {}", domainType);
  auto *domain = geom->getDomainByDomainType(domainType);
  SPDLOG_INFO("  - domain: {}", domain->getId());
  SPDLOG_INFO("  - numInteriorPoints: {}", domain->getNumInteriorPoints());
  if (domain->getNumInteriorPoints() == 0) {
    SPDLOG_INFO("  - no interior point found");
    return QPointF(0, 0);
  }
  auto *interiorPoint = domain->getInteriorPoint(0);
  QPointF point(interiorPoint->getCoord1(), interiorPoint->getCoord2());
  SPDLOG_INFO("  - interior point {}", point);
  return point;
}

void SbmlDocWrapper::setCompartmentInteriorPoint(const QString &compartmentID,
                                                 const QPointF &point) {
  SPDLOG_INFO("compartmentID: {}", compartmentID);
  SPDLOG_INFO("  - setting interior point {}", point);
  auto *comp = model->getCompartment(compartmentID.toStdString());
  auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
      comp->getPlugin("spatial"));
  const std::string &domainType = scp->getCompartmentMapping()->getDomainType();
  SPDLOG_INFO("  - domainType: {}", domainType);
  auto *domain = geom->getDomainByDomainType(domainType);
  SPDLOG_INFO("  - domain: {}", domain->getId());
  auto *interiorPoint = domain->getInteriorPoint(0);
  if (interiorPoint == nullptr) {
    SPDLOG_INFO("  - creating new interior point");
    interiorPoint = domain->createInteriorPoint();
  }
  interiorPoint->setCoord1(point.x());
  // convert from QPoint with (0,0) in top-left to (0,0) in bottom-left
  interiorPoint->setCoord2(compartmentImage.height() - 1 - point.y());
  // update mesh with new interior point
  updateMesh();
}

void SbmlDocWrapper::setAnalyticConcentration(
    const QString &speciesID, const QString &analyticExpression) {
  SPDLOG_INFO("speciesID: {}", speciesID);
  SPDLOG_INFO("  - expression: {}", analyticExpression);
  libsbml::InitialAssignment *asgn;
  if (model->getInitialAssignmentBySymbol(speciesID.toStdString()) != nullptr) {
    asgn = model->getInitialAssignmentBySymbol(speciesID.toStdString());
    SPDLOG_INFO("  - replacing existing assignment: {}", asgn->getId());
  } else {
    asgn = model->createInitialAssignment();
    asgn->setSymbol(speciesID.toStdString());
    asgn->setId(speciesID.toStdString() + "_initialConcentration");
    SPDLOG_INFO("  - creating new assignment: {}", asgn->getId());
  }
  std::unique_ptr<libsbml::ASTNode> argAST(
      libsbml::SBML_parseL3Formula(analyticExpression.toStdString().c_str()));
  asgn->setMath(argAST.get());
  setFieldConcAnalytic(mapSpeciesIdToField.at(speciesID),
                       analyticExpression.toStdString());
}

QString SbmlDocWrapper::getAnalyticConcentration(
    const QString &speciesID) const {
  const auto *asgn =
      model->getInitialAssignmentBySymbol(speciesID.toStdString());
  if (asgn != nullptr) {
    return ASTtoString(asgn->getMath()).c_str();
  }
  return QString();
}

std::string SbmlDocWrapper::getSpeciesSampledFieldInitialAssignment(
    const std::string &speciesID) const {
  // look for existing initialAssignment to a sampledField
  std::string sampledFieldID;
  if (model->getInitialAssignmentBySymbol(speciesID) != nullptr) {
    auto *asgn = model->getInitialAssignmentBySymbol(speciesID);
    if (asgn->getMath()->isName()) {
      std::string paramID = asgn->getMath()->getName();
      SPDLOG_INFO("  - found initialAssignment: {}", paramID);
      auto *param = model->getParameter(paramID);
      if (param != nullptr) {
        auto *spp = dynamic_cast<libsbml::SpatialParameterPlugin *>(
            param->getPlugin("spatial"));
        if (spp != nullptr) {
          const auto &ref = spp->getSpatialSymbolReference()->getSpatialRef();
          SPDLOG_INFO("  - found spatialSymbolReference: {}", ref);
          if (geom->getSampledField(ref) != nullptr) {
            sampledFieldID = ref;
            SPDLOG_INFO("  - this is a reference to a SampledField");
          }
        }
      }
    }
  }
  return sampledFieldID;
}

void SbmlDocWrapper::importConcentrationFromImage(const QString &speciesID,
                                                  const QString &filename) {
  SPDLOG_INFO("speciesID: {}", speciesID);
  QImage img;
  img.load(filename);
  auto &field = mapSpeciesIdToField.at(speciesID);
  field.importConcentration(img);
  std::vector<double> concentrationArray = field.getConcentrationArray();

  // look for existing initialAssignment to a sampledField
  std::string sampledFieldID =
      getSpeciesSampledFieldInitialAssignment(speciesID.toStdString());
  // if found, update existing sampledField with new concentration array
  if (!sampledFieldID.empty()) {
    auto *sf = geom->getSampledField(sampledFieldID);
    sf->setSamples(concentrationArray);
    sf->setNumSamples1(static_cast<int>(concentrationArray.size()));
    SPDLOG_INFO("  - set samples to array of length {}",
                concentrationArray.size());
  } else {
    auto *sf = geom->createSampledField();
    sf->setId(speciesID.toStdString() + "_initialConcentration");
    sf->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE);
    sf->setInterpolationType(
        libsbml::InterpolationKind_t::SPATIAL_INTERPOLATIONKIND_LINEAR);
    sf->setCompression(
        libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
    SPDLOG_INFO("  - creating SampledField: {}", sf->getId());
    sf->setSamples(concentrationArray);
    sf->setNumSamples1(static_cast<int>(concentrationArray.size()));
    SPDLOG_INFO("  - set samples to array of length {}",
                concentrationArray.size());
    auto *param = model->createParameter();
    param->setId(speciesID.toStdString() + "_initialConcentration");
    param->setConstant(true);
    SPDLOG_INFO("  - creating Parameter: {}", param->getId());
    auto *spp = dynamic_cast<libsbml::SpatialParameterPlugin *>(
        param->getPlugin("spatial"));
    auto *ssr = spp->createSpatialSymbolReference();
    ssr->setSpatialRef(sf->getId());
    SPDLOG_INFO("  - with spatialSymbolReference: {}", ssr->getSpatialRef());
    auto *asgn = model->createInitialAssignment();
    asgn->setSymbol(speciesID.toStdString());
    asgn->setId(speciesID.toStdString() + "_initialConcentration");
    std::unique_ptr<libsbml::ASTNode> argAST(
        libsbml::SBML_parseL3Formula(param->getId().c_str()));
    asgn->setMath(argAST.get());
    SPDLOG_INFO("  - creating initialAssignment: {}",
                asgn->getMath()->getName());
  }
}

QImage SbmlDocWrapper::getConcentrationImage(const QString &speciesID) const {
  if (!hasGeometry) {
    return compartmentImage;
  }
  return mapSpeciesIdToField.at(speciesID).getConcentrationImage();
}

void SbmlDocWrapper::setIsSpatial(const QString &speciesID, bool isSpatial) {
  mapSpeciesIdToField.at(speciesID).isSpatial = isSpatial;
  auto *spec = model->getSpecies(speciesID.toStdString());
  auto *ssp =
      dynamic_cast<libsbml::SpatialSpeciesPlugin *>(spec->getPlugin("spatial"));
  if (ssp != nullptr) {
    ssp->setIsSpatial(isSpatial);
  }
}

bool SbmlDocWrapper::getIsSpatial(const QString &speciesID) const {
  return mapSpeciesIdToField.at(speciesID).isSpatial;
}

void SbmlDocWrapper::setDiffusionConstant(const QString &speciesID,
                                          double diffusionConstant) {
  bool diffConstantExists = false;
  for (unsigned i = 0; i < model->getNumParameters(); ++i) {
    auto *param = model->getParameter(i);
    auto *pplugin = dynamic_cast<libsbml::SpatialParameterPlugin *>(
        param->getPlugin("spatial"));
    // iterate over diff coeff params
    if (pplugin != nullptr && pplugin->isSetDiffusionCoefficient()) {
      auto *diffcoeff = pplugin->getDiffusionCoefficient();
      if (diffcoeff->getVariable() == speciesID.toStdString()) {
        diffConstantExists = true;
        param->setValue(diffusionConstant);
        SPDLOG_INFO("Setting diffusion constant:");
        SPDLOG_INFO("  - speciesID: {}", speciesID);
        SPDLOG_INFO("  - paramID: {}", param->getId());
        SPDLOG_INFO("  - new value: {}", param->getValue());
      }
    }
  }
  if (!diffConstantExists) {
    auto *param = model->createParameter();
    param->setConstant(true);
    // todo: first check for ID name clash
    param->setId(speciesID.toStdString() + "_diffusionConstant");
    param->setValue(diffusionConstant);
    auto *pplugin = dynamic_cast<libsbml::SpatialParameterPlugin *>(
        param->getPlugin("spatial"));
    auto *diffCoeff = pplugin->createDiffusionCoefficient();
    diffCoeff->setVariable(speciesID.toStdString());
    diffCoeff->setType(
        libsbml::DiffusionKind_t::SPATIAL_DIFFUSIONKIND_ISOTROPIC);
    SPDLOG_INFO("Setting new diffusion constant:");
    SPDLOG_INFO("  - speciesID: {}", speciesID);
    SPDLOG_INFO("  - paramID: {}", param->getId());
    SPDLOG_INFO("  - new value: {}", param->getValue());
  }
  mapSpeciesIdToField.at(speciesID).diffusionConstant = diffusionConstant;
}

double SbmlDocWrapper::getDiffusionConstant(const QString &speciesID) const {
  return mapSpeciesIdToField.at(speciesID).diffusionConstant;
}

void SbmlDocWrapper::setInitialConcentration(const QString &speciesID,
                                             double concentration) {
  mapSpeciesIdToField.at(speciesID).setUniformConcentration(concentration);
  model->getSpecies(speciesID.toStdString())
      ->setInitialConcentration(concentration);
}

double SbmlDocWrapper::getInitialConcentration(const QString &speciesID) const {
  return model->getSpecies(speciesID.toStdString())->getInitialConcentration();
}

void SbmlDocWrapper::setSpeciesColour(const QString &speciesID,
                                      const QColor &colour) {
  mapSpeciesIdToField.at(speciesID).colour = colour;
}

const QColor &SbmlDocWrapper::getSpeciesColour(const QString &speciesID) const {
  return mapSpeciesIdToField.at(speciesID).colour;
}

void SbmlDocWrapper::setIsSpeciesConstant(const std::string &speciesID,
                                          bool constant) {
  auto *spec = model->getSpecies(speciesID);
  spec->setConstant(constant);
  // todo: think about how to deal with boundaryCondition properly
  // for now, set it to false here: species is either constant, or not
  // todo: check if "constant" refers to constant concentration or amount?
  // what happens to a constant species when compartment size is changed?
  spec->setBoundaryCondition(false);
}

bool SbmlDocWrapper::getIsSpeciesConstant(const std::string &speciesID) const {
  auto *spec = model->getSpecies(speciesID);
  if (spec->isSetConstant() && spec->getConstant()) {
    // `Constant` species is a constant:
    //  - cannot be altered by Reactions or by RateRules
    return true;
  }
  if ((spec->isSetBoundaryCondition() && spec->getBoundaryCondition()) &&
      model->getRateRule(speciesID) == nullptr) {
    // `BoundaryCondition` species is a constant:
    //   - unless it is altered by a RateRule
    return true;
  }
  return false;
}

bool SbmlDocWrapper::isSpeciesReactive(const std::string &speciesID) const {
  // true if this species should have a PDE generated for it
  // by the Reactions that involve it
  auto *spec = model->getSpecies(speciesID);
  if ((spec->isSetConstant() && spec->getConstant()) ||
      (spec->isSetBoundaryCondition() && spec->getBoundaryCondition())) {
    return false;
  }
  return true;
}

std::map<std::string, double> SbmlDocWrapper::getGlobalConstants() const {
  std::map<std::string, double> constants;
  // add all *constant* species as constants
  for (unsigned k = 0; k < model->getNumSpecies(); ++k) {
    const auto *spec = model->getSpecies(k);
    if (getIsSpeciesConstant(spec->getId())) {
      SPDLOG_TRACE("found constant species {}", spec->getId());
      // todo: check if species is *also* non-spatial - work out what constant
      // spatial means, boundaryCondition spatial, etc...
      double init_conc = 0;
      // if SBML file specifies amount: convert to concentration
      if (spec->isSetInitialAmount()) {
        double amount = spec->getInitialAmount();
        double vol = model->getCompartment(spec->getCompartment())->getSize();
        init_conc = amount / vol;
        SPDLOG_INFO(
            "converting amount {} to concentration {} by dividing by vol {}",
            amount, init_conc, vol);
      } else {
        init_conc = spec->getInitialConcentration();
      }
      constants[spec->getId()] = init_conc;
      SPDLOG_TRACE("parameter {} = {}", spec->getId(), init_conc);
    }
  }
  // add any parameters (that are not replaced by an AssignmentRule)
  for (unsigned k = 0; k < model->getNumParameters(); ++k) {
    const auto *param = model->getParameter(k);
    if (model->getAssignmentRule(param->getId()) == nullptr) {
      SPDLOG_TRACE("parameter {} = {}", param->getId(), param->getValue());
      constants[param->getId()] = param->getValue();
    }
  }
  // also get compartment volumes (the compartmentID may be used in the reaction
  // equation, and it should be replaced with the value of the "Size"
  // parameter for this compartment)
  for (unsigned int k = 0; k < model->getNumCompartments(); ++k) {
    const auto *comp = model->getCompartment(k);
    SPDLOG_TRACE("parameter {} = {}", comp->getId(), comp->getSize());
    constants[comp->getId()] = comp->getSize();
  }
  // remove x and y if present, as these are not really parameters
  // (we want them to remain as variables to be parsed by symbolic parser)
  for (const auto &c : {"x", "y"}) {
    auto iter = constants.find(c);
    if (iter != constants.cend()) {
      constants.erase(iter);
    }
  }
  return constants;
}

double SbmlDocWrapper::getPixelWidth() const { return pixelWidth; }

void SbmlDocWrapper::setPixelWidth(double width, bool resizeCompartments) {
  pixelWidth = width;
  // update pixelWidth for each compartment
  for (auto &pair : mapCompIdToGeometry) {
    pair.second.pixelWidth = width;
  }
  if (resizeCompartments) {
    // update compartment Size based on pixel count & pixel size
    for (unsigned int k = 0; k < model->getNumCompartments(); ++k) {
      auto *comp = model->getCompartment(k);
      SPDLOG_INFO("compartmentID {}", comp->getId());
      SPDLOG_INFO("  - previous size: {}", comp->getSize());
      std::size_t nPixels =
          mapCompIdToGeometry.at(comp->getId().c_str()).ix.size();
      SPDLOG_INFO("  - number of pixels: {}", nPixels);
      double pixelArea = pixelWidth * pixelWidth;
      SPDLOG_INFO("  - pixel area (width*width): {}", pixelArea);
      double newSize = static_cast<double>(nPixels) * pixelArea;
      comp->setSize(newSize);
      SPDLOG_INFO("  - new size: {}", comp->getSize());
    }
  }
  SPDLOG_DEBUG("New pixel width = {}", pixelWidth);
  mesh.setPhysicalGeometry(width, origin);
  // update xy coordinates
  auto *coord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_X);
  auto *min = coord->getBoundaryMin();
  auto *max = coord->getBoundaryMax();
  max->setValue(origin.x() +
                pixelWidth * static_cast<double>(compartmentImage.width()));
  SPDLOG_INFO("  - x now in range [{},{}]", min->getValue(), max->getValue());
  coord = geom->getCoordinateComponentByKind(
      libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  min = coord->getBoundaryMin();
  max = coord->getBoundaryMax();
  max->setValue(origin.y() +
                pixelWidth * static_cast<double>(compartmentImage.height()));
  SPDLOG_INFO("  - y now in range [{},{}]", min->getValue(), max->getValue());
}

std::string SbmlDocWrapper::inlineExpr(
    const std::string &mathExpression) const {
  std::string inlined;
  // inline any Function calls in expr
  inlined = inlineFunctions(mathExpression);
  // inline any Assignment Rules in expr
  inlined = inlineAssignments(inlined);
  return inlined;
}

std::string SbmlDocWrapper::inlineFunctions(
    const std::string &mathExpression) const {
  std::string expr = mathExpression;
  SPDLOG_DEBUG("inlining {}", expr);
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    // get copy of function body as AST node
    std::unique_ptr<libsbml::ASTNode> funcBody(func->getBody()->deepCopy());
    // search for function call in expression
    std::string funcCallString = func->getId() + "(";
    auto loc = expr.find(funcCallString);
    auto fn_loc = loc;
    while (loc != std::string::npos) {
      // function call found
      fn_loc = loc;
      loc += func->getId().size() + 1;
      for (unsigned int j = 0; j < func->getNumArguments(); ++j) {
        // compare each argument used in the function call (arg)
        // to the corresponding variable in the function definition
        while (expr[loc] == ' ') {
          // trim any leading spaces
          ++loc;
        }
        auto arg_len = expr.find_first_of(",)", loc + 1) - loc;
        std::string arg = expr.substr(loc, arg_len);
        if (func->getArgument(j)->getName() != arg) {
          // create desired new argument as AST node
          std::unique_ptr<libsbml::ASTNode> argAST(
              libsbml::SBML_parseL3Formula(arg.c_str()));
          // replace existing argument with new argument
          funcBody->replaceArgument(func->getArgument(j)->getName(),
                                    argAST.get());
        }
        loc += arg_len + 1;
      }
      // replace function call with inlined body of function
      std::string funcBodyString = ASTtoString(funcBody.get());
      // wrap function body in parentheses
      std::string pre_expr = expr.substr(0, fn_loc);
      std::string post_expr = expr.substr(loc);
      expr = pre_expr + "(" + funcBodyString + ")" + post_expr;
      // go to end of inlined function body in expr
      loc = fn_loc + funcBodyString.size() + 2;
      SPDLOG_DEBUG("  - new expr = {}", expr);
      // search for next call to same function in expr
      loc = expr.find(funcCallString, loc);
    }
  }
  return expr;
}

std::string SbmlDocWrapper::inlineAssignments(
    const std::string &mathExpression) const {
  const std::string delimeters = "()-^*/+, ";
  std::string expr = mathExpression;
  std::string old_expr;
  SPDLOG_DEBUG("inlining {}", expr);
  // iterate through names in expression
  // where names are things in between any of these chars:
  // "()^*/+, "
  // http://sbml.org/Special/Software/libSBML/docs/formatted/cpp-api/class_a_s_t_node.html
  while (expr != old_expr) {
    old_expr = expr;
    auto start = expr.find_first_not_of(delimeters);
    while (start != std::string::npos) {
      auto end = expr.find_first_of(delimeters, start);
      std::string name = expr.substr(start, end - start);
      const auto *assignment = model->getAssignmentRule(name);
      if (assignment != nullptr) {
        // replace name with inlined body of Assignment rule
        const std::string &assignmentBody =
            model->getAssignmentRule(name)->getFormula();
        // wrap function body in parentheses
        std::string pre_expr = expr.substr(0, start);
        std::string post_expr = expr.substr(end);
        expr = pre_expr + "(" + assignmentBody + ")" + post_expr;
        SPDLOG_DEBUG("  - new expr = {}", expr);
        // go to end of inlined assignment body in expr
        end = start + assignmentBody.size() + 2;
      }
      start = expr.find_first_not_of(delimeters, end);
    }
  }
  return expr;
}

}  // namespace sbml
