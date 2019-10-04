// Partial Differential Equation (PDE) class
//  - construct PDE for given compartment or membrane
//  - PDE equation + Jacobian for each species as infix strings
//  - uses Reaction class to construct equations
//  - then Symbolic class to substitute constants, differentiate and simplify

#pragma once

#include <string>
#include <vector>

#include "sbml.hpp"

namespace pde {

class PDE {
 private:
  std::vector<std::string> species;
  std::vector<std::string> rhs;
  std::vector<std::vector<std::string>> jacobian;

 public:
  explicit PDE(const sbml::SbmlDocWrapper *doc_ptr,
               const std::vector<std::string> &speciesIDs,
               const std::vector<std::string> &reactionIDs,
               const std::vector<std::string> &relabelledSpeciesIDs = {},
               const std::vector<std::string> &reactionScaleFactors = {});
  const std::vector<std::string> &getRHS() const;
  const std::vector<std::vector<std::string>> &getJacobian() const;
};

}  // namespace pde
