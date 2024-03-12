#include <QString>
#include <vector>

namespace sme::geometry {

void fillMissingByDilation(std::vector<std::size_t> &arr, int nx, int ny,
                           int nz, std::size_t invalidIndex);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
void saveDebuggingIndicesImageXY(const std::vector<std::size_t> &arrayPoints,
                                 int nx, int ny, int nz, std::size_t maxIndex,
                                 const QString &filename);
#endif
} // namespace sme::geometry
