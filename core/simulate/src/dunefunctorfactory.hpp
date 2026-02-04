#pragma once

#include "dune_headers.hpp"
#include "sme/duneconverter.hpp"
#include "sme/logger.hpp"
#include "sme/voxel.hpp"
#include <QPoint>
#include <algorithm>
#include <cstddef>
#include <dune/common/parametertree.hh>
#include <dune/copasi/model/functor_factory.hh>
#include <dune/copasi/parser/context.hh>
#include <memory>
#include <vector>

namespace Dune {
template <typename F, int n> class FieldVector;
}

namespace Dune::Copasi {
template <std::size_t dim> struct LocalDomain;
}

namespace sme::simulate {

/* A wrapper around the dune-copasi FunctorFactoryParser
 *
 * If the parser_type in the config is "sme" and the expression corresponds
 * to a diffusion constant array defined in the DuneConverter, then a functor
 * is created that looks up the diffusion constant based on the local position.
 *
 * Otherwise, the request is forwarded to the dune-copasi FunctorFactoryParser.
 */
template <std::size_t dim>
class SmeFunctorFactory final : public Dune::Copasi::FunctorFactory<dim> {
public:
  explicit SmeFunctorFactory(
      const DuneConverter &duneConverter,
      Dune::Copasi::ParserType parser_type = Dune::Copasi::default_parser,
      std::shared_ptr<const Dune::Copasi::ParserContext> parser_context =
          nullptr)
      : Dune::Copasi::FunctorFactory<dim>(), dc(duneConverter) {
    functor_factory_parser =
        std::make_shared<Dune::Copasi::FunctorFactoryParser<dim>>(
            parser_type, std::move(parser_context));
  }

  SmeFunctorFactory(const SmeFunctorFactory &) = delete;
  SmeFunctorFactory(SmeFunctorFactory &&) = delete;

  SmeFunctorFactory &operator=(const SmeFunctorFactory &) = delete;
  SmeFunctorFactory &operator=(SmeFunctorFactory &&) = delete;

  ~SmeFunctorFactory() override = default;

  [[nodiscard]] Dune::Copasi::FunctorFactory<dim>::ScalarFunctor
  make_scalar(std::string_view prefix, const Dune::ParameterTree &config,
              const Dune::Copasi::LocalDomain<dim> &local_domain,
              int codim) const override {
    if (config.get("parser_type", std::string{}) == "sme") {
      // special case for SME diffusion constant arrays
      std::string arrayName = config.get("expression", std::string{});
      const auto &diffusionConstantArrays = dc.getDiffusionConstantArrays();
      auto origin = dc.getOrigin();
      auto voxel = dc.getVoxelSize();
      auto vol = dc.getImageSize();
      if (const auto it = diffusionConstantArrays.find(arrayName);
          it != diffusionConstantArrays.end()) {
        const auto array = it->second;
        SPDLOG_DEBUG("Using SME diffusion constant array '{}'", arrayName);
        auto *pos = &local_domain.position;
        return [array, pos, origin, voxel, vol]() noexcept {
          // get nearest voxel to physical point
          auto ix = common::toVoxelIndex((*pos)[0], origin.p.x(), voxel.width(),
                                         vol.width());
          auto iy = common::toVoxelIndex((*pos)[1], origin.p.y(),
                                         voxel.height(), vol.height());
          int iz = 0;
          if constexpr (dim == 3) {
            iz = common::toVoxelIndex((*pos)[2], origin.z, voxel.depth(),
                                      static_cast<int>(vol.depth()));
          }
          auto ci = common::voxelArrayIndex(vol, ix, iy,
                                            static_cast<std::size_t>(iz));
          SPDLOG_TRACE("  -> voxel ({},{},{})", ix, iy, iz);
          SPDLOG_TRACE("  -> diffusionConstant[{}] = {}", ci, array[ci]);
          return Dune::FieldVector<double, 1>{array[ci]};
        };
      }
      auto errorMsg =
          fmt::format("SME diffusion constant array '{}' not found", arrayName);
      SPDLOG_ERROR("{}", errorMsg);
      throw std::runtime_error(errorMsg);
    }
    return functor_factory_parser->make_scalar(prefix, config, local_domain,
                                               codim);
  }

  [[nodiscard]] Dune::Copasi::FunctorFactory<dim>::VectorFunctor
  make_vector(std::string_view prefix, const Dune::ParameterTree &config,
              const Dune::Copasi::LocalDomain<dim> &local_domain,
              int codim) const override {
    if (config.get("parser_type", std::string{}) == "sme") {
      throw std::runtime_error("sme make_vector not implemented");
    }
    return functor_factory_parser->make_vector(prefix, config, local_domain,
                                               codim);
  }

  [[nodiscard]] Dune::Copasi::FunctorFactory<dim>::TensorApplyFunctor
  make_tensor_apply(std::string_view prefix, const Dune::ParameterTree &config,
                    const Dune::Copasi::LocalDomain<dim> &local_domain,
                    int codim) const override {
    if (config.get("parser_type", std::string{}) == "sme") {
      if (config.get("type", "scalar") == "scalar") {
        if (auto f = make_scalar(prefix, config, local_domain, codim)) {
          return
              [f = std::move(f)](Dune::FieldVector<double, dim> vec) noexcept {
                return f()[0] * vec;
              };
        }
        return nullptr;
      }
      throw std::runtime_error(
          "sme make_tensor_apply not implemented for tensors");
    }
    return functor_factory_parser->make_tensor_apply(prefix, config,
                                                     local_domain, codim);
  }

private:
  std::shared_ptr<Dune::Copasi::FunctorFactoryParser<dim>>
      functor_factory_parser;
  DuneConverter dc;
};

} // namespace sme::simulate
