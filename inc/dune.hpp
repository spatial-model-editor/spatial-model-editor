// DUNE solver interface
//  - DuneSimulation: wrapper to dune-copasi library
//  - DuneConverter: construct DUNE ini file
//  - iniFile class: simple ini file generation one line at a time

#pragma once

#include <QColor>
#include <QImage>
#include <QPointF>
#include <QSizeF>
#include <QString>
#include <map>
#include <memory>
#include <unordered_set>

namespace sbml {
class SbmlDocWrapper;
}

namespace dune {

using QTriangleF = std::array<QPointF, 3>;
using Weight = std::pair<QPoint, std::array<double, 3>>;
using PixelLocalPair = std::pair<QPoint, std::array<double, 2>>;

class IniFile {
 private:
  QString text;

 public:
  const QString &getText() const;
  void addSection(const QString &str);
  void addSection(const QString &str1, const QString &str2);
  void addSection(const QString &str1, const QString &str2,
                  const QString &str3);
  void addValue(const QString &var, const QString &value);
  void addValue(const QString &var, int value);
  void addValue(const QString &var, double value, int precision);
  void clear();
};

class DuneConverter {
 public:
  explicit DuneConverter(const sbml::SbmlDocWrapper &SbmlDoc, double dt = 1e-2,
                         int doublePrecision = 15);
  QString getIniFile() const;
  QColor getSpeciesColour(const std::string &duneName) const;
  std::map<std::string, std::string> mapSpeciesIdsToDuneNames;
  const std::unordered_set<int> &getGMSHCompIndices() const;

 private:
  const sbml::SbmlDocWrapper &doc;
  IniFile ini;
  std::unordered_set<int> gmshCompIndices;
  std::map<std::string, QColor> mapDuneNameToColour;
};

class DuneSimulation {
 private:
  // Dune objects via pimpl to hide DUNE headers
  class DuneImpl;
  std::shared_ptr<DuneImpl> pDuneImpl;
  // index of compartment/species name in these vectors is the Dune index:
  std::vector<std::string> compartmentNames;
  std::vector<std::vector<std::string>> speciesNames;
  // map SBML speciesIds to corresponding DUNE name
  std::map<std::string, std::string> mapSpeciesIdsToDuneNames;
  std::vector<std::vector<QColor>> speciesColours;
  std::map<std::string, double> mapSpeciesIDToAvConc;
  std::vector<std::vector<double>> maxConcs;
  // dimensions of model
  QSizeF geometrySize;
  QPointF scaleFactor;
  // dimensions of the image
  QSize imageSize = QSize(500, 500);
  // pixels+dune local coords for each triangle
  // compartment::triangle::pixels+local-coord
  std::vector<std::vector<std::vector<PixelLocalPair>>> pixels;
  // triangles that make up each compartment
  // compartment::triangle
  std::vector<std::vector<QTriangleF>> triangles;
  // for each triangle: vector of {pixel, W1, W2, W3}
  // compartment::triangle::pixels-with-weights
  std::vector<std::vector<std::vector<Weight>>> weights;
  // for each triangle corner: concentration
  // compartment::species::triangle::corner-conentration-values
  std::vector<std::vector<std::vector<std::vector<double>>>> concentrations;
  void initDuneModel(const sbml::SbmlDocWrapper &sbmlDoc, double dt);
  void initCompartmentNames();
  void initSpeciesNames(const DuneConverter &dc);
  void updatePixels();
  void updateTriangles();
  void updateBarycentricWeights();
  void updateSpeciesConcentrations();
  QRgb pixelColour(std::size_t iDomain, const std::vector<double> &concs) const;

 public:
  explicit DuneSimulation(const sbml::SbmlDocWrapper &sbmlDoc, double dt,
                          QSize imgSize = QSize(500, 500));
  void doTimestep(double dt);
  QImage getConcImage(bool linearInterpolationOnly = false) const;
  double getAverageConcentration(const std::string &speciesID) const;
  void setImageSize(const QSize &imgSize);
};

}  // namespace dune
