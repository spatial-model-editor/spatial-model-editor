// Spatial Geometry
//  - Compartment class: defines set of points that make up a compartment &
//  nearest neighbours for each point
//  - Membrane class: defines set of points on either side of the boundary
//  between two compartments, aka the membrane
//  - Field class: species concentration within a compartment

#pragma once

#include "sme/image_stack.hpp"
#include "sme/voxel.hpp"
#include <QImage>
#include <QPoint>
#include <QRgb>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace sme::geometry {

class Compartment {
private:
  // indices of nearest neighbours
  std::vector<std::size_t> nn;
  std::string compartmentId;
  // vector of voxels that make up compartment
  std::vector<common::Voxel> ix;
  // index of corresponding point for each voxel in array
  std::vector<std::size_t> arrayPoints;
  QRgb color{0};
  sme::common::ImageStack images;

public:
  Compartment() = default;
  // create compartment geometry from all pixels in `img` of color `col`
  Compartment(std::string compId, const common::ImageStack &imgs, QRgb col);
  [[nodiscard]] const std::string &getId() const;
  [[nodiscard]] QRgb getColor() const;
  void setColor(QRgb newColor);
  [[nodiscard]] inline const std::vector<common::Voxel> &getVoxels() const {
    return ix;
  }
  [[nodiscard]] inline const common::Voxel &getVoxel(std::size_t i) const {
    return ix[i];
  }
  [[nodiscard]] inline std::size_t nVoxels() const { return ix.size(); }
  // e.g. ix[up_x[i]] is the +x neighbour of point ix[i]
  // a field stores the concentration at point ix[i] at index i
  // zero flux Neumann bcs: outside neighbour of point on boundary is itself
  [[nodiscard]] inline std::size_t up_x(std::size_t i) const {
    return nn[6 * i];
  }
  [[nodiscard]] inline std::size_t dn_x(std::size_t i) const {
    return nn[6 * i + 1];
  }
  [[nodiscard]] inline std::size_t up_y(std::size_t i) const {
    return nn[6 * i + 2];
  }
  [[nodiscard]] inline std::size_t dn_y(std::size_t i) const {
    return nn[6 * i + 3];
  }
  [[nodiscard]] inline std::size_t up_z(std::size_t i) const {
    return nn[6 * i + 4];
  }
  [[nodiscard]] inline std::size_t dn_z(std::size_t i) const {
    return nn[6 * i + 5];
  }
  [[nodiscard]] const common::Volume &getImageSize() const;
  // return a QImage of the compartment geometry
  [[nodiscard]] const sme::common::ImageStack &getCompartmentImages() const;
  [[nodiscard]] const std::vector<std::size_t> &getArrayPoints() const;
};

class Membrane {
private:
  std::array<std::vector<std::pair<std::size_t, std::size_t>>, 3> indexPairs{};
  std::string id{};
  const Compartment *compA{};
  const Compartment *compB{};
  sme::common::ImageStack images{};

public:
  enum FLUX_DIRECTION { X = 0, Y = 1, Z = 2 };
  Membrane() = default;
  Membrane(std::string membraneId, const Compartment *A, const Compartment *B,
           const std::vector<std::pair<common::Voxel, common::Voxel>>
               *membranePairs);
  [[nodiscard]] const std::string &getId() const;
  void setId(const std::string &membraneId);
  [[nodiscard]] const Compartment *getCompartmentA() const;
  [[nodiscard]] const Compartment *getCompartmentB() const;
  [[nodiscard]] const std::vector<std::pair<std::size_t, std::size_t>> &
  getIndexPairs(FLUX_DIRECTION fluxDirection) const;
  [[nodiscard]] const sme::common::ImageStack &getImages() const;
};

class Field {
private:
  std::string id{};
  const Compartment *comp{};
  QRgb color{};
  std::vector<double> conc{};
  std::vector<double> diff{};
  bool isUniformConcentration{true};
  bool isSpatial{true};
  bool isUniformDiffusionConstant{true};
  std::vector<double> importSbmlArray(const std::vector<double> &sbmlArray);
  [[nodiscard]] std::vector<double>
  getImageArray(const std::vector<double> &values,
                bool maskAndInvertY = false) const;

public:
  Field() = default;
  Field(const Compartment *compartment, std::string specID,
        double diffConst = 1.0, QRgb col = qRgb(255, 0, 0));
  [[nodiscard]] const std::string &getId() const;
  [[nodiscard]] QRgb getColor() const;
  void setColor(QRgb col);
  [[nodiscard]] bool getIsSpatial() const;
  void setIsSpatial(bool spatial);
  [[nodiscard]] bool getIsUniformConcentration() const;
  void setIsUniformConcentration(bool uniform);
  [[nodiscard]] bool getIsUniformDiffusionConstant() const;
  void setIsUniformDiffusionConstant(bool uniform);
  [[nodiscard]] const std::vector<double> &getDiffusionConstant() const;
  void setDiffusionConstant(std::size_t index, double diffusionConstant);
  void setUniformDiffusionConstant(double diffConst);
  void setDiffusionConstant(const std::vector<double> &diffusionConstantArray);
  void importDiffusionConstant(const std::vector<double> &sbmlArray);
  [[nodiscard]] std::vector<double>
  getDiffusionConstantImageArray(bool maskAndInvertY = false) const;
  [[nodiscard]] const Compartment *getCompartment() const;
  [[nodiscard]] const std::vector<double> &getConcentration() const;
  void setConcentration(std::size_t index, double concentration);
  void setUniformConcentration(double concentration);
  void importConcentration(const std::vector<double> &sbmlConcentrationArray);
  void setConcentration(const std::vector<double> &concentration);
  [[nodiscard]] common::ImageStack getConcentrationImages() const;
  [[nodiscard]] std::vector<double>
  getConcentrationImageArray(bool maskAndInvertY = false) const;
  void setCompartment(const Compartment *comp);
};

} // namespace sme::geometry
