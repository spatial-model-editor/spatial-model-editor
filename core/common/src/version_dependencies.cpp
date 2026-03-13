#include "sme/version.hpp"
#include <QtGlobal>
#ifdef SME_WITH_CUDA
#include <cuda.h>
#endif
#include <CGAL/version_macros.h>
#include <boost/version.hpp>
#include <bzlib.h>
#include <cereal/version.hpp>
#include <dune-copasi-config.hh>
#include <expat.h>
#include <fmt/core.h>
#include <gmp.h>
#include <llvm/Config/llvm-config.h>
#include <mpfr.h>
#include <nlopt.hpp>
#include <omex/common/libcombine-version.h>
#include <oneapi/tbb/version.h>
#include <opencv2/opencv.hpp>
#include <pagmo/config.hpp>
#include <sbml/common/libsbml-version.h>
#include <scotch.h>
#include <spdlog/version.h>
#include <symengine/symengine_config.h>
#include <tiffvers.h>
#include <zlib.h>

namespace {

std::string makeVersion(int major, int minor, int patch) {
  return std::to_string(major) + "." + std::to_string(minor) + "." +
         std::to_string(patch);
}

std::string makeVersion(int major, int minor) {
  return std::to_string(major) + "." + std::to_string(minor);
}

std::string tiffVersion() {
  std::string version{TIFFLIB_VERSION_STR};
  constexpr auto prefix{"Version "};
  auto prefixPos{version.find(prefix)};
  if (prefixPos != std::string::npos) {
    version.erase(0, prefixPos + std::char_traits<char>::length(prefix));
  }
  return version;
}

#ifdef SME_WITH_CUDA
std::string cudaVersion() {
  return makeVersion(CUDA_VERSION / 1000, (CUDA_VERSION % 1000) / 10);
}
#endif

} // namespace

namespace sme::common {

std::vector<DependencyVersion> getCoreDependencyVersions() {
  std::vector<DependencyVersion> versions{{
      {"dune-copasi", "https://gitlab.dune-project.org/copasi/dune-copasi",
       makeVersion(DUNE_COPASI_VERSION_MAJOR, DUNE_COPASI_VERSION_MINOR,
                   DUNE_COPASI_VERSION_REVISION)},
      {"libSBML", "https://sbml.org/software/libsbml/",
       libsbml::getLibSBMLDottedVersion()},
      {"Qt", "https://qt.io", QT_VERSION_STR},
      {"spdlog", "https://github.com/gabime/spdlog",
       makeVersion(SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH)},
      {"fmt", "https://github.com/fmtlib/fmt",
       makeVersion(FMT_VERSION / 10000, (FMT_VERSION % 10000) / 100,
                   FMT_VERSION % 100)},
      {"SymEngine", "https://github.com/symengine/symengine",
       SYMENGINE_VERSION},
      {"LLVM core", "https://llvm.org", LLVM_VERSION_STRING},
      {"GMP", "https://gmplib.org",
       makeVersion(__GNU_MP_VERSION, __GNU_MP_VERSION_MINOR,
                   __GNU_MP_VERSION_PATCHLEVEL)},
      {"MPFR", "https://www.mpfr.org/",
       makeVersion(MPFR_VERSION_MAJOR, MPFR_VERSION_MINOR,
                   MPFR_VERSION_PATCHLEVEL)},
      {"Boost", "https://www.boost.org/",
       makeVersion(BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000,
                   BOOST_VERSION % 100)},
      {"CGAL", "https://www.cgal.org/",
       makeVersion(CGAL_VERSION_MAJOR, CGAL_VERSION_MINOR, CGAL_VERSION_PATCH)},
      {"libTIFF", "http://www.libtiff.org/", tiffVersion()},
      {"expat", "https://libexpat.github.io/",
       makeVersion(XML_MAJOR_VERSION, XML_MINOR_VERSION, XML_MICRO_VERSION)},
      {"oneTBB", "https://github.com/oneapi-src/oneTBB",
       makeVersion(TBB_VERSION_MAJOR, TBB_VERSION_MINOR, TBB_VERSION_PATCH)},
      {"OpenCV", "https://github.com/opencv/opencv",
       makeVersion(CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION)},
      {"cereal", "https://uscilab.github.io/cereal",
       makeVersion(CEREAL_VERSION_MAJOR, CEREAL_VERSION_MINOR,
                   CEREAL_VERSION_PATCH)},
      {"zlib", "https://zlib.net/",
       makeVersion(ZLIB_VER_MAJOR, ZLIB_VER_MINOR, ZLIB_VER_REVISION)},
      {"bzip2", "https://www.sourceware.org/bzip2", BZ2_bzlibVersion()},
      {"pagmo", "https://esa.github.io/pagmo2",
       makeVersion(PAGMO_VERSION_MAJOR, PAGMO_VERSION_MINOR,
                   PAGMO_VERSION_PATCH)},
      {"zipper", "https://github.com/fbergmann/zipper", "master"},
      {"libCombine", "https://github.com/sbmlteam/libCombine",
       libcombine::getLibCombineDottedVersion()},
      {"scotch", "https://gitlab.inria.fr/scotch/scotch",
       makeVersion(SCOTCH_VERSION, SCOTCH_RELEASE, SCOTCH_PATCHLEVEL)},
      {"nlopt", "https://github.com/stevengj/nlopt",
       makeVersion(nlopt::version_major(), nlopt::version_minor(),
                   nlopt::version_bugfix())},
  }};
#ifdef SME_WITH_CUDA
  versions.push_back(
      {"CUDA", "https://developer.nvidia.com/cuda-toolkit", cudaVersion()});
#endif
  return versions;
}

} // namespace sme::common
