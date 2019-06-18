// Spatial Geometry
//  - Compartment class: defines set of points that make up a compartment
//  - Field class: species concentrations w/bcs within a compartment
//  - Membrane class: defines set of points on either side of the boundary
//  between two compartments, aka the membrane
//  - CompartmentIndexer class: utility class to convert a QPoint to the
//  corresponding vector index for a Compartment (only used for initialising
//  Fields and Membranes)

#pragma once

#include <unordered_map>

#include <QDebug>
#include <QImage>

class Compartment {
 private:
  QImage img_comp;

 public:
  Compartment() = default;
  // create compartment geometry from all pixels in `img` of colour `col`
  Compartment(const std::string &compID, const QImage &img, QRgb col);
  // compartmentID
  std::string compartmentID;
  // vector of points that make up compartment
  std::vector<QPoint> ix;
  // size of QImage
  QSize img_size;
  // return a QImage of the compartment geometry
  const QImage &getCompartmentImage();
};

class CompartmentIndexer {
 private:
  const Compartment &comp;
  std::unordered_map<int, std::size_t> index;

 public:
  CompartmentIndexer(const Compartment &comp);
  std::size_t getIndex(const QPoint &point);
  bool isValid(const QPoint &point);
};

class Field {
 private:
  std::size_t n_bcs = 1;
  // vector of indices of the nearest-neighbours
  // of the point i:
  // [4*i+0] = +x neighbour
  // [4*i+1] = -x neighbour
  // [4*i+2] = +y neighbour
  // [4*i+3] = -y neighbour
  // size: 4*n_pixels
  std::vector<std::size_t> nn;
  QImage img_conc;
  // map from speciesID to species index
  std::map<std::string, std::size_t> mapSpeciesIdToIndex;

 public:
  enum BOUNDARY_CONDITION { DIRICHLET, NEUMANN };
  Compartment *geometry;
  std::size_t n_species;
  std::size_t n_pixels;
  // vector of speciesID strings
  std::vector<std::string> speciesID;
  // field of species concentrations
  std::vector<double> conc;
  // field of dcdt values
  std::vector<double> dcdt;
  // vector of diffusion constants for each species
  std::vector<double> diffusion_constant;
  // a set of default colours for display purposes
  std::vector<QColor> speciesColour{
      {230, 25, 75},  {60, 180, 75},   {255, 225, 25}, {0, 130, 200},
      {245, 130, 48}, {145, 30, 180},  {70, 240, 240}, {240, 50, 230},
      {210, 245, 60}, {250, 190, 190}, {0, 128, 128},  {230, 190, 255},
      {170, 110, 40}, {255, 250, 200}, {128, 0, 0},    {170, 255, 195},
      {128, 128, 0},  {255, 215, 180}, {0, 0, 128},    {128, 128, 128}};

  void init(Compartment *geom, const std::vector<std::string> &speciesIDvec,
            BOUNDARY_CONDITION bc = NEUMANN);
  void init(Compartment *geom, const QStringList &speciesIDvec,
            BOUNDARY_CONDITION bc = NEUMANN);
  void setConstantConcentration(std::size_t species_index,
                                double concentration);
  void importConcentration(std::size_t species_index, QImage img,
                           double scale_factor = 1.0);
  void importConcentration(const std::string &species_ID, QImage img,
                           double scale_factor = 1.0);
  // return a QImage of the concentration of given species
  const QImage &getConcentrationImage(std::size_t species_index);
  // return a QImage of the concentration of given species
  const QImage &getConcentrationImage(std::string species_ID);
  // return a QImage of the concentration of the given set of species
  const QImage &getConcentrationImage(
      const std::vector<std::size_t> &species_indices);
  // return a QImage of the concentration of all species
  const QImage &getConcentrationImage();
  // field.dcdt = result of the diffusion operator acting on field.conc
  void applyDiffusionOperator();
  double getMeanConcentration(std::size_t species_index);
};

class Membrane {
 private:
 public:
  std::string membraneID;
  Field *fieldA;
  Field *fieldB;
  std::vector<std::pair<std::size_t, std::size_t>> indexPair;
  Membrane() = default;
  Membrane(const std::string &ID, Field *A, Field *B,
           const std::vector<std::pair<QPoint, QPoint>> &membranePairs);
};
