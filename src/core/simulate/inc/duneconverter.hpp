// DUNE-copasi ini file generation
//  - DuneConverter: construct DUNE ini file from model

#pragma once

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
                         double dt = 1e-2, int doublePrecision = 18);
  QString getIniFile() const;
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
  QString iniFile;
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
