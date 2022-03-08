#include "sme/optimize.hpp"
#include "optimize_impl.hpp"
#include "sme/logger.hpp"
#include "sme/model.hpp"
#include <iostream>
#include <pagmo/algorithms/pso.hpp>
#include <utility>

namespace sme::simulate {

Optimization::Optimization(sme::model::Model &model,
                           OptimizeOptions optimizeOptions)
    : options{std::move(optimizeOptions)} {
  PagmoUDP udp{};
  udp.init(model.getXml().toStdString(), options);
  prob = pagmo::problem{std::move(udp)};
  algo = pagmo::algorithm{pagmo::pso()};
  pop = pagmo::population{prob, options.nParticles};
}

void Optimization::evolve() {
  pop = algo.evolve(pop);
  ++niter;
}

void Optimization::applyParametersToModel(sme::model::Model *model) const {
  applyParameters(params(), options.optParams, model);
}

std::vector<double> Optimization::params() const { return pop.champion_x(); }

std::vector<double> Optimization::fitness() const { return pop.champion_f(); }

std::size_t Optimization::iterations() const { return niter; };

} // namespace sme::simulate
