#include <unordered_map>

#include "simulate.h"

void simulate::compile_reactions(const QString &compartmentID) {
  // init vector of species with zeros
  const auto &species_list = doc.species.at(compartmentID);
  species_values = std::vector<double>(species_list.size(), 0.0);
  // init matrix M with zeros, matrix size: n_species x n_reactions
  M.clear();
  for (unsigned int i = 0; i < species_values.size(); ++i) {
    M.emplace_back(std::vector<double>(doc.model->getNumReactions(), 0));
  }
  reac_eval.reserve(doc.model->getNumReactions());
  qDebug() << M;
  // process each reaction
  int reaction_index = 0;
  for (unsigned int ri = 0; ri < doc.model->getNumReactions(); ++ri) {
    const auto *reac = doc.model->getReaction(ri);
    qDebug() << reac->getCompartment().c_str();
    if (reac->getCompartment().c_str() == compartmentID ||
        reac->getCompartment().size() == 0) {
      // get mathematical formula
      const auto *kin = reac->getKineticLaw();
      std::string expr = kin->getFormula();

      // TODO: deal with amount vs concentration issues correctly
      // if getHasOnlySubstanceUnits is true for some (all?) species
      // note: would also need to also do this in the inlining step, and in the
      // stoich matrix factors

      // inline function calls
      qDebug() << expr.c_str();
      expr = doc.inlineFunctions(expr);
      qDebug() << "after inlining:";
      qDebug() << expr.c_str();

      // get all parameters and constants used in reaction
      std::vector<std::string> constant_names;
      std::vector<double> constant_values;
      // get local parameters and their values
      for (unsigned k = 0; k < kin->getNumLocalParameters(); ++k) {
        constant_names.push_back(kin->getLocalParameter(k)->getId());
        constant_values.push_back(kin->getLocalParameter(k)->getValue());
      }
      for (unsigned k = 0; k < kin->getNumParameters(); ++k) {
        constant_names.push_back(kin->getParameter(k)->getId());
        constant_values.push_back(kin->getParameter(k)->getValue());
      }
      // get global parameters and their values:
      for (unsigned k = 0; k < doc.model->getNumParameters(); ++k) {
        constant_names.push_back(doc.model->getParameter(k)->getId());
        constant_values.push_back(doc.model->getParameter(k)->getValue());
      }
      // get compartment volumes (the compartmentID may be used in the reaction
      // equation, and it should be replaced with the value of the "Size"
      // parameter for this compartment)
      for (unsigned int k = 0; k < doc.model->getNumCompartments(); ++k) {
        const auto *comp = doc.model->getCompartment(k);
        constant_names.push_back(comp->getId());
        constant_values.push_back(comp->getSize());
      }

      for (std::size_t k = 0; k < constant_names.size(); ++k) {
        qDebug("const: %s %f", constant_names[k].c_str(), constant_values[k]);
      }

      // compile expression and add to reac_eval vector
      reac_eval.emplace_back(
          numerics::ReactionEvaluate(expr, doc.speciesID, species_values,
                                     constant_names, constant_values));

      // add difference of stoichiometric coefficients to matrix M for each
      // species produced and consumed by this reaction (unless the species
      // concentration is fixed, i.e. constant or boundaryCondition)
      for (unsigned k = 0; k < reac->getNumProducts(); ++k) {
        // get product species reference
        const auto *spec_ref = reac->getProduct(k);
        // get index in species_list
        int species_index = 0;
        while (species_list[species_index] !=
               QString(spec_ref->getSpecies().c_str())) {
          ++species_index;
        }
        qDebug() << species_index;
        // if species is not constant, or a boundaryCondition, then *add*
        // stoichiometric coefficient to M(species_index,reaction_index)
        const auto *spec = doc.model->getSpecies(spec_ref->getSpecies());
        if (!((spec->isSetConstant() && spec->getConstant()) ||
              (spec->isSetBoundaryCondition() &&
               spec->getBoundaryCondition()))) {
          M[species_index][reaction_index] += spec_ref->getStoichiometry();
          qDebug("M[%d][%d] += %f", species_index, reaction_index,
                 spec_ref->getStoichiometry());
        }
      }
      for (unsigned k = 0; k < reac->getNumReactants(); ++k) {
        // get product species reference
        const auto *spec_ref = reac->getReactant(k);
        // get index in species_list
        int species_index = 0;
        while (species_list[species_index] != spec_ref->getSpecies().c_str()) {
          ++species_index;
        }
        // if species is not constant, or a boundaryCondition, then *add*
        // stoichiometric coefficient to M(species_index,reaction_index)
        const auto *spec = doc.model->getSpecies(spec_ref->getSpecies());
        if (!((spec->isSetConstant() && spec->getConstant()) ||
              (spec->isSetBoundaryCondition() &&
               spec->getBoundaryCondition()))) {
          M[species_index][reaction_index] -= spec_ref->getStoichiometry();
          qDebug("M[%d][%d] -= %f", species_index, reaction_index,
                 spec_ref->getStoichiometry());
        }
      }
      ++reaction_index;
    }
  }
  qDebug() << M;
}

std::vector<double> simulate::evaluate_reactions() {
  std::vector<double> result(species_values.size(), 0.0);
  for (std::size_t j = 0; j < reac_eval.size(); ++j) {
    double r_j = reac_eval[j]();
    for (std::size_t i = 0; i < M.size(); ++i) {
      result[i] += M[i][j] * r_j;
    }
  }
  return result;
}

void simulate::timestep_1d_euler(double dt) {
  std::vector<double> dcdt = evaluate_reactions();
  for (std::size_t i = 0; i < species_values.size(); ++i) {
    //    if (!doc.model->getSpecies(static_cast<unsigned
    //    int>(i))->getConstant()) {
    species_values[i] += dcdt[i] * dt;
    //    }
  }
  qDebug() << species_values;
}

