// Shared helpers used by multiple pixel-simulator backends (CPU, CUDA, Metal).
// Header-only: everything here is backend-agnostic and small enough to inline.

#pragma once

#include "sme/geometry.hpp"
#include "sme/model.hpp"
#include "sme/simulate_options.hpp"
#include "sme/voxel.hpp"
#include <array>
#include <cstddef>
#include <fmt/core.h>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace sme::simulate::detail {

inline constexpr std::size_t invalidCompartmentIndex{
    std::numeric_limits<std::size_t>::max()};

inline constexpr std::array allMembraneFaceDirections{
    geometry::Membrane::FACE_DIRECTION::XP,
    geometry::Membrane::FACE_DIRECTION::XM,
    geometry::Membrane::FACE_DIRECTION::YP,
    geometry::Membrane::FACE_DIRECTION::YM,
    geometry::Membrane::FACE_DIRECTION::ZP,
    geometry::Membrane::FACE_DIRECTION::ZM};

// Forwards Euler stability bound for dimensionless diffusion constant
// {D/dx^2, D/dy^2, D/dz^2}
[[nodiscard]] inline double calculateMaxStableTimestep(
    const std::array<double, 3> &dimensionlessDiffusion) {
  return 1.0 / (2.0 * (dimensionlessDiffusion[0] + dimensionlessDiffusion[1] +
                       dimensionlessDiffusion[2]));
}

[[nodiscard]] inline double
getFaceFluxLength(geometry::Membrane::FACE_DIRECTION faceDirection,
                  const common::VolumeF &voxelSize) {
  switch (faceDirection) {
  case geometry::Membrane::FACE_DIRECTION::XP:
  case geometry::Membrane::FACE_DIRECTION::XM:
    return voxelSize.width();
  case geometry::Membrane::FACE_DIRECTION::YP:
  case geometry::Membrane::FACE_DIRECTION::YM:
    return voxelSize.height();
  case geometry::Membrane::FACE_DIRECTION::ZP:
  case geometry::Membrane::FACE_DIRECTION::ZM:
    return voxelSize.depth();
  }
  return voxelSize.width();
}

[[nodiscard]] inline bool hasAnyCrossDiffusion(
    const model::Model &doc,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds) {
  for (const auto &speciesIds : compartmentSpeciesIds) {
    for (const auto &targetSpeciesId : speciesIds) {
      for (const auto &sourceSpeciesId : speciesIds) {
        if (targetSpeciesId == sourceSpeciesId) {
          continue;
        }
        if (!doc.getSpecies()
                 .getCrossDiffusionConstant(targetSpeciesId.c_str(),
                                            sourceSpeciesId.c_str())
                 .isEmpty()) {
          return true;
        }
      }
    }
  }
  return false;
}

// Validates PoC-backend compartment inputs. Throws ErrorT with a message
// prefixed by `backendName` (e.g. "CUDA" or "Metal") when an unsupported
// feature is encountered.
template <typename ErrorT>
void validatePocCompartmentInputs(
    const model::Model &doc, const std::vector<std::string> &compartmentIds,
    const std::vector<std::vector<std::string>> &compartmentSpeciesIds,
    std::string_view backendName) {
  for (std::size_t compIndex = 0; compIndex < compartmentIds.size();
       ++compIndex) {
    const auto *compartment{doc.getCompartments().getCompartment(
        compartmentIds[compIndex].c_str())};
    if (compartment == nullptr) {
      throw ErrorT(
          fmt::format("{} pixel backend PoC could not find compartment '{}'",
                      backendName, compartmentIds[compIndex]));
    }
    for (const auto &speciesId : compartmentSpeciesIds[compIndex]) {
      const auto *field = doc.getSpecies().getField(speciesId.c_str());
      if (field == nullptr) {
        throw ErrorT(fmt::format(
            "{} pixel backend PoC could not find field for species '{}'",
            backendName, speciesId));
      }
      if (!field->getIsSpatial()) {
        throw ErrorT(
            fmt::format("{} pixel backend PoC does not yet support non-spatial "
                        "species",
                        backendName));
      }
      if (!field->getIsUniformDiffusionConstant()) {
        throw ErrorT(fmt::format(
            "{} pixel backend PoC does not yet support non-uniform diffusion "
            "constants",
            backendName));
      }
      if (doc.getSpecies().getStorage(speciesId.c_str()) != 1.0) {
        throw ErrorT(fmt::format(
            "{} pixel backend PoC currently requires unit storage for all "
            "species",
            backendName));
      }
    }
  }
}

// Adaptive RK error exponent. Only the adaptive schemes (RK212/323/435) call
// this; RK101 is never adaptive. Matches 1/(order of embedded estimate).
[[nodiscard]] inline double getErrorPower(PixelIntegratorType integ) {
  switch (integ) {
  case PixelIntegratorType::RK323:
    return 1.0 / 3.0;
  case PixelIntegratorType::RK435:
    return 1.0 / 4.0;
  default:
    return 1.0 / 2.0;
  }
}

// RK3(2)3: Shu Osher method with embedded Heun error estimate.
// https://doi.org/10.1016/0021-9991(88)90177-5 eq. (2.18)
namespace rk323 {
inline constexpr std::array<double, 3> g1{1.0, 0.25, 0.666666666666666666666};
inline constexpr std::array<double, 3> g2{0.0, 0.0, 0.0};
inline constexpr std::array<double, 3> g3{0.0, 0.75, 0.333333333333333333333};
inline constexpr std::array<double, 3> beta{1.0, 0.25, 0.6666666666666666666};
inline constexpr std::array<double, 3> delta{0.0, 0.0, 1.0};
inline constexpr std::array<double, 3> finaliseFactors{0.0, 2.0, -1.0};
} // namespace rk323

// RK4(3)5: 3S* algorithm 6 (5-stage RK4 with embedded RK3 error estimate).
// https://doi.org/10.1016/j.jcp.2009.11.006 table 6
namespace rk435 {
inline constexpr std::array<double, 5> g1{0.0, -0.497531095840104,
                                          1.010070514199942, -3.196559004608766,
                                          1.717835630267259};
inline constexpr std::array<double, 5> g2{1.0, 1.384996869124138,
                                          3.878155713328178, -2.324512951813145,
                                          -0.514633322274467};
inline constexpr std::array<double, 5> g3{0.0, 0.0, 0.0, 1.642598936063715,
                                          0.188295940828347};
inline constexpr std::array<double, 5> beta{
    0.075152045700771, 0.211361016946069, 1.100713347634329, 0.728537814675568,
    0.393172889823198};
inline constexpr std::array<double, 7> delta{1.0,
                                             0.081252332929194,
                                             -1.083849060586449,
                                             -1.096110881845602,
                                             2.859440022030827,
                                             -0.655568367959557,
                                             -0.194421504490852};
inline constexpr double deltaSumReciprocal =
    1.0 / (delta[0] + delta[1] + delta[2] + delta[3] + delta[4] + delta[5] +
           delta[6]);
} // namespace rk435

} // namespace sme::simulate::detail
