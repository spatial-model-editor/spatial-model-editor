// Spatial Geometry
//  - Compartment class: defines set of points that make up a compartment &
//  nearest neighbours for each point
//  - Membrane class: defines set of points on either side of the boundary
//  between two compartments, aka the membrane
//  - Field class: species concentration within a compartment
//  - CompartmentIndexer class: utility class to convert a QPoint to the
//  corresponding vector index for a Compartment (for initialising Membranes)

#pragma once

#include <unordered_map>

#include <QDebug>
#include <QImage>

namespace geometry {

// a set of default colours for display purposes
class defaultSpeciesColours {
 private:
  const std::vector<QColor> colours{
      {230, 25, 75},  {60, 180, 75},   {255, 225, 25}, {0, 130, 200},
      {245, 130, 48}, {145, 30, 180},  {70, 240, 240}, {240, 50, 230},
      {210, 245, 60}, {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
      {170, 110, 40}, {255, 250, 200}, {128, 0, 0},    {170, 255, 195},
      {128, 128, 0},  {255, 215, 180}, {0, 0, 128},    {128, 128, 128}};

 public:
  const QColor &operator[](std::size_t i) const {
    return colours[i % colours.size()];
  }
};

class Compartment {
 private:
  QImage imgComp;
  // vector of indices of nearest-neighbours
  std::vector<std::size_t> nn;

 public:
  Compartment() = default;
  // create compartment geometry from all pixels in `img` of colour `col`
  Compartment(const std::string &compID, const QImage &img, QRgb col);
  // compartmentID
  std::string compartmentID;
  // vector of points that make up compartment
  std::vector<QPoint> ix;
  // indices of nearest neighbours
  // e.g. ix[up_x[i]] is the +x neigbour of point ix[i]
  // a field stores the concentration at point ix[i] at index i
  // zero flux Neumann bcs: outside neighbour of point on boundary is itself
  inline std::size_t up_x(std::size_t i) const { return nn[4 * i]; }
  inline std::size_t dn_x(std::size_t i) const { return nn[4 * i + 1]; }
  inline std::size_t up_y(std::size_t i) const { return nn[4 * i + 2]; }
  inline std::size_t dn_y(std::size_t i) const { return nn[4 * i + 3]; }
  // return a QImage of the compartment geometry
  const QImage &getCompartmentImage() const;
};

class Membrane {
 public:
  std::string membraneID;
  const Compartment *compA;
  const Compartment *compB;
  std::vector<std::pair<std::size_t, std::size_t>> indexPair;
  Membrane() = default;
  Membrane(const std::string &ID, const Compartment *A, const Compartment *B,
           const std::vector<std::pair<QPoint, QPoint>> &membranePairs);
};

class Field {
 private:
  QImage imgConc;

 public:
  std::string speciesID;
  const Compartment *geometry;
  double diffusionConstant;
  QColor colour;
  // field of species concentration
  std::vector<double> conc;
  // field of dcdt values
  std::vector<double> dcdt;
  Field() = default;
  Field(const Compartment *geom, const std::string &specID,
        double diffConst = 1.0, const QColor &col = QColor(255, 0, 0));
  void setConstantConcentration(double concentration);
  void importConcentration(const QImage &img, double scale_factor = 1.0);
  // return a QImage of the concentration of given species
  const QImage &getConcentrationImage();
  double getMeanConcentration();
  // field.dcdt = result of the diffusion operator acting on field.conc
  void applyDiffusionOperator();
};

class CompartmentIndexer {
 private:
  const Compartment &comp;
  int imgHeight;
  std::unordered_map<int, std::size_t> index;

 public:
  explicit CompartmentIndexer(const Compartment &c);
  std::size_t getIndex(const QPoint &point);
};

}  // namespace geometry
