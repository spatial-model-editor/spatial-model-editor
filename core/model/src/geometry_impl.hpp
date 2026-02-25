#include <QString>
#include <vector>

namespace sme::geometry {

/**
 * @brief Replace invalid entries in index array using neighbor dilation.
 */
void fillMissingByDilation(std::vector<std::size_t> &arr, int nx, int ny,
                           int nz, std::size_t invalidIndex);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
/**
 * @brief Save XY debugging image for compartment index arrays.
 */
void saveDebuggingIndicesImageXY(const std::vector<std::size_t> &arrayPoints,
                                 int nx, int ny, int nz, std::size_t maxIndex,
                                 const QString &filename);
#endif
} // namespace sme::geometry
