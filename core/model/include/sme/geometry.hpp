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
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sme::geometry {

/**
 * @brief Geometry voxels and neighbor topology for one compartment.
 */
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
  /**
   * @brief Construct empty compartment.
   */
  Compartment() = default;
  /**
   * @brief Construct compartment geometry from image voxels of ``col``.
   */
  Compartment(std::string compId, const common::ImageStack &imgs, QRgb col);
  /**
   * @brief Compartment id.
   */
  [[nodiscard]] const std::string &getId() const;
  /**
   * @brief Compartment display color.
   */
  [[nodiscard]] QRgb getColor() const;
  /**
   * @brief Set compartment display color.
   */
  void setColor(QRgb newColor);
  /**
   * @brief Voxels belonging to compartment.
   */
  [[nodiscard]] inline const std::vector<common::Voxel> &getVoxels() const {
    return ix;
  }
  /**
   * @brief Voxel at linear compartment index ``i``.
   */
  [[nodiscard]] inline const common::Voxel &getVoxel(std::size_t i) const {
    return ix[i];
  }
  /**
   * @brief Number of voxels in compartment.
   */
  [[nodiscard]] inline std::size_t nVoxels() const { return ix.size(); }
  /**
   * @brief +x neighbor index for voxel ``i`` (Neumann boundary handling).
   */
  [[nodiscard]] inline std::size_t up_x(std::size_t i) const {
    return nn[6 * i];
  }
  /**
   * @brief -x neighbor index for voxel ``i``.
   */
  [[nodiscard]] inline std::size_t dn_x(std::size_t i) const {
    return nn[6 * i + 1];
  }
  /**
   * @brief +y neighbor index for voxel ``i``.
   */
  [[nodiscard]] inline std::size_t up_y(std::size_t i) const {
    return nn[6 * i + 2];
  }
  /**
   * @brief -y neighbor index for voxel ``i``.
   */
  [[nodiscard]] inline std::size_t dn_y(std::size_t i) const {
    return nn[6 * i + 3];
  }
  /**
   * @brief +z neighbor index for voxel ``i``.
   */
  [[nodiscard]] inline std::size_t up_z(std::size_t i) const {
    return nn[6 * i + 4];
  }
  /**
   * @brief -z neighbor index for voxel ``i``.
   */
  [[nodiscard]] inline std::size_t dn_z(std::size_t i) const {
    return nn[6 * i + 5];
  }
  /**
   * @brief Underlying geometry image dimensions.
   */
  [[nodiscard]] const common::Volume &getImageSize() const;
  /**
   * @brief Binary image stack for this compartment.
   */
  [[nodiscard]] const sme::common::ImageStack &getCompartmentImages() const;
  /**
   * @brief Mapping from full-image voxels to compartment voxel indices.
   */
  [[nodiscard]] const std::vector<std::size_t> &getArrayPoints() const;
  /**
   * @brief Compartment voxel index for an image voxel, if inside compartment.
   */
  [[nodiscard]] std::optional<std::size_t>
  getIdx(const common::Voxel &voxel) const;
};

/**
 * @brief Membrane interface between two compartments.
 */
class Membrane {
private:
  std::array<std::vector<std::pair<std::size_t, std::size_t>>, 6>
      faceIndexPairs{};
  std::string id{};
  const Compartment *compA{};
  const Compartment *compB{};
  sme::common::ImageStack images{};

public:
  /**
   * @brief Signed membrane face direction from compartment A to B.
   */
  enum FACE_DIRECTION { XP = 0, XM = 1, YP = 2, YM = 3, ZP = 4, ZM = 5 };
  /**
   * @brief Construct empty membrane.
   */
  Membrane() = default;
  /**
   * @brief Construct membrane from adjacent voxel pairs.
   */
  Membrane(std::string membraneId, const Compartment *A, const Compartment *B,
           const std::vector<std::pair<common::Voxel, common::Voxel>>
               *membranePairs);
  /**
   * @brief Membrane id.
   */
  [[nodiscard]] const std::string &getId() const;
  /**
   * @brief Set membrane id.
   */
  void setId(const std::string &membraneId);
  /**
   * @brief First adjacent compartment.
   */
  [[nodiscard]] const Compartment *getCompartmentA() const;
  /**
   * @brief Second adjacent compartment.
   */
  [[nodiscard]] const Compartment *getCompartmentB() const;
  /**
   * @brief Number of adjacent voxel pairs in the membrane.
   */
  [[nodiscard]] std::size_t size() const;
  /**
   * @brief Index pairs for one signed membrane face direction.
   */
  [[nodiscard]] const std::vector<std::pair<std::size_t, std::size_t>> &
  getFaceIndexPairs(FACE_DIRECTION faceDirection) const;
  /**
   * @brief Membrane visualization image stack.
   */
  [[nodiscard]] const sme::common::ImageStack &getImages() const;
};

