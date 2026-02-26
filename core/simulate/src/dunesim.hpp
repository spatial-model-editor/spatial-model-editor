//  - DuneSim: wrapper around dune-copasi library

#pragma once

#include "basesim.hpp"
#include "sme/image_stack.hpp"
#include "sme/simulate_options.hpp"
#include "sme/voxel.hpp"
#include <QPointF>
#include <QSize>
#include <array>
#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace sme {

namespace model {
class Model;
}

namespace geometry {
class Compartment;
}

namespace simulate {

class DuneConverter;

template <int DuneDimensions> class DuneImpl;

/**
 * @brief DUNE-backed simulation backend.
 */
class DuneSim : public BaseSim {
private:
  std::unique_ptr<DuneImpl<2>> pDuneImpl2d;
  std::unique_ptr<DuneImpl<3>> pDuneImpl3d;
  std::string currentErrorMessage{};
  common::ImageStack currentErrorImages{};
  std::size_t numMaxThreads{0};

public:
  /**
   * @brief Construct DUNE simulator for selected compartments.
   */
  explicit DuneSim(
      const model::Model &sbmlDoc,
      const std::vector<std::string> &compartmentIds,
      const std::map<std::string, double, std::less<>> &substitutions = {});
  /**
   * @brief Destructor.
   */
  ~DuneSim() override;
  /**
   * @brief Run simulation segment.
   */
  std::size_t run(double time, double timeout_ms,
                  const std::function<bool()> &stopRunningCallback) override;

  /**
   * @brief Concentration array for compartment.
   */
  [[nodiscard]] const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const override;
  /**
   * @brief Concentration array padding.
   */
  [[nodiscard]] std::size_t getConcentrationPadding() const override;
  /**
   * @brief Current error message.
   */
  [[nodiscard]] const std::string &errorMessage() const override;
  /**
   * @brief Current error images.
   */
  [[nodiscard]] const common::ImageStack &errorImages() const override;
  /**
   * @brief Stop-request flag.
   */
  [[nodiscard]] bool getStopRequested() const override;

  /**
   * @brief Request/clear stop flag.
   */
  void setStopRequested(bool stop) override;
  /**
   * @brief Set current error message.
   */
  void setCurrentErrormessage(const std::string &msg) override;
};

} // namespace simulate

} // namespace sme
