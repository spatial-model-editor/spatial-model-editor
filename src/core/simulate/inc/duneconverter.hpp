// DUNE-copasi ini file generation
//  - DuneConverter: construct DUNE ini file from model

#pragma once

#include "simulate_options.hpp"
#include <QString>
#include <unordered_set>
#include <vector>

namespace model {
class Model;
}

namespace mesh {
class Mesh;
}

namespace simulate {

class DuneConverter {
public:
  explicit DuneConverter(const model::Model &model, bool forExternalUse = false,
                         const simulate::DuneOptions &duneOptions = {},
                         const QString &outputIniFile = {},
                         int doublePrecision = 18);
  QString getIniFile(std::size_t compartmentIndex = 0) const;
  const std::vector<QString> &getIniFiles() const;
  const std::unordered_set<int> &getGMSHCompIndices() const;
  bool hasIndependentCompartments() const;
  const mesh::Mesh *getMesh() const;
  const std::vector<std::vector<std::vector<double>>> &
  getConcentrations() const;
  double getXOrigin() const;
  double getYOrigin() const;
  double getPixelWidth() const;
  int getImageWidth() const;

private:
  std::vector<QString> iniFiles;
  std::unordered_set<int> gmshCompIndices;
  bool independentCompartments{true};
  const mesh::Mesh *mesh;
  std::vector<std::vector<std::vector<double>>> concentrations;
  double x0;
  double y0;
  double a;
  int w;
};

} // namespace simulate
