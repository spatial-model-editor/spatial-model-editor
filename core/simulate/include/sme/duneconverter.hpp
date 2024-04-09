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

class DuneConverter {
public:
  explicit DuneConverter(
      const model::Model &model,
      const std::map<std::string, double, std::less<>> &substitutions = {},
      bool forExternalUse = false, const QString &outputIniFile = {},
      int doublePrecision = 18);
  [[nodiscard]] QString getIniFile() const;

  [[nodiscard]] const mesh::Mesh2d *getMesh() const;
  [[nodiscard]] const mesh::Mesh3d *getMesh3d() const;
  [[nodiscard]] const std::unordered_map<std::string, std::vector<double>> &
  getConcentrations() const;
  [[nodiscard]] const std::unordered_map<std::string,
                                         std::vector<std::string>> &
  getSpeciesNames() const;
  [[nodiscard]] const std::vector<std::string> &getCompartmentNames() const;
  [[nodiscard]] common::VoxelF getOrigin() const;
  [[nodiscard]] common::VolumeF getVoxelSize() const;
  [[nodiscard]] common::Volume getImageSize() const;

private:
  QString iniFile;
  const mesh::Mesh2d *mesh;
  const mesh::Mesh3d *mesh3d;
  std::unordered_map<std::string, std::vector<double>> concentrations;
  std::unordered_map<std::string, std::vector<std::string>> speciesNames;
  std::vector<std::string> compartmentNames;
  common::VoxelF origin;
  common::VolumeF voxelSize;
  common::Volume imageSize;
};

} // namespace simulate

} // namespace sme
