#include "sbml.h"

#include <unordered_set>

void sbmlDocWrapper::importSBMLFile(const std::string &filename) {
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  hasGeometry = false;
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    isValid = false;
    // todo: doc->printErrors(stream)
  } else {
    isValid = true;
  }

  model = doc->getModel();

  // get list of compartments
  species.clear();
  compartments.clear();
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
      qDebug() << "Error: HasOnlySubstanceUnits=true not yet supported.";
      exit(1);
    }
    const auto id = spec->getId().c_str();
    species[spec->getCompartment().c_str()] << QString(id);
  }

  // get list of reactions
  reactions.clear();
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    const auto *reac = model->getReaction(i);
    reactions << QString(reac->getId().c_str());
  }

  // get list of functions
  functions.clear();
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    functions << QString(func->getId().c_str());
  }
}

void sbmlDocWrapper::importGeometryFromImage(const QString &filename) {
  compartmentImage.load(filename);
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
      qDebug("ColourPair iA,iB: %d,%d: %u,%u", iA, iB, std::min(colA, colB),
             std::max(colA, colB));
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
  hasGeometry = true;
}

const QImage &sbmlDocWrapper::getMembraneImage(const QString &membraneID) {
  if (!hasGeometry) {
    return compartmentImage;
  }
  return mapMembraneToImage.at(membraneID);
}

void sbmlDocWrapper::updateMembraneList() {
  // construct membrane list & images
  membranes.clear();
  mapMembraneToIndex.clear();
  mapMembraneToImage.clear();
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
          // generate membrane name
          QString name = compartments[i] + "-" + compartments[j];
          membranes.push_back(name);
          // map name to index of membrane location pairs
          mapMembraneToIndex[name] = index;
          // generate image
          QImage img = compartmentImage.convertToFormat(QImage::Format_ARGB32);
          for (const auto &pair : membranePairs[mapColPairToIndex.at(
                   {std::min(colA, colB), std::max(colA, colB)})]) {
            img.setPixel(pair.first, QColor(0, 155, 40, 255).rgba());
            img.setPixel(pair.second, QColor(0, 199, 40, 255).rgba());
          }
          mapMembraneToImage[name] = img;
        }
      }
    }
  }
}

const QImage &sbmlDocWrapper::getCompartmentImage() { return compartmentImage; }

QString sbmlDocWrapper::getCompartmentID(QRgb colour) const {
  auto iter = mapColourToCompartment.find(colour);
  if (iter == mapColourToCompartment.cend()) {
    return "";
  }
  return iter->second;
}

QRgb sbmlDocWrapper::getCompartmentColour(const QString &compartmentID) const {
  auto iter = mapCompartmentToColour.find(compartmentID);
  if (iter == mapCompartmentToColour.cend()) {
    return 0;
  }
  return iter->second;
}

void sbmlDocWrapper::setCompartmentColour(const QString &compartmentID,
                                          QRgb colour) {
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
  mapCompIdToGeometry[compartmentID] = Geometry();
  mapCompIdToGeometry[compartmentID].init(getCompartmentImage(), colour);
  std::vector<std::string> tmpSpeciesID;
  tmpSpeciesID.reserve(static_cast<std::size_t>(species[compartmentID].size()));
  for (const auto &s : species[compartmentID]) {
    tmpSpeciesID.push_back(s.toStdString());
  }
  mapCompIdToField[compartmentID] = Field();
  mapCompIdToField[compartmentID].init(&mapCompIdToGeometry[compartmentID],
                                       tmpSpeciesID);
  // set all species concentrations to their initial value at all
  // points
  for (int i = 0; i < species[compartmentID].size(); ++i) {
    mapCompIdToField[compartmentID].setConstantConcentration(
        static_cast<std::size_t>(i),
        model->getSpecies(species[compartmentID][i].toStdString())
            ->getInitialConcentration());
  }
  // update list of possible inter-compartment membranes
  updateMembraneList();
}

void sbmlDocWrapper::importConcentrationFromImage(const QString &speciesID,
                                                  const QString &filename) {
  QString comp =
      model->getSpecies(speciesID.toStdString())->getCompartment().c_str();
  QImage img;
  img.load(filename);
  mapCompIdToField[comp].importConcentration(speciesID.toStdString(), img);
}

const QImage &sbmlDocWrapper::getConcentrationImage(const QString &speciesID) {
  if (!hasGeometry) {
    return compartmentImage;
  }
  QString comp =
      model->getSpecies(speciesID.toStdString())->getCompartment().c_str();
  return mapCompIdToField[comp].getConcentrationImage(speciesID.toStdString());
}

QString sbmlDocWrapper::getXml() const {
  return libsbml::writeSBMLToString(doc.get());
}

std::string sbmlDocWrapper::inlineFunctions(
    const std::string &mathExpression) const {
  std::string expr = mathExpression;
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    // get copy of function body as AST node
    std::unique_ptr<libsbml::ASTNode> funcBody(func->getBody()->deepCopy());
    // search for function call in expression
    std::string funcCallString = func->getId() + "(";
    auto loc = expr.find(funcCallString);
    while (loc != std::string::npos) {
      // function call found
      loc += func->getId().size() + 1;
      for (unsigned int j = 0; j < func->getNumArguments(); ++j) {
        // compare each argument used in the function call in expr to the
        // variable in the function definition
        while (expr[loc] == ' ') {
          // trim any leading spaces
          ++loc;
        }
        auto arg_len = expr.find_first_of(",)", loc + 1) - loc;
        std::string arg = expr.substr(loc, arg_len);
        qDebug() << func->getArgument(j)->getName();
        qDebug() << arg.c_str();
        if (func->getArgument(j)->getName() != arg) {
          // create desired new argument as AST node
          std::unique_ptr<libsbml::ASTNode> argAST(
              libsbml::SBML_parseL3Formula(arg.c_str()));
          // replace existing argument with new argument
          funcBody->replaceArgument(func->getArgument(j)->getName(),
                                    argAST.get());
          qDebug("replacing %s with %s in function",
                 func->getArgument(j)->getName(), arg.c_str());
        }
        loc += arg_len + 1;
      }
      // replace function call with inlined body of function
      std::string funcBodyString =
          libsbml::SBML_formulaToL3String(funcBody.get());
      auto end = expr.find(")", loc);
      // wrap function body in parentheses
      expr = expr.substr(0, loc) + "(" + funcBodyString + ")" +
             expr.substr(end + 1);
      // search for next call to same function in expr
      loc += funcBodyString.size() + 2;
      loc = expr.find(funcCallString);
    }
  }
  return expr;
}
