#include "sbml.hpp"

#include <sstream>
#include <unordered_set>

#include "logger.hpp"

namespace sbml {

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
}

void SbmlDocWrapper::importSBMLString(const std::string &xml) {
  clearAllModelData();
  doc.reset(libsbml::readSBMLFromString(xml.c_str()));
  spdlog::info(
      "SbmlDocWrapper::importSBMLFile :: importing SBML from string...");
  initModelData();
}

void SbmlDocWrapper::importSBMLFile(const std::string &filename) {
  clearAllModelData();
  spdlog::info("SbmlDocWrapper::importSBMLFile :: Loading SBML file {}...",
               filename);
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  initModelData();
}

void SbmlDocWrapper::initSpatialData() {
  spdlog::info(
      "SbmlDocWrapper::initSpatialData :: Creating new 2d SBML model "
      "geometry");
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
  auto *min = coord->createBoundaryMin();
  min->setId("xBoundaryMin");
  min->setValue(0);
  auto *max = coord->createBoundaryMax();
  max->setId("xBoundaryMax");
  max->setValue(1);
  coord = geom->getCoordinateComponent(1);
  coord->setType(libsbml::CoordinateKind_t::SPATIAL_COORDINATEKIND_CARTESIAN_Y);
  coord->setId("yCoord");
  min = coord->createBoundaryMin();
  min->setId("yBoundaryMin");
  min->setValue(0);
  max = coord->createBoundaryMax();
  max->setId("yBoundaryMax");
  max->setValue(1);

  // set isSpatial to false for all species
  for (unsigned i = 0; i < model->getNumSpecies(); ++i) {
    auto *ssp = dynamic_cast<libsbml::SpatialSpeciesPlugin *>(
        model->getSpecies(i)->getPlugin("spatial"));
    ssp->setIsSpatial(false);
  }

  // set isLocal to false for all reactions
  for (unsigned i = 0; i < model->getNumReactions(); ++i) {
    auto *srp = dynamic_cast<libsbml::SpatialReactionPlugin *>(
        model->getReaction(i)->getPlugin("spatial"));
    srp->setIsLocal(false);
  }

  // create sampled field geometry with empty SampledField
  sfgeom = geom->createSampledFieldGeometry();
  sfgeom->setId("geometry");
  sfgeom->setIsActive(true);
  auto *sf = geom->createSampledField();
  sf->setId("geometryImage");
  sf->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_UINT32);
  sf->setInterpolationType(
      libsbml::InterpolationKind_t::SPATIAL_INTERPOLATIONKIND_LINEAR);
  sf->setCompression(
      libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
  sfgeom->setSampledField(sf->getId());

  // for each compartment:
  //  - create DomainType
  //  - create CompartmentMapping from compartment to DomainType
  //  - create Domain with this DomainType
  //  - create SampledVolume with with DomainType
  for (unsigned i = 0; i < model->getNumCompartments(); ++i) {
    auto *comp = model->getCompartment(i);
    auto *scp = dynamic_cast<libsbml::SpatialCompartmentPlugin *>(
        comp->getPlugin("spatial"));
    const std::string &compartmentID = comp->getId();
    auto *dt = geom->createDomainType();
    dt->setId(compartmentID + "_domainType");
    dt->setSpatialDimensions(static_cast<int>(nDimensions));
    auto *cmap = scp->createCompartmentMapping();
    cmap->setId(compartmentID + "_compartmentMapping");
    cmap->setDomainType(dt->getId());
    cmap->setUnitSize(1.0);
    auto *dom = geom->createDomain();
    dom->setId(compartmentID + "_domain");
    dom->setDomainType(dt->getId());
    auto *sfvol = sfgeom->createSampledVolume();
    sfvol->setId(compartmentID + "_sampledVolume");
    sfvol->setDomainType(dt->getId());
  }
}

