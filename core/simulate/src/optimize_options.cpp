#include "sme/optimize_options.hpp"

namespace sme::simulate {

std::string toString(OptAlgorithmType optAlgorithmType) {
  using enum OptAlgorithmType;
  switch (optAlgorithmType) {
  case PSO:
    return "Particle Swarm Optimization";
  case GPSO:
    return "Generational Particle Swarm Optimization";
  case DE:
    return "Differential Evolution";
  case iDE:
    return "Self-adaptive Differential Evolution (iDE)";
  case jDE:
    return "Self-adaptive Differential Evolution (jDE)";
  case pDE:
    return "Self-adaptive Differential Evolution (pDE)";
  case ABC:
    return "Artificial Bee Colony";
  case gaco:
    return "Extended Ant Colony Optimization";
  case COBYLA:
    return "Constrained Optimization BY Linear Approximations";
  case BOBYQA:
    return "Bound Optimization BY Quadratic Approximations";
  case NMS:
    return "Nelder-Mead Simplex algorithm";
  case sbplx:
    return "Rowan's SUPLEX algorithm";
  case AL:
    return "Augmented Lagrangian method";
  case PRAXIS:
    return "Brent's principle axis method";
  default:
    return "";
  }
}

} // namespace sme::simulate
