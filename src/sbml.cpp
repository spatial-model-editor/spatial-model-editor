#include "sbml.h"

#include <unordered_set>

void SbmlDocWrapper::importSBMLFile(const std::string &filename) {
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  hasGeometry = false;
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    isValid = false;
    // doc->printErrors();
    qDebug(
        "sbmlDocWrapper::importSBMLFile :: Warning - errors while reading "
        "SBML file (continuing anyway...)");
    isValid = true;
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

  // get list of functions
  functions.clear();
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    functions << QString(func->getId().c_str());
  }
}

void SbmlDocWrapper::importGeometryFromImage(const QString &filename) {
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
          // generate membrane name
          QString name = compartments[i] + "-" + compartments[j];
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
          Field *fieldA = &mapCompIdToField.at(compartments[i]);
          Field *fieldB = &mapCompIdToField.at(compartments[j]);
          if (colB < colA) {
            std::swap(fieldA, fieldB);
          }
          membraneVec.emplace_back(name.toStdString(), fieldA, fieldB,
                                   membranePairs[index]);
        }
      }
    }
  }
}

void SbmlDocWrapper::updateReactionList() {
  reactions.clear();
  for (unsigned int i = 0; i < model->getNumReactions(); ++i) {
    const auto *reac = model->getReaction(i);
    QString reacID = reac->getId().c_str();
    // get set of compartments where reaction takes place
    std::unordered_set<std::string> comps;
    for (unsigned int k = 0; k < reac->getNumProducts(); ++k) {
      const std::string &specID = reac->getProduct(k)->getSpecies();
      comps.insert(model->getSpecies(specID)->getCompartment());
    }
    // TODO: also include modifiers here??
    for (unsigned int k = 0; k < reac->getNumReactants(); ++k) {
      const std::string &specID = reac->getReactant(k)->getSpecies();
      comps.insert(model->getSpecies(specID)->getCompartment());
    }
    // single compartment
    if (comps.size() == 1) {
      QString comp = QString(QString(comps.begin()->c_str()));
      reactions[comp] << QString(reacID);
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
    } else {
      // invalid reaction
      qDebug("SbmlDocWrapper::updateReactionList :: Error: invalid reaction");
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
  mapCompIdToGeometry[compartmentID] =
      Compartment(compartmentID.toStdString(), getCompartmentImage(), colour);
  mapCompIdToField[compartmentID] = Field();
  mapCompIdToField[compartmentID].init(&mapCompIdToGeometry[compartmentID],
                                       species[compartmentID]);
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
  // update list of reactions for each compartment/membrane
  updateReactionList();
}

void SbmlDocWrapper::importConcentrationFromImage(const QString &speciesID,
                                                  const QString &filename) {
  QString comp =
      model->getSpecies(speciesID.toStdString())->getCompartment().c_str();
  QImage img;
  img.load(filename);
  mapCompIdToField.at(comp).importConcentration(speciesID.toStdString(), img);
}

const QImage &SbmlDocWrapper::getConcentrationImage(const QString &speciesID) {
  if (!hasGeometry) {
    return compartmentImage;
  }
  QString comp =
      model->getSpecies(speciesID.toStdString())->getCompartment().c_str();
  return mapCompIdToField.at(comp).getConcentrationImage(
      speciesID.toStdString());
}

QString SbmlDocWrapper::getXml() const {
  return libsbml::writeSBMLToString(doc.get());
}

std::string SbmlDocWrapper::inlineFunctions(
    const std::string &mathExpression) const {
  std::string expr = mathExpression;
  qDebug("SbmlDocWrapper::inlineFunctions :: inlining %s", expr.c_str());
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    // get copy of function body as AST node
    std::unique_ptr<libsbml::ASTNode> funcBody(func->getBody()->deepCopy());
    // search for function call in expression
    std::string funcCallString = func->getId() + "(";
    auto loc = expr.find(funcCallString);
    auto fn_loc = loc;
    qDebug("SbmlDocWrapper::inlineFunctions :: searching for function '%s'",
           func->getId().c_str());
    while (loc != std::string::npos) {
      // function call found
      fn_loc = loc;
      qDebug("SbmlDocWrapper::inlineFunctions ::   -> found at %lu", fn_loc);
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
        qDebug("SbmlDocWrapper::inlineFunctions ::   - arg = %s", arg.c_str());
        qDebug("SbmlDocWrapper::inlineFunctions ::   - def = %s",
               func->getArgument(j)->getName());
        if (func->getArgument(j)->getName() != arg) {
          // create desired new argument as AST node
          std::unique_ptr<libsbml::ASTNode> argAST(
              libsbml::SBML_parseL3Formula(arg.c_str()));
          // replace existing argument with new argument
          funcBody->replaceArgument(func->getArgument(j)->getName(),
                                    argAST.get());
          qDebug("SbmlDocWrapper::inlineFunctions ::      - %s -> %s",
                 func->getArgument(j)->getName(), arg.c_str());
        }
        loc += arg_len + 1;
      }
      // replace function call with inlined body of function
      std::string funcBodyString =
          libsbml::SBML_formulaToL3String(funcBody.get());
      // wrap function body in parentheses
      std::string pre_expr = expr.substr(0, fn_loc);
      qDebug("SbmlDocWrapper::inlineFunctions ::   - pre expr[0,%lu] = %s",
             fn_loc, pre_expr.c_str());
      std::string post_expr = expr.substr(loc);
      qDebug("SbmlDocWrapper::inlineFunctions ::   - post expr[%lu,] = %s", loc,
             post_expr.c_str());
      expr = pre_expr + "(" + funcBodyString + ")" + post_expr;
      // go to end of inlined function body in expr
      loc = fn_loc + funcBodyString.size() + 2;
      qDebug("SbmlDocWrapper::inlineFunctions ::   - new expr = %s",
             expr.c_str());
      qDebug("SbmlDocWrapper::inlineFunctions ::   - new loc = %lu, %s", loc,
             &expr[loc]);
      // search for next call to same function in expr
      loc = expr.find(funcCallString, loc);
    }
  }
  return expr;
}