/**
 * @brief Species field values over a compartment.
 */
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
  /**
   * @brief Construct empty field.
   */
  Field() = default;
  /**
   * @brief Construct field for a compartment/species.
   */
  Field(const Compartment *compartment, std::string specID,
        double diffConst = 1.0, QRgb col = qRgb(255, 0, 0));
  /**
   * @brief Species id.
   */
  [[nodiscard]] const std::string &getId() const;
  /**
   * @brief Field display color.
   */
  [[nodiscard]] QRgb getColor() const;
  /**
   * @brief Set field display color.
   */
  void setColor(QRgb col);
  /**
   * @brief Returns ``true`` if field is spatially resolved.
   */
  [[nodiscard]] bool getIsSpatial() const;
  /**
   * @brief Set whether field is spatially resolved.
   */
  void setIsSpatial(bool spatial);
  /**
   * @brief Returns ``true`` if concentration is uniform.
   */
  [[nodiscard]] bool getIsUniformConcentration() const;
  /**
   * @brief Set whether concentration is uniform.
   */
  void setIsUniformConcentration(bool uniform);
  /**
   * @brief Returns ``true`` if diffusion constant is uniform.
   */
  [[nodiscard]] bool getIsUniformDiffusionConstant() const;
  /**
   * @brief Set whether diffusion constant is uniform.
   */
  void setIsUniformDiffusionConstant(bool uniform);
  /**
   * @brief Diffusion constant values.
   */
  [[nodiscard]] const std::vector<double> &getDiffusionConstant() const;
  /**
   * @brief Set diffusion constant at one voxel.
   */
  void setDiffusionConstant(std::size_t index, double diffusionConstant);
  /**
   * @brief Set uniform diffusion constant.
   */
  void setUniformDiffusionConstant(double diffConst);
  /**
   * @brief Set full diffusion-constant array.
   */
  void setDiffusionConstant(const std::vector<double> &diffusionConstantArray);
  /**
   * @brief Import diffusion constants from SBML ordering.
   */
  void importDiffusionConstant(const std::vector<double> &sbmlArray);
  /**
   * @brief Diffusion-constant array in image ordering.
   */
  [[nodiscard]] std::vector<double>
  getDiffusionConstantImageArray(bool maskAndInvertY = false) const;
  /**
   * @brief Owning compartment.
   */
  [[nodiscard]] const Compartment *getCompartment() const;
  /**
   * @brief Concentration values.
   */
  [[nodiscard]] const std::vector<double> &getConcentration() const;
  /**
   * @brief Set concentration at one voxel.
   */
  void setConcentration(std::size_t index, double concentration);
  /**
   * @brief Set uniform concentration.
   */
  void setUniformConcentration(double concentration);
  /**
   * @brief Import concentrations from SBML ordering.
   */
  void importConcentration(const std::vector<double> &sbmlConcentrationArray);
  /**
   * @brief Set full concentration array.
   */
  void setConcentration(const std::vector<double> &concentration);
  /**
   * @brief Concentration visualization images.
   */
  [[nodiscard]] common::ImageStack getConcentrationImages() const;
  /**
   * @brief Concentration array in image ordering.
   */
  [[nodiscard]] std::vector<double>
  getConcentrationImageArray(bool maskAndInvertY = false) const;
  /**
   * @brief Reassign field to a different compartment.
   */
  void setCompartment(const Compartment *comp);
};

} // namespace sme::geometry
