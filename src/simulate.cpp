#include "simulate.h"

void simulate::compile_reactions() {
  // init vector of species with zeros
  species_values = std::vector<double>(doc.model->getNumSpecies(), 0.0);
  // init matrix M with zeros, matrix size: n_species x n_reactions
  M.clear();
  for (unsigned int i = 0; i < doc.model->getNumSpecies(); ++i) {
    M.emplace_back(std::vector<double>(doc.model->getNumReactions(), 0));
  }
  reac_eval.reserve(doc.model->getNumReactions());
  // process each reaction
  for (unsigned int j = 0; j < doc.model->getNumReactions(); ++j) {
    const auto *reac = doc.model->getReaction(j);
    // get mathematical formula
    const auto *kin = reac->getKineticLaw();
    std::string expr = kin->getFormula();

    // inline function calls
    qDebug() << expr.c_str();
    expr = inline_functions(expr);
    qDebug() << "after inlining:";
    qDebug() << expr.c_str();

    // get all parameters and constants used in reaction
    std::vector<std::string> constant_names;
    std::vector<double> constant_values;
    // get local parameters and their values
    for (unsigned k = 0; k < kin->getNumLocalParameters(); ++k) {
      constant_names.push_back(kin->getLocalParameter(k)->getId());
      constant_values.push_back(kin->getLocalParameter(k)->getValue());
    }
    for (unsigned k = 0; k < kin->getNumParameters(); ++k) {
      constant_names.push_back(kin->getParameter(k)->getId());
      constant_values.push_back(kin->getParameter(k)->getValue());
    }
    // get global parameters and their values:
    for (unsigned k = 0; k < doc.model->getNumParameters(); ++k) {
      constant_names.push_back(doc.model->getParameter(k)->getId());
      constant_values.push_back(doc.model->getParameter(k)->getValue());
    }
    // get compartment volumes (the compartmentID may be used in the reaction
    // equation, and it should be replaced with the value of the "Size"
    // parameter for this compartment )
    for (unsigned int k = 0; k < doc.model->getNumCompartments(); ++k) {
      const auto *comp = doc.model->getCompartment(k);
      constant_names.push_back(comp->getId());
      constant_values.push_back(comp->getSize());
    }

    for (std::size_t k = 0; k < constant_names.size(); ++k) {
      qDebug("const: %s %f", constant_names[k].c_str(), constant_values[k]);
    }

    // compile expression and add to reac_eval vector
    reac_eval.emplace_back(numerics::reaction_eval(
        expr, doc.speciesID, species_values, constant_names, constant_values));

    // add difference of stochiometric coefficients to matrix M for each species
    // produced by this reaction
    for (unsigned k = 0; k < reac->getNumProducts(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getProduct(k);
      // convert species ID to species index i
      std::size_t i = doc.speciesIndex.at(spec_ref->getSpecies().c_str());
      // add stoichiometric coefficient at (i,j) in matrix M
      M[i][j] += spec_ref->getStoichiometry();
    }
    // add a -1 to matrix M for each species consumed by this reaction
    for (unsigned k = 0; k < reac->getNumReactants(); ++k) {
      // get product species reference
      const auto *spec_ref = reac->getReactant(k);
      // convert species ID to species index i
      std::size_t i = doc.speciesIndex.at(spec_ref->getSpecies().c_str());
      // subtract stoichiometric coefficient at (i,j) in matrix M
      M[i][j] -= spec_ref->getStoichiometry();
    }
  }
}

std::string simulate::inline_functions(const std::string &expr) {
  std::string expr_inlined = expr;
  for (unsigned int i = 0; i < doc.model->getNumFunctionDefinitions(); ++i) {
    const auto *func = doc.model->getFunctionDefinition(i);
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
          // if they differ, replace the variable in the function def with the
          // argument used in expr
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

std::vector<double> simulate::evaluate_reactions() {
  std::vector<double> result(species_values.size(), 0.0);
  for (std::size_t i = 0; i < M.size(); ++i) {
    for (std::size_t j = 0; j < reac_eval.size(); ++j) {
      result[i] += M[i][j] * reac_eval[j]();
    }
  }
  return result;
}

void simulate::euler_timestep(double dt) {
  std::vector<double> dcdt = evaluate_reactions();
  for (std::size_t i = 0; i < species_values.size(); ++i) {
    if (!doc.model->getSpecies(static_cast<unsigned int>(i))->getConstant()) {
      species_values[i] += dcdt[i] * dt;
    }
  }
  qDebug() << species_values;
}
