// DUNE-copasi ini file generation
//  - DuneConverter: construct DUNE ini file from model

#pragma once

#include "sme/simulate_options.hpp"
#include "sme/voxel.hpp"
#include <QPoint>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <map>
#include <unordered_map>
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace mesh {
class Mesh2d;
class Mesh3d;
} // namespace mesh

namespace simulate {

/**
 * @brief Convert model data into DUNE configuration and arrays.
 */
class DuneConverter {
public:
  /**
   * @brief Build DUNE ini and associated arrays from model.
   * @param model Source model.
   * @param substitutions Constant substitutions.
   * @param forExternalUse Generate output suitable for external execution.
   * @param outputIniFile Optional output ini filename.
   * @param doublePrecision Decimal precision for floating values.
   */
  explicit DuneConverter(
      const model::Model &model,
      const std::map<std::string, double, std::less<>> &substitutions = {},
      bool forExternalUse = false, const QString &outputIniFile = {},
      int doublePrecision = 18);
  /**
   * @brief Generated DUNE ini file text.
   * @returns INI file text.
   */
  [[nodiscard]] QString getIniFile() const;

  /**
   * @brief 2D mesh used for conversion (nullable).
   * @returns 2D mesh pointer or ``nullptr``.
   */
  [[nodiscard]] const mesh::Mesh2d *getMesh() const;
  /**
   * @brief 3D mesh used for conversion (nullable).
   * @returns 3D mesh pointer or ``nullptr``.
   */
  [[nodiscard]] const mesh::Mesh3d *getMesh3d() const;
  /**
   * @brief Initial concentration arrays keyed by species name.
   * @returns Concentration arrays.
   */
  [[nodiscard]] const std::unordered_map<std::string, std::vector<double>> &
  getConcentrations() const;
  /**
   * @brief Diffusion-constant arrays keyed by species name.
   * @returns Diffusion arrays.
   */
  [[nodiscard]] const std::unordered_map<std::string, std::vector<double>> &
  getDiffusionConstantArrays() const;
  /**
   * @brief Species names grouped by compartment id.
   * @returns Species names per compartment.
   */
  [[nodiscard]] const std::unordered_map<std::string,
                                         std::vector<std::string>> &
  getSpeciesNames() const;
  /**
   * @brief Compartment ids in subdomain order.
   * @returns Compartment ids.
   */
  [[nodiscard]] const std::vector<std::string> &getCompartmentNames() const;
  /**
   * @brief Physical origin.
   * @returns Geometry origin.
   */
  [[nodiscard]] common::VoxelF getOrigin() const;
  /**
   * @brief Physical voxel size.
   * @returns Voxel size.
   */
  [[nodiscard]] common::VolumeF getVoxelSize() const;
  /**
   * @brief Geometry image size.
   * @returns Image size.
   */
  [[nodiscard]] common::Volume getImageSize() const;

private:
  QString iniFile;
  const mesh::Mesh2d *mesh;
  const mesh::Mesh3d *mesh3d;
  std::unordered_map<std::string, std::vector<double>> concentrations;
  std::unordered_map<std::string, std::vector<double>> diffusionConstantArrays;
  std::unordered_map<std::string, std::vector<std::string>> speciesNames;
  std::vector<std::string> compartmentNames;
  common::VoxelF origin;
  common::VolumeF voxelSize;
  common::Volume imageSize;
};

} // namespace simulate

} // namespace sme
