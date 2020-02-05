// Spatial Geometry
//  - Compartment class: defines set of points that make up a compartment &
//  nearest neighbours for each point
//  - Membrane class: defines set of points on either side of the boundary
//  between two compartments, aka the membrane
//  - Field class: species concentration within a compartment

#pragma once

#include <QImage>
#include <vector>

namespace geometry {

class Compartment {
 private:
  QImage imgComp;
  std::string compartmentID;
  double pixelWidth = 1.0;
  // vector of points that make up compartment
  std::vector<QPoint> ix;
  // indices of nearest neighbours
  std::vector<std::size_t> nn;

 public:
  Compartment() = default;
  // create compartment geometry from all pixels in `img` of colour `col`
  Compartment(const std::string &compID, const QImage &img, QRgb col);
  const std::string &getId() const;
  double getPixelWidth() const;
  void setPixelWidth(double width);
  inline const std::vector<QPoint> &getPixels() const { return ix; }
  inline const QPoint &getPixel(std::size_t i) const { return ix[i]; }
  inline std::size_t nPixels() const { return ix.size(); }
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
 public:
  std::string speciesID;
  const Compartment *geometry;
  double diffusionConstant;
  QColor colour;
  // field of species concentration
  std::vector<double> conc;
  Field() = default;
  Field(const Compartment *geom, const std::string &specID,
        double diffConst = 1.0, const QColor &col = QColor(255, 0, 0));
  void setUniformConcentration(double concentration);
  void importConcentration(const std::vector<double> &sbmlConcentrationArray);
  QImage getConcentrationImage() const;
  std::vector<double> getConcentrationArray() const;
  bool isUniformConcentration = true;
  bool isSpatial = true;
  void setCompartment(const Compartment *compartment);
};

}  // namespace geometry
