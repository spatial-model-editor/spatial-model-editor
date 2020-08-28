//  - makeDuneFunctions(): create dune-copasi grid functions
//  - they evaluate the initial concentrations for all species in model
//  - based on pdelab_expression_adapter.hh from dune-copasi
//  - Note: ensure any changes to pdelab_expression_adapter.hh in future
//  versions of dune-copasi are taken into account here if relevant

#pragma once

#include "duneconverter.hpp"
#include "logger.hpp"
#include <QPoint>
#include <algorithm>
#include <cstddef>
#include <dune/pdelab/common/function.hh>
#include <memory>
#include <vector>

namespace Dune {
template <typename F, int n> class FieldVector;
}

namespace simulate {

static double
getNearestValidPixelConcentration(int ix, int iy, int w, int h,
                                  const std::vector<double> &concentration) {
  std::vector<QPoint> queue;
  queue.reserve(8);
  queue.emplace_back(ix, iy);
  std::size_t queueIndex{0};
  std::size_t maxAttempts{10 * concentration.size()};
  for (std::size_t i = 0; i < maxAttempts; ++i) {
    const auto &p = queue[queueIndex];
    for (const auto &dp :
         {QPoint(1, 0), QPoint(-1, 0), QPoint(0, 1), QPoint(0, -1)}) {
      auto np = p + dp;
      SPDLOG_TRACE("  - ({},{})", np.x(), np.y());
      np.rx() = std::clamp(np.x(), 0, w - 1);
      np.ry() = std::clamp(np.y(), 0, h - 1);
      std::size_t index{static_cast<std::size_t>(np.x() + w * np.y())};
      if (double conc = concentration[index]; conc >= 0.0) {
        return conc;
      } else {
        queue.push_back(np);
      }
    }
    ++queueIndex;
  }
  SPDLOG_WARN("Failed to find valid nearby pixel to ({},{})", ix, iy);
  return 0.0;
}

template <typename GV>
class GridFunction
    : public Dune::PDELab::GridFunctionBase<
          Dune::PDELab::GridFunctionTraits<GV, double, 1,
                                           Dune::FieldVector<double, 1>>,
          GridFunction<GV>> {
public:
  using Traits = Dune::PDELab::GridFunctionTraits<GV, double, 1,
                                                  Dune::FieldVector<double, 1>>;
  GridFunction(double xOrigin, double yOrigin, double pixelWidth, int imgWidth,
               const std::vector<double> &concentration)
      : x0(xOrigin), y0(yOrigin), a(pixelWidth), w(imgWidth),
        h(static_cast<int>(concentration.size()) / imgWidth), c(concentration) {
    SPDLOG_TRACE("  - {}x{} pixels", w, h);
    SPDLOG_TRACE("  - {} pixel width", a);
    SPDLOG_TRACE("  - ({},{}) origin", x0, y0);
  }
  void set_time([[maybe_unused]] double t) { return; }
  template <typename Element, typename Domain>
  void evaluate(const Element &elem, const Domain &localPos,
                typename Traits::RangeType &result) const {
    SPDLOG_TRACE("localPos ({},{})", localPos[0], localPos[1]);
    auto globalPos = elem.geometry().global(localPos);
    SPDLOG_TRACE("globalPos ({},{})", globalPos[0], globalPos[1]);
    // get nearest pixel to physical point
    auto ix = std::clamp(static_cast<int>((globalPos[0] - x0) / a), 0, w - 1);
    auto iy = std::clamp(static_cast<int>((globalPos[1] - y0) / a), 0, h - 1);
    result = c[static_cast<std::size_t>(ix + w * iy)];
    SPDLOG_TRACE("pixel ({},{})", ix, iy);
    SPDLOG_TRACE("conc {}", result);
    if (result < 0) {
      result = getNearestValidPixelConcentration(ix, iy, w, h, c);
      SPDLOG_TRACE("  -> nearest valid conc: {}", result);
    }
  }

private:
  double x0;
  double y0;
  double a;
  int w;
  int h;
  std::vector<double> c;
};

template <class GV>
auto makeCompartmentDuneFunctions(const DuneConverter &dc,
                                  std::size_t compIndex) {
  std::vector<std::shared_ptr<GridFunction<GV>>> functions;
  const auto &concentrations = dc.getConcentrations()[compIndex];
  std::size_t nSpecies{concentrations.size()};
  functions.reserve(nSpecies);
  SPDLOG_TRACE("compartment {}", compIndex);
  SPDLOG_TRACE("  - contains {} species", nSpecies);
  auto x0 = dc.getXOrigin();
  auto y0 = dc.getYOrigin();
  auto a = dc.getPixelWidth();
  auto w = dc.getImageWidth();
  for (const auto &concentration : concentrations) {
    functions.emplace_back(
        std::make_shared<GridFunction<GV>>(x0, y0, a, w, concentration));
  }
  return functions;
}

template <class GV> auto makeModelDuneFunctions(const DuneConverter &dc) {
  std::vector<std::vector<std::shared_ptr<GridFunction<GV>>>> functions;
  const auto &concentrations = dc.getConcentrations();
  functions.reserve(concentrations.size());
  for (std::size_t i = 0; i < concentrations.size(); ++i) {
    functions.push_back(makeCompartmentDuneFunctions<GV>(dc, i));
  }
  return functions;
}

} // namespace simulate