void SbmlDocWrapper::importSpatialData() {
  geom = plugin->getGeometry();
  spdlog::info(
      "SbmlDocWrapper::importSpatialData :: Importing existing {}d SBML model "
      "geometry",
      geom->getNumCoordinateComponents());
  if (geom->getNumCoordinateComponents() != nDimensions) {
    spdlog::critical(
        "SbmlDocWrapper::initModelData :: Error: Only {}d models are "
        "currently supported",
        nDimensions);
    // todo: offer to delete spatial part of model & import anyway
    qFatal(
        "SbmlDocWrapper::initModelData :: Error: Only 2d models are "
        "currently supported");
  }
  // get sampled field geometry
  sfgeom = nullptr;
  for (unsigned i = 0; i < geom->getNumGeometryDefinitions(); ++i) {
    auto *def = geom->getGeometryDefinition(i);
    if (def->getIsActive() && def->isSampledFieldGeometry()) {
      sfgeom = dynamic_cast<libsbml::SampledFieldGeometry *>(def);
    }
  }
  if (sfgeom == nullptr) {
    spdlog::critical(
        "SbmlDocWrapper::initModelData :: Error: Only sampled field "
        "geometries are currently supported");
    qFatal(
        "SbmlDocWrapper::initModelData :: Error: Only sampled field "
        "geometries are currently supported");
  }

  // import geometry image
  auto *sf = geom->getSampledField(sfgeom->getSampledField());
  int xVals = sf->getNumSamples1();
  int yVals = sf->getNumSamples2();
  int totalVals = sf->getSamplesLength();
  if (xVals * yVals != totalVals) {
    spdlog::critical(
        "SbmlDocWrapper::initModelData :: Error: interpolation not yet "
        "supported: should be one value for each pixel");
  }
  std::vector<int> samples;
  // this push-backs all ints in the samples array
  sf->getSamples(samples);
  // convert values into 2d pixmap
  // NOTE: assuming "x runs faster" ordering
  QImage img(xVals, yVals, QImage::Format_RGB32);
  auto iter = samples.begin();
  for (int y = 0; y < img.height(); ++y) {
    for (int x = 0; x < img.width(); ++x) {
      img.setPixel(x, y, static_cast<unsigned int>(*iter));
      ++iter;
    }
  }
  spdlog::info(
      "SbmlDocWrapper::importSpatialData ::   - found {} geometry image",
      img.size());
  importGeometryFromImage(img, false);

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
      spdlog::info(
          "SbmlDocWrapper::importSpatialData :: setting compartment {} colour "
          "to {:x}",
          compartmentID, col);
      spdlog::info("SbmlDocWrapper::importSpatialData ::   - DomainType: {}",
                   domainTypeID);
      spdlog::info(
          "SbmlDocWrapper::importSpatialData ::   - SampledFieldVolume: {}",
          sfvol->getId());
      setCompartmentColour(compartmentID, col, false);
    }
  }
}

void SbmlDocWrapper::initModelData() {
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    std::stringstream ss;
    doc->printErrors(ss);
    spdlog::warn("SBML document errors:\n\n{}", ss.str());
    spdlog::warn(
        "SbmlDocWrapper::initModelData :: Warning - errors while reading "
        "SBML file (continuing anyway...)");
    isValid = true;
  } else {
    spdlog::info(
        "SbmlDocWrapper::initModelData :: Successfully imported SBML Level {}, "
        "Version {} model",
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
      spdlog::critical(errorMessage);
      qFatal("%s", errorMessage.c_str());
    }
    const auto id = spec->getId().c_str();
    species[spec->getCompartment().c_str()] << QString(id);
    // assign a default colour for displaying the species
    mapSpeciesIdToColour[id] = defaultSpeciesColours()[i];
  }

  // get list of functions
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    functions << QString(func->getId().c_str());
  }

  // upgrade SBML document to latest version
  if (doc->setLevelAndVersion(libsbml::SBMLDocument::getDefaultLevel(),
                              libsbml::SBMLDocument::getDefaultVersion())) {
    spdlog::info(
        "SbmlDocWrapper::initModelData :: Successfully upgraded SBML model "
        "to Level {}, Version {}",
        doc->getLevel(), doc->getVersion());
  } else {
    spdlog::critical(
        "SbmlDocWrapper::initModelData :: Error - failed to upgrade SBML "
        "file (continuing anyway...)");
  }
  if (doc->getNumErrors() > 0) {
    std::stringstream ss;
    doc->printErrors(ss);
    spdlog::warn("SBML document warnings:\n\n{}", ss.str());
  }

  if (!doc->isPackageEnabled("spatial")) {
    doc->enablePackage(libsbml::SpatialExtension::getXmlnsL3V1V1(), "spatial",
                       true);
    doc->setPackageRequired("spatial", true);
    spdlog::info("SbmlDocWrapper::initModelData :: Enabling spatial extension");
  }

  plugin =
      dynamic_cast<libsbml::SpatialModelPlugin *>(model->getPlugin("spatial"));
  if (plugin == nullptr) {
    spdlog::warn(
        "SbmlDocWrapper::initModelData :: Failed to get SpatialModelPlugin "
        "from SBML document");
  }

  if (plugin->isSetGeometry()) {
    importSpatialData();
  } else {
    initSpatialData();
    // if we already had a geometry image, and we loaded a model without spatial
    // info, use this geometry image
    if (hasGeometry) {
      importGeometryFromImage(compartmentImage);
    }
  }
}