void simulate::evaluate_reactions(field &field) {
  for (std::size_t ix = 0; ix < field.n_pixels; ++ix) {
    // populate species concentrations
    for (std::size_t s = 0; s < field.n_species; ++s) {
      species_values[s] = field.conc[ix * field.n_species + s];
    }
    for (std::size_t j = 0; j < reac_eval.size(); ++j) {
      // evaluate reaction terms
      double r_j = reac_eval[j]();
      for (std::size_t s = 0; s < M.size(); ++s) {
        // add results to dcdt
        field.dcdt[ix * field.n_species + s] += M[s][j] * r_j;
      }
    }
  }
}

void simulate::timestep_2d_euler(field &field, double dt) {
  field.diffusion_op();
  evaluate_reactions(field);
  for (std::size_t i = 0; i < field.conc.size(); ++i) {
    field.conc[i] += dt * field.dcdt[i];
  }
}

field::field(std::size_t n_species_, QImage img, QRgb col,
             BOUNDARY_CONDITION bc) {
  n_species = n_species_;
  qDebug("field: n_species: %d", n_species);
  img_size = img.size();
  img_comp = QImage(img_size, QImage::Format_Mono);
  img_conc =
      std::vector<QImage>(n_species, QImage(img_size, QImage::Format_ARGB32));

  // set diffusion constants to 1 for now:
  diffusion_constant = std::vector<double>(n_species, 0.0);
  std::unordered_map<int, std::size_t> index;
  ix.clear();
  // find pixels in compartment: store image QPoint for each
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      if (img.pixel(x, y) == col) {
        // if colour matches, add pixel to field
        qDebug("%d, %d", x, y);
        ix.push_back(QPoint(x, y));
        // also add to temporary map for neighbour lookup
        index[x * img.height() + y] = ix.size() - 1;
      }
    }
  }
  nn.clear();
  nn.reserve(4 * ix.size());
  // find neighbours of each pixel in compartment
  std::size_t outside = ix.size();
  for (const auto &p : ix) {
    for (const auto &pp :
         {QPoint(p.x() + 1, p.y()), QPoint(p.x() - 1, p.y()),
          QPoint(p.x(), p.y() + 1), QPoint(p.x(), p.y() - 1)}) {
      if (img.valid(pp) && (img.pixel(pp) == col)) {
        qDebug() << pp;
        nn.push_back(index.at(pp.x() * img.height() + pp.y()));
      } else {
        if (bc == DIRICHLET) {
          // Dirichlet bcs: specify value of conc. at boundary.
          // Here all points on boundary point to the same pixel with
          // index, "outside", which will typically have zero concentration
          nn.push_back(outside);
        } else if (bc == NEUMANN) {
          // Neumann bcs: specify derivative of conc. in direction normal to
          // boundary. Here we define a zero flux condition by setting the value
          // of the boundary conc. to be equal to that of the neighbour.
          // This is done here very naively by making the neighbour of the pixel
          // point to the pixel itself.
          nn.push_back(index.at(p.x() * img.height() + p.y()));
        } else {
          qDebug() << "Error: boundary condition not supported";
          exit(1);
        }
      }
    }
  }
  // add n_species per pixel in comp, plus one for each boundary value
  conc.resize(n_species * (ix.size() + n_bcs), 0.0);
  dcdt = conc;
}

void field::importConcentration(std::size_t species_index, QImage img,
                                double scale_factor) {
  for (std::size_t i = 0; i < ix.size(); ++i) {
    conc[n_species * i + species_index] = img.pixel(ix[i]) * scale_factor;
  }
}

void field::setConstantConcentration(std::size_t species_index,
                                     double concentration) {
  for (std::size_t i = 0; i < ix.size(); ++i) {
    conc[n_species * i + species_index] = concentration;
  }
}

const QImage &field::compartment_image() {
  img_comp.fill(0);
  for (const auto &p : ix) {
    img_comp.setPixel(p, 1);
  }
  return img_comp;
}

const QImage &field::concentration_image(std::size_t species_index) {
  img_conc[species_index].fill(qRgba(0, 0, 0, 0));
  double max_conc = 1e-5;
  for (std::size_t i = 0; i < ix.size(); ++i) {
    max_conc = std::max(max_conc, conc[i * n_species + species_index]);
  }
  for (std::size_t i = 0; i < ix.size(); ++i) {
    int r =
        static_cast<int>(255 * conc[i * n_species + species_index] / max_conc);
    img_conc[species_index].setPixel(ix[i], QColor(r, 0, 0, 255).rgba());
  }
  return img_conc[species_index];
}

void field::diffusion_op() {
  for (std::size_t i = 0; i < ix.size(); ++i) {
    std::size_t index = n_species * i;
    std::size_t xup = n_species * nn[4 * i];
    std::size_t xdn = n_species * nn[4 * i + 1];
    std::size_t yup = n_species * nn[4 * i + 2];
    std::size_t ydn = n_species * nn[4 * i + 3];
    for (std::size_t s = 0; s < n_species; ++s) {
      dcdt[index + s] = diffusion_constant[s] *
                        (conc[xup + s] + conc[xdn + s] + conc[yup + s] +
                         conc[ydn + s] - 4 * conc[index + s]);
    }
  }
}

double field::get_mean_concentration(std::size_t species_index) {
  double sum = 0;
  for (std::size_t i = 0; i < ix.size(); ++i) {
    sum += conc[n_species * i + species_index];
  }
  return sum / static_cast<double>(ix.size());
}
