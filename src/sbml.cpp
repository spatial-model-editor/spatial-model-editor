#include "sbml.h"

void sbmlDocWrapper::importSBMLFile(const std::string &filename) {
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    isValid = false;
    // todo: doc->printErrors(stream)
  } else {
    isValid = true;
  }

  xml = libsbml::writeSBMLToString(doc.get());
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
  speciesIndex.clear();
  speciesID.clear();
  std::size_t idx = 0;
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
    // populate speciesID and speciesIndex for first compartment only
    if (spec->getCompartment().c_str() == compartments[0]) {
      speciesID.push_back(id);
      speciesIndex[id] = idx++;
    }
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
  // create compartment geometry
  geom.init(getCompartmentImage(), colour);
  field.init(&geom, static_cast<std::size_t>(species[compartmentID].size()));
  // initially set all species concentrations to their initial value at all
  // points
  for (int i = 0; i < species[compartmentID].size(); ++i) {
    field.setConstantConcentration(
        i, model->getSpecies(qPrintable(species[compartmentID][i]))
               ->getInitialConcentration());
  }
}

void sbmlDocWrapper::importConcentrationFromImage(const QString &speciesID,
                                                  const QString &filename) {
  QString comp =
      model->getSpecies(qPrintable(speciesID))->getCompartment().c_str();
  QImage img;
  img.load(filename);
  field.importConcentration(speciesIndex[qPrintable(speciesID)], img);
}

const QImage &sbmlDocWrapper::getConcentrationImage(const QString &speciesID) {
  QString comp =
      model->getSpecies(qPrintable(speciesID))->getCompartment().c_str();
  return field.getConcentrationImage(speciesIndex[qPrintable(speciesID)]);
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
