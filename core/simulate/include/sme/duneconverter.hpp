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
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace mesh {
class Mesh;
}

namespace simulate {

class DuneConverter {
public:
  explicit DuneConverter(
      const model::Model &model,
      const std::map<std::string, double, std::less<>> &substitutions = {},
      bool forExternalUse = false, const QString &outputIniFile = {},
      int doublePrecision = 18);
  [[nodiscard]] QString getIniFile(std::size_t compartmentIndex = 0) const;
  [[nodiscard]] const std::vector<QString> &getIniFiles() const;
  [[nodiscard]] bool hasIndependentCompartments() const;

  [[nodiscard]] const mesh::Mesh *getMesh() const;
  [[nodiscard]] const std::vector<std::vector<std::vector<double>>> &
  getConcentrations() const;
  [[nodiscard]] common::VoxelF getOrigin() const;
  [[nodiscard]] common::VolumeF getVoxelSize() const;
  [[nodiscard]] common::Volume getImageSize() const;

private:
  std::vector<QString> iniFiles;
  bool independentCompartments{true};
  const mesh::Mesh *mesh;
  std::vector<std::vector<std::vector<double>>> concentrations;
  common::VoxelF origin;
  common::VolumeF voxelSize;
  common::Volume imageSize;
};

} // namespace simulate

} // namespace sme
