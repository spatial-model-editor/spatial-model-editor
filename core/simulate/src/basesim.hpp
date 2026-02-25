// Simulator interface class

#pragma once

#include "sme/image_stack.hpp"
#include <QImage>
#include <string>
#include <vector>

namespace sme::simulate {

/**
 * @brief Abstract interface implemented by simulation backends.
 */
class BaseSim {
public:
  /**
   * @brief Virtual destructor.
   */
  virtual ~BaseSim() = default;
  /**
   * @brief Run simulation for ``time`` with optional timeout/stop callback.
   */
  virtual std::size_t run(double time, double timeout_ms,
                          const std::function<bool()> &stopRunningCallback) = 0;
  /**
   * @brief Flattened concentrations for compartment.
   */
  [[nodiscard]] virtual const std::vector<double> &
  getConcentrations(std::size_t compartmentIndex) const = 0;
  /**
   * @brief Concentration array padding.
   */
  [[nodiscard]] virtual std::size_t getConcentrationPadding() const = 0;
  /**
   * @brief Current backend error message.
   */
  [[nodiscard]] virtual const std::string &errorMessage() const = 0;
  /**
   * @brief Current backend error images.
   */
  [[nodiscard]] virtual const common::ImageStack &errorImages() const = 0;
  /**
   * @brief Set current error message.
   */
  virtual void setCurrentErrormessage(const std::string &msg) = 0;
  /**
   * @brief Returns whether stop has been requested.
   */
  [[nodiscard]] virtual bool getStopRequested() const = 0;
  /**
   * @brief Request/clear stop flag.
   */
  virtual void setStopRequested(bool stoprequested) = 0;
};

} // namespace sme::simulate