std::string SbmlDocWrapper::inlineAssignments(
    const std::string &mathExpression) const {
  std::string delimeters = "()-^*/+, ";
  std::string expr = mathExpression;
  qDebug("SbmlDocWrapper::inlineAssignments :: inlining %s", expr.c_str());
  // iterate through names in expression
  // Assuming that names are things in between any of these chars:
  // "()^*/+, "
  // http://sbml.org/Special/Software/libSBML/docs/formatted/cpp-api/class_a_s_t_node.html
  auto start = expr.find_first_not_of(delimeters);
  while (start != std::string::npos) {
    auto end = expr.find_first_of(delimeters, start);
    std::string name = expr.substr(start, end - start);
    qDebug("SbmlDocWrapper::inlineAssignments:: name '%s'", name.c_str());
    const auto *assignment = model->getAssignmentRule(name);
    if (assignment != nullptr) {
      // replace name with inlined body of Assignment rule
      std::string assignmentBody = model->getAssignmentRule(name)->getFormula();
      qDebug("SbmlDocWrapper::inlineAssignments:: -> %s",
             assignmentBody.c_str());
      // wrap function body in parentheses
      std::string pre_expr = expr.substr(0, start);
      qDebug("SbmlDocWrapper::inlineAssignments ::   - pre expr[0,%lu] = %s",
             start, pre_expr.c_str());
      std::string post_expr = expr.substr(end);
      qDebug("SbmlDocWrapper::inlineAssignments ::   - post expr[%lu,] = %s",
             end, post_expr.c_str());
      expr = pre_expr + "(" + assignmentBody + ")" + post_expr;
      qDebug("SbmlDocWrapper::inlineAssignments ::   - new expr = %s",
             expr.c_str());
      // go to end of inlined assignment body in expr
      end = start + assignmentBody.size() + 2;
    }
    start = expr.find_first_not_of(delimeters, end);
  }
  return expr;
}
