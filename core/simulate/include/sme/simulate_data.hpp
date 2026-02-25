// SimulatorData

#pragma once

#include "sme/simulate_options.hpp"
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <string>
#include <vector>

namespace sme::simulate {

/**
 * @brief Stored simulation outputs across timepoints.
 */
class SimulationData {
public:
  /**
   * @brief Simulated time values.
   */
  std::vector<double> timePoints;
  /**
   * @brief Concentrations:
   * ``time -> compartment -> flattened(voxel,species)``.
   */
  std::vector<std::vector<std::vector<double>>> concentration;
  /**
   * @brief Average/min/max by ``time -> compartment -> species``.
   */
  std::vector<std::vector<std::vector<AvgMinMax>>> avgMinMax;
  /**
   * @brief Concentration maxima by ``time -> compartment -> species``.
   */
  std::vector<std::vector<std::vector<double>>> concentrationMax;
  /**
   * @brief Padding values for flattened concentration arrays per timepoint.
   */
  std::vector<std::size_t> concPadding;
  /**
   * @brief Serialized XML of model this data belongs to.
   */
  std::string xmlModel;
  /**
   * @brief Clear all stored data.
   */
  void clear();
  /**
   * @brief Number of stored timepoints.
   */
  [[nodiscard]] std::size_t size() const;
  /**
   * @brief Estimated memory usage of currently stored data.
   */
  [[nodiscard]] std::size_t getEstimatedMemoryBytes() const;
  /**
   * @brief Estimated additional memory for extra timepoints.
   */
  [[nodiscard]] std::size_t
  getEstimatedAdditionalMemoryBytes(std::size_t nAdditionalTimepoints) const;
  /**
   * @brief Reserve storage for ``n`` timepoints.
   */
  void reserve(std::size_t n);
  /**
   * @brief Remove last stored timepoint.
   */
  void pop_back();

  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version) {
    if (version == 0) {
      ar(timePoints, concentration, avgMinMax, concentrationMax, concPadding,
         xmlModel);
    }
  }
};

} // namespace sme::simulate

CEREAL_CLASS_VERSION(sme::simulate::SimulationData, 0);
