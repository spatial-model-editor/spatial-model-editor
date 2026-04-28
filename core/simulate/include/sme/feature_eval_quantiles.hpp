#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace sme::simulate {

/**
 * @brief Class for computing quantiles. Features an internal buffer in order to
 * avoid reordering the original data. The implementation is numerically
 * equivalent to python's pandas' 'quantile' function with interpolation method
 * 'nearest'.
 *
 */
class Quantile {
  double _quantile_value;
  std::vector<double> _copied_data;

public:
  /**
   * @brief Set the quantile object
   *
   * @param q
   */
  auto set_quantile(double q) -> void;

  /**
   * @brief Get the quantile object
   *
   * @return double
   */
  auto get_quantile() -> double;

  /**
   * @brief Clear the interal buffer
   *
   */
  auto clear() -> void;

  /**
   * @brief Reserves space in the internal buffer to avoid reallocations
   *
   */
  auto reserve(std::size_t capacity) -> void;

  /**
   * @brief Construct a new Quantile object
   *
   * @param q
   */
  Quantile(double q);

  /**
   * @brief Compute quantile on internal buffer
   *
   * @return double
   */
  auto compute() -> double;

  /**
   * @brief add data to internal buffer
   *
   * @param v
   */
  auto add_data(double v) -> void;
};

} // namespace sme::simulate
