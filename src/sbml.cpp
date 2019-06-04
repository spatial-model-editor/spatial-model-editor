#include "sbml.h"

void sbmlDocWrapper::loadFile(const std::string &filename) {
  doc.reset(libsbml::readSBMLFromFile(filename.c_str()));
  if (doc->getErrorLog()->getNumFailsWithSeverity(libsbml::LIBSBML_SEV_ERROR) >
      0) {
    isValid = false;
    // todo: doc->printErrors(stream)
  } else {
    isValid = true;
  }

  // write raw xml to SBML panel
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
    speciesIndex[id] = i;
    speciesID.push_back(id);
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

void sbmlDocWrapper::importGeometry(const QString &filename) {
  compartment_image.load(filename);
  // convert geometry image to 8-bit indexed format
  // each pixel points to an index in the colorTable
  // which contains an RGB value for each color in the image
  compartment_image =
      compartment_image.convertToFormat(QImage::Format_Indexed8);
}

std::string sbmlDocWrapper::inlineFunctions(const std::string &expr) const {
  std::string expr_inlined = expr;
  for (unsigned int i = 0; i < model->getNumFunctionDefinitions(); ++i) {
    const auto *func = model->getFunctionDefinition(i);
    std::unique_ptr<libsbml::ASTNode> func_with_args(
        func->getBody()->deepCopy());
    std::unique_ptr<libsbml::ASTNode> arg_as_ast;
    // search for function call in expression
    auto loc = expr_inlined.find(func->getId() + "(");
    if (loc != std::string::npos) {
      // function call found
      auto arg_loc = loc + func->getId().size() + 1;
      std::size_t arg_end;
      for (unsigned int j = 0; j < func->getNumArguments(); ++j) {
        // compare each argument used in the function call in expr to the
        // variable in the function definition
        while (expr_inlined[arg_loc] == ' ') {
          // trim any leading spaces
          ++arg_loc;
        }
        arg_end = expr_inlined.find_first_of(",)", arg_loc + 1);
        std::string arg = expr_inlined.substr(arg_loc, arg_end - arg_loc);
        qDebug() << func->getArgument(j)->getName();
        qDebug() << arg.c_str();
        if (func->getArgument(j)->getName() != arg) {
          // if they differ, replace the variable in the function def with
          // the argument used in expr
          arg_as_ast.reset(libsbml::SBML_parseL3Formula(arg.c_str()));
          func_with_args->replaceArgument(func->getArgument(j)->getName(),
                                          arg_as_ast.get());
          qDebug("replacing %s with %s in function",
                 func->getArgument(j)->getName(), arg.c_str());
        }
        arg_loc = arg_end + 1;
      }
      // inline body of function, wrapped in parentheses, in expression
      auto end = expr_inlined.find(")", loc);
      expr_inlined = expr_inlined.substr(0, loc) + "(" +
                     libsbml::SBML_formulaToL3String(func_with_args.get()) +
                     ")" + expr_inlined.substr(end + 1);
      // todo: allow for multiple calls to the same function
      // todo: check for case that ")" not found: invalid expression
    }
  }
  return expr_inlined;
}