void SbmlDocWrapper::exportSBMLFile(const std::string &filename) const {
  if (isValid) {
    spdlog::info("SbmlDocWrapper::exportSBMLFile : exporting SBML model to {}",
                 filename);
    if (!libsbml::SBMLWriter().writeSBML(doc.get(), filename)) {
      spdlog::error("SbmlDocWrapper::exportSBMLFile : failed to write to %s",
                    filename);
    }
  }
}

void SbmlDocWrapper::importGeometryFromImage(const QImage &img,
                                             bool updateSBML) {
  clearAllGeometryData();
  compartmentImage = img;
  initMembraneColourPairs();
  if (isValid && updateSBML) {
    initSampledFieldGeometry();
  }
  hasGeometry = true;
}

void SbmlDocWrapper::importGeometryFromImage(const QString &filename,
                                             bool updateSBML) {
  clearAllGeometryData();
  compartmentImage.load(filename);
  initMembraneColourPairs();
  if (isValid && updateSBML) {
    initSampledFieldGeometry();
  }
  hasGeometry = true;
}

void SbmlDocWrapper::initSampledFieldGeometry() {
  // export image to SampledField
  auto *sf = geom->getSampledField(sfgeom->getSampledField());
  sf->setNumSamples1(compartmentImage.width());
  sf->setNumSamples2(compartmentImage.height());
  // todo: change from int to uint here if/when possible with libsbml?
  std::vector<int> samples;
  samples.reserve(static_cast<std::size_t>(compartmentImage.width() *
                                           compartmentImage.height()));
  // convert 2d pixmap into array of ints
  // NOTE: assuming "x runs faster" ordering
  for (int y = 0; y < compartmentImage.height(); ++y) {
    for (int x = 0; x < compartmentImage.width(); ++x) {
      samples.push_back(static_cast<int>(compartmentImage.pixel(x, y)));
    }
  }
  sf->setSamples(samples);
  spdlog::info(
      "SbmlDocWrapper::initSampledFieldGeometry :: SampledField '{}': "
      "assigned an array of length {}",
      sf->getId(), sf->getSamplesLength());
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
      spdlog::debug(
          "SbmlDocWrapper::importGeometryFromImage : ColourPair iA,iB: "
          "{},{}: "
          "{:x},{:x}",
          iA, iB, std::min(colA, colB), std::max(colA, colB));
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

const QImage &SbmlDocWrapper::getMembraneImage(const QString &membraneID) {
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
          QString name = compartments[i] + "-" + compartments[j];
          if (colA > colB) {
            name = compartments[j] + "-" + compartments[i];
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
      QString membraneID = compA + "-" + compB;
      if (colA > colB) {
        membraneID = compB + "-" + compA;
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
      spdlog::error(
          "SbmlDocWrapper::updateReactionList :: reaction involves {} "
          "compartments - not supported",
          comps.size());
      exit(1);
    }
  }
}

const QImage &SbmlDocWrapper::getCompartmentImage() { return compartmentImage; }

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
  const std::string fn("SbmlDocWrapper::setCompartmentColour");
  // todo: add check that colour exists in geometry image?
  QRgb oldColour = getCompartmentColour(compartmentID);
  if (oldColour != 0) {
    // if there was already a colour mapped to this compartment, map it to a
    // null compartment
    mapColourToCompartment[oldColour] = "";
  }
  auto oldCompartmentID = getCompartmentID(colour);
  if (oldCompartmentID != "") {
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
    mapSpeciesIdToField[s] = geometry::Field(&comp, s.toStdString(), 1.0,
                                             mapSpeciesIdToColour.at(s));
    geometry::Field &field = mapSpeciesIdToField.at(s);
    // set all species concentrations to their initial values
    auto *spec = model->getSpecies(s.toStdString());
    // todo: deal with case of initialAmount, or neither being set
    field.setUniformConcentration(spec->getInitialConcentration());
    // if an initial assingment exists, it takes precedence over
    // InitialConcentration set above
    // simplest case: formula is just the name of a SampledField
    // -> set field concentration using this SampledField of values
    // -> for now don't support anything else
    if (model->getInitialAssignmentBySymbol(s.toStdString()) != nullptr) {
      auto *asgn = model->getInitialAssignmentBySymbol(s.toStdString());
      if (asgn->getMath()->isName()) {
        std::string paramID = asgn->getMath()->getName();
        spdlog::info("{} ::   - found initialAssignment: {}", fn, paramID);
        auto *param = model->getParameter(paramID);
        if (param != nullptr) {
          auto *spp = dynamic_cast<libsbml::SpatialParameterPlugin *>(
              param->getPlugin("spatial"));
          if (spp != nullptr) {
            const std::string &sampledFieldID =
                spp->getSpatialSymbolReference()->getSpatialRef();
            spdlog::info("{} ::      - found spatialSymbolReference: {}", fn,
                         sampledFieldID);
            auto *sf = geom->getSampledField(sampledFieldID);
            std::vector<double> arrayConc;
            sf->getSamples(arrayConc);
            field.importConcentration(arrayConc);
          }
        }
      }
    }
    // set isSpatial flag
    auto *ssp = dynamic_cast<libsbml::SpatialSpeciesPlugin *>(
        spec->getPlugin("spatial"));
    if (ssp != nullptr && ssp->isSetIsSpatial() && ssp->getIsSpatial()) {
      field.isSpatial = true;
    } else {
      field.isSpatial = false;
    }
    // set diffusion constant value
    // todo: deal with potential non-isotropic case in SBML
    for (unsigned i = 0; i < model->getNumParameters(); ++i) {
      auto *param = model->getParameter(i);
      auto *pplugin = dynamic_cast<libsbml::SpatialParameterPlugin *>(
          param->getPlugin("spatial"));
      // iterate over diff coeff params
      if (pplugin != nullptr && pplugin->isSetDiffusionCoefficient()) {
        // check if diff coeff applies to our species
        auto *diffcoeff = pplugin->getDiffusionCoefficient();
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

void SbmlDocWrapper::importConcentrationFromImage(const QString &speciesID,
                                                  const QString &filename) {
  const std::string fn("SbmlDocWrapper::importConcentrationFromImage");
  spdlog::info("{} :: speciesID: {}", fn, speciesID);
  QImage img;
  img.load(filename);
  auto &field = mapSpeciesIdToField.at(speciesID);
  field.importConcentration(img);
  std::vector<double> concentrationArray = field.getConcentrationArray();

  // look for existing initialAssignment
  std::string sampledFieldID;
  if (model->getInitialAssignmentBySymbol(speciesID.toStdString()) != nullptr) {
    auto *asgn = model->getInitialAssignmentBySymbol(speciesID.toStdString());
    if (asgn->getMath()->isName()) {
      std::string paramID = asgn->getMath()->getName();
      spdlog::info("{} ::   - found initialAssignment: {}", fn, paramID);
      auto *param = model->getParameter(paramID);
      if (param != nullptr) {
        auto *spp = dynamic_cast<libsbml::SpatialParameterPlugin *>(
            param->getPlugin("spatial"));
        if (spp != nullptr) {
          sampledFieldID = spp->getSpatialSymbolReference()->getSpatialRef();
          spdlog::info("{} ::      - found spatialSymbolReference: {}", fn,
                       sampledFieldID);
        }
      }
    }
  }
  // if found, update existing sampledField with new concentration array
  if (!sampledFieldID.empty()) {
    auto *sf = geom->getSampledField(sampledFieldID);
    sf->setSamples(concentrationArray);
    sf->setNumSamples1(static_cast<int>(concentrationArray.size()));
    spdlog::info("{} ::   - set samples to array of length {}", fn,
                 concentrationArray.size());
  } else {
    auto *sf = geom->createSampledField();
    sf->setId(speciesID.toStdString() + "_initialConcentration");
    sf->setDataType(libsbml::DataKind_t::SPATIAL_DATAKIND_DOUBLE);
    sf->setInterpolationType(
        libsbml::InterpolationKind_t::SPATIAL_INTERPOLATIONKIND_LINEAR);
    sf->setCompression(
        libsbml::CompressionKind_t::SPATIAL_COMPRESSIONKIND_UNCOMPRESSED);
    spdlog::info("{} ::   - creating SampledField: {}", fn, sf->getId());
    sf->setSamples(concentrationArray);
    sf->setNumSamples1(static_cast<int>(concentrationArray.size()));
    spdlog::info("{} ::   - set samples to array of length {}", fn,
                 concentrationArray.size());
    auto *param = model->createParameter();
    param->setId(speciesID.toStdString() + "_initialConcentration");
    param->setConstant(true);
    spdlog::info("{} ::   - creating Parameter: {}", fn, param->getId());
    auto *spp = dynamic_cast<libsbml::SpatialParameterPlugin *>(
        param->getPlugin("spatial"));
    auto *ssr = spp->createSpatialSymbolReference();
    ssr->setSpatialRef(sf->getId());
    spdlog::info("{} ::   - with spatialSymbolReference: {}", fn,
                 ssr->getSpatialRef());
    auto *asgn = model->createInitialAssignment();
    asgn->setSymbol(speciesID.toStdString());
    asgn->setId(speciesID.toStdString() + "_initialConcentration");
    std::unique_ptr<libsbml::ASTNode> argAST(
        libsbml::SBML_parseL3Formula(param->getId().c_str()));
    asgn->setMath(argAST.get());
    spdlog::info("{} ::   - creating initialAssignment: {}", fn,
                 asgn->getMath()->getName());
  }
}

const QImage &SbmlDocWrapper::getConcentrationImage(const QString &speciesID) {
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
  const std::string fn("SbmlDocWrapper::setDiffusionConstant");
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
        spdlog::info("{} :: Setting diffusion constant:", fn);
        spdlog::info("{} ::   - speciesID: {}", fn, speciesID);
        spdlog::info("{} ::   - paramID: {}", fn, param->getId());
        spdlog::info("{} ::   - new value: {}", fn, param->getValue());
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
    spdlog::info("{} :: Setting new diffusion constant:", fn);
    spdlog::info("{} ::   - speciesID: {}", fn, speciesID);
    spdlog::info("{} ::   - paramID: {}", fn, param->getId());
    spdlog::info("{} ::   - new value: {}", fn, param->getValue());
  }
  mapSpeciesIdToField.at(speciesID).diffusionConstant = diffusionConstant;
}

double SbmlDocWrapper::getDiffusionConstant(const QString &speciesID) const {
  return mapSpeciesIdToField.at(speciesID).diffusionConstant;
}

void SbmlDocWrapper::setSpeciesColour(const QString &speciesID,
                                      const QColor &colour) {
  mapSpeciesIdToField.at(speciesID).colour = colour;
}

const QColor &SbmlDocWrapper::getSpeciesColour(const QString &speciesID) const {
  return mapSpeciesIdToField.at(speciesID).colour;
}

const QString &SbmlDocWrapper::getXml() {
  std::unique_ptr<char, decltype(&std::free)> xmlChar(
      libsbml::writeSBMLToString(doc.get()), &std::free);
  xmlString = QString(xmlChar.get());
  return xmlString;
}

bool SbmlDocWrapper::isSpeciesConstant(const std::string &speciesID) const {
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

std::string SbmlDocWrapper::inlineFunctions(
    const std::string &mathExpression) const {
  std::string expr = mathExpression;
  spdlog::debug("SbmlDocWrapper::inlineFunctions :: inlining {}", expr);
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
      std::unique_ptr<char, decltype(&std::free)> funcBodyChar(
          libsbml::SBML_formulaToL3String(funcBody.get()), &std::free);
      std::string funcBodyString(funcBodyChar.get());
      // wrap function body in parentheses
      std::string pre_expr = expr.substr(0, fn_loc);
      std::string post_expr = expr.substr(loc);
      expr = pre_expr + "(" + funcBodyString + ")" + post_expr;
      // go to end of inlined function body in expr
      loc = fn_loc + funcBodyString.size() + 2;
      spdlog::debug("SbmlDocWrapper::inlineFunctions ::   - new expr = {}",
                    expr);
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
  spdlog::debug("SbmlDocWrapper::inlineAssignments :: inlining {}", expr);
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
        spdlog::debug("SbmlDocWrapper::inlineAssignments ::   - new expr = {}",
                      expr);
        // go to end of inlined assignment body in expr
        end = start + assignmentBody.size() + 2;
      }
      start = expr.find_first_not_of(delimeters, end);
    }
  }
  return expr;
}

}  // namespace sbml
