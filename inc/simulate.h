#ifndef SIMULATE_H
#define SIMULATE_H

#include <QDebug>
#include <QImage>
#include <QPoint>

#include "numerics.h"
#include "sbml.h"

class field {
  enum BOUNDARY_CONDITION { DIRICHLET, NEUMANN };

 private:
  const std::size_t n_bcs = 1;
  // location of index in image
  std::vector<QPoint> ix;
  // vector of indices of the nearest-neighbours
  // of the point i:
  // [4*i+0] = +x neighbour
  // [4*i+1] = -x neighbour
  // [4*i+2] = +y neighbour
  // [4*i+3] = -y neighbour
  // size: 4*n_pixels
  std::vector<std::size_t> nn;
  // size of QImage
  QSize img_size;

 public:
  std::size_t n_species = 1;
  std::size_t n_pixels = 1;
  // field of species concentrations
  std::vector<double> conc;
  // field of dcdt values
  std::vector<double> dcdt;
  // vector of diffusion constants for each species
  std::vector<double> diffusion_constant;

  field(std::size_t n_species_, QImage img, QRgb col,
        BOUNDARY_CONDITION bc = NEUMANN);
  // return a QImage of the compartment geometry
  QImage img_comp;
  const QImage &compartment_image();
  // return a QImage of the concentration of of a species
  std::vector<QImage> img_conc;
  const QImage &concentration_image(std::size_t species_index);
  // field.dcdt = result of the diffusion operator acting on field.conc
  void diffusion_op();
  double get_mean_concentration(std::size_t species_index);
};

class simulate {
 private:
  const sbmlDocWrapper &doc;
  // vector of reaction expressions
  std::vector<numerics::reaction_eval> reac_eval;
  // matrix M_ij of stoichiometric coefficients
  // i is the species index
  // j is the reaction index
  std::vector<std::vector<double>> M;

 public:
  // vector of species that expressions will use
  std::vector<double> species_values;

  simulate(const sbmlDocWrapper &doc) : doc(doc) {}
  // compile reaction expressions
  void compile_reactions();

  // 1d simulation
  // vector of result of applying reaction expressions
  std::vector<double> evaluate_reactions();
  // integration timestep: 1d, forwards euler
  void timestep_1d_euler(double dt);

  // 2d spatial simulation
  // field.dcdt += result of applying reaction expressions to field.conc
  void evaluate_reactions(field &field);
  // integration timestep: forwards euler: conc += dcdt * dt
  void timestep_2d_euler(field &field, double dt);
};

#endif  // SIMULATE_H
