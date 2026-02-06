#include "catch_wrapper.hpp"
#include "model_test_utils.hpp"
#include "sme/duneconverter.hpp"
#include "sme/gmsh.hpp"
#include "sme/utils.hpp"
#include <QDir>
#include <QFile>
#include <algorithm>
#include <array>
#include <limits>
#include <locale>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>

using namespace sme;

namespace {

[[nodiscard]] QString writeTmpFile(const QString &filename,
                                   const QString &contents) {
  const auto path = QDir::current().filePath(filename);
  QFile::remove(path);
  QFile f(path);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return {};
  }
  const auto bytes = contents.toUtf8();
  if (f.write(bytes) != bytes.size()) {
    return {};
  }
  return path;
}

class ScopedTestLocale {
public:
  explicit ScopedTestLocale(std::locale newLocale)
      : previousLocale(std::locale::global(std::move(newLocale))) {}
  ~ScopedTestLocale() { std::locale::global(previousLocale); }

private:
  std::locale previousLocale;
};

} // namespace

TEST_CASE("Gmsh voxel import",
          "[core/mesh/gmsh][core/mesh][core][mesh][gmsh]") {
  SECTION("invalid file") {
    auto filename = writeTmpFile("tmp_invalid.msh", "not a gmsh mesh");
    REQUIRE(QFile::exists(filename));
    auto gmshMesh = mesh::readGMSHMesh(filename);
    REQUIRE(!gmshMesh.has_value());
    common::ImageStack img;
    if (gmshMesh.has_value()) {
      img = mesh::voxelizeGMSHMesh(*gmshMesh, 20);
    }
    REQUIRE(img.empty());
  }

  SECTION("single tetrahedron uses first indexed color") {
    const QString gmsh = R"($MeshFormat
2.2 0 8
$EndMeshFormat
$Nodes
4
1 0 0 0
2 1 0 0
3 0 1 0
4 0 0 1
$EndNodes
$Elements
1
1 4 2 7 7 1 2 3 4
$EndElements
)";
    auto filename = writeTmpFile("tmp_single_tetra.msh", gmsh);
    REQUIRE(QFile::exists(filename));
    auto gmshMesh = mesh::readGMSHMesh(filename);
    REQUIRE(gmshMesh.has_value());
    REQUIRE(gmshMesh->vertices.size() == 4);
    REQUIRE(gmshMesh->tetrahedra.size() == 1);

    auto img = mesh::voxelizeGMSHMesh(*gmshMesh, 20);
    REQUIRE(!img.empty());
    REQUIRE(img.volume().width() == 20);
    REQUIRE(img.volume().height() == 20);
    REQUIRE(img.volume().depth() == 20);

    const QRgb nullColor = qRgb(0, 0, 0);
    const QRgb expectedColor = common::indexedColors()[0].rgb();
    std::size_t nExpectedColorVoxels{0};
    for (std::size_t z = 0; z < img.volume().depth(); ++z) {
      for (int y = 0; y < img.volume().height(); ++y) {
        for (int x = 0; x < img.volume().width(); ++x) {
          const auto px = img[z].pixel(x, y);
          if (px == nullColor) {
            continue;
          }
          REQUIRE(px == expectedColor);
          ++nExpectedColorVoxels;
        }
      }
    }
    REQUIRE(nExpectedColorVoxels > 0);

    auto img2 = mesh::voxelizeGMSHMesh(*gmshMesh, 10);
    REQUIRE(!img2.empty());
    REQUIRE(img2.volume().width() == 10);
    REQUIRE(img2.volume().height() == 10);
    REQUIRE(img2.volume().depth() == 10);
  }

  SECTION("single tetrahedron parses under DE locale") {
    std::optional<std::locale> deLocale;
    for (const char *localeName : {"de_DE.UTF-8", "de_DE.utf8", "de_DE"}) {
      try {
        deLocale.emplace(localeName);
        break;
      } catch (const std::runtime_error &) {
      }
    }
    if (!deLocale.has_value()) {
      WARN("Skipping locale test, no DE locale available");
      return;
    }
    ScopedTestLocale localeGuard(*deLocale);

    const QString gmsh = R"($MeshFormat
2.2 0 8
$EndMeshFormat
$Nodes
4
1 0 0 0
2 1 0 0
3 0 1 0
4 0 0 1
$EndNodes
$Elements
1
1 4 2 7 7 1 2 3 4
$EndElements
)";
    auto filename = writeTmpFile("tmp_single_tetra.msh", gmsh);
    REQUIRE(QFile::exists(filename));
    auto gmshMesh = mesh::readGMSHMesh(filename);
    REQUIRE(gmshMesh.has_value());
  }

  SECTION("multiple compartments use indexed colors in ascending tag order") {
    const QString gmsh = R"($MeshFormat
2.2 0 8
$EndMeshFormat
$Nodes
8
1 0 0 0
2 1 0 0
3 0 1 0
4 0 0 1
5 3 0 0
6 4 0 0
7 3 1 0
8 3 0 1
$EndNodes
$Elements
2
1 4 2 5 5 5 6 7 8
2 4 2 2 2 1 2 3 4
$EndElements
)";
    auto filename = writeTmpFile("tmp_two_tetra_compartments.msh", gmsh);
    REQUIRE(QFile::exists(filename));
    auto gmshMesh = mesh::readGMSHMesh(filename);
    REQUIRE(gmshMesh.has_value());
    REQUIRE(gmshMesh->vertices.size() == 8);
    REQUIRE(gmshMesh->tetrahedra.size() == 2);
    auto imgWithBackground = mesh::voxelizeGMSHMesh(*gmshMesh, 20, true);
    REQUIRE(!imgWithBackground.empty());
    REQUIRE(imgWithBackground.volume().width() == 20);
    REQUIRE(imgWithBackground.volume().height() == 5);
    REQUIRE(imgWithBackground.volume().depth() == 5);
    auto imgWithoutBackground = mesh::voxelizeGMSHMesh(*gmshMesh, 20, false);
    REQUIRE(!imgWithoutBackground.empty());

    // a voxel inside the first compartment has the first indexed color
    REQUIRE(imgWithBackground[1].pixel(1, 4) ==
            common::indexedColors()[0].rgb());
    REQUIRE(imgWithoutBackground[1].pixel(1, 4) ==
            common::indexedColors()[0].rgb());
    // voxel inside second compartment
    REQUIRE(imgWithBackground[1].pixel(16, 4) ==
            common::indexedColors()[1].rgb());
    REQUIRE(imgWithoutBackground[1].pixel(16, 4) ==
            common::indexedColors()[1].rgb());
    // a voxel outside of the two compartments but closer to the first
    // compartment
    REQUIRE(imgWithBackground[4].pixel(1, 4) ==
            qRgb(0, 0, 0)); // black if background is allowed
    REQUIRE(imgWithoutBackground[4].pixel(1, 4) ==
            common::indexedColors()[0].rgb()); // nearest compartment otherwise
    // a voxel outside of the two compartments but closer to the second
    // compartment
    REQUIRE(imgWithBackground[4].pixel(16, 4) ==
            qRgb(0, 0, 0)); // black if background is allowed
    REQUIRE(imgWithoutBackground[4].pixel(16, 4) ==
            common::indexedColors()[1].rgb()); // nearest compartment otherwise

    REQUIRE(imgWithoutBackground.colorTable().size() == 2);
    REQUIRE(imgWithBackground.colorTable().size() == 3);
    REQUIRE(!imgWithoutBackground.colorTable().contains(qRgb(0, 0, 0)));
    REQUIRE(imgWithBackground.colorTable().contains(qRgb(0, 0, 0)));
  }

  SECTION("round-trip via duneconverter gmsh export") {
    constexpr double minMatchingFraction{0.80};
    for (const auto &modelId :
         {test::Mod::VerySimpleModel3D, test::Mod::SelKov3D,
          test::Mod::FitzhughNagumo3D}) {
      for (const bool includeBackground : {true, false}) {
        CAPTURE(modelId);
        CAPTURE(includeBackground);
        auto model = test::getExampleModel(modelId);
        const auto &original = model.getGeometry().getImages();
        REQUIRE(!original.empty());
        const auto originalVolume = original.volume();
        const int nMax =
            std::max({originalVolume.width(), originalVolume.height(),
                      static_cast<int>(originalVolume.depth())});
        std::map<QRgb, QRgb> originalToRoundTripColor;
        const auto &compartmentColors = model.getCompartments().getColors();
        const common::indexedColors indexedColors;
        for (int i = 0; i < compartmentColors.size(); ++i) {
          originalToRoundTripColor[compartmentColors[i]] =
              indexedColors[static_cast<std::size_t>(i)].rgb();
        }

        const QString iniFilename = QDir::current().filePath("tmp_gmsh_rt.ini");
        const QString gmshFilename =
            QDir::current().filePath("tmp_gmsh_rt.msh");
        QFile::remove(iniFilename);
        QFile::remove(gmshFilename);
        simulate::DuneConverter dc(model, {}, true, iniFilename);
        (void)dc;
        REQUIRE(QFile::exists(gmshFilename));

        auto gmshMesh = mesh::readGMSHMesh(gmshFilename);
        REQUIRE(gmshMesh.has_value());
        auto roundTrip =
            mesh::voxelizeGMSHMesh(*gmshMesh, nMax, includeBackground);
        REQUIRE(!roundTrip.empty());

        common::VoxelF meshMin{std::numeric_limits<double>::max(),
                               std::numeric_limits<double>::max(),
                               std::numeric_limits<double>::max()};
        common::VoxelF meshMax{std::numeric_limits<double>::lowest(),
                               std::numeric_limits<double>::lowest(),
                               std::numeric_limits<double>::lowest()};
        for (const auto &v : gmshMesh->vertices) {
          meshMin.p.rx() = std::min(meshMin.p.x(), v.p.x());
          meshMin.p.ry() = std::min(meshMin.p.y(), v.p.y());
          meshMin.z = std::min(meshMin.z, v.z);
          meshMax.p.rx() = std::max(meshMax.p.x(), v.p.x());
          meshMax.p.ry() = std::max(meshMax.p.y(), v.p.y());
          meshMax.z = std::max(meshMax.z, v.z);
        }
        const double w = meshMax.p.x() - meshMin.p.x();
        const double h = meshMax.p.y() - meshMin.p.y();
        const double d = meshMax.z - meshMin.z;
        const auto roundVol = roundTrip.volume();
        const auto origin = model.getGeometry().getPhysicalOrigin();
        const auto voxelSize = model.getGeometry().getVoxelSize();

        std::size_t nSame = 0;
        std::size_t nTotal = roundVol.nVoxels();
        for (std::size_t z = 0; z < roundVol.depth(); ++z) {
          double pz = meshMin.z + d * (0.5 + static_cast<double>(z)) /
                                      static_cast<double>(roundVol.depth());
          auto oz = static_cast<std::size_t>(
              common::toVoxelIndex(pz, origin.z, voxelSize.depth(),
                                   static_cast<int>(originalVolume.depth())));
          for (int y = 0; y < roundVol.height(); ++y) {
            double pyBottom = static_cast<double>(roundVol.height() - 1 - y);
            double py =
                meshMin.p.y() +
                h * (0.5 + pyBottom) / static_cast<double>(roundVol.height());
            int oyBottom = common::toVoxelIndex(
                py, origin.p.y(), voxelSize.height(), originalVolume.height());
            int oy = originalVolume.height() - 1 - oyBottom;
            for (int x = 0; x < roundVol.width(); ++x) {
              double px =
                  meshMin.p.x() + w * (0.5 + static_cast<double>(x)) /
                                      static_cast<double>(roundVol.width());
              int ox = common::toVoxelIndex(px, origin.p.x(), voxelSize.width(),
                                            originalVolume.width());
              auto originalColor = original[oz].pixel(ox, oy);
              auto mappedColorIter =
                  originalToRoundTripColor.find(originalColor);
              auto expectedRoundColor = originalColor;
              if (mappedColorIter != originalToRoundTripColor.end()) {
                expectedRoundColor = mappedColorIter->second;
              }
              if (roundTrip[z].pixel(x, y) == expectedRoundColor) {
                ++nSame;
              }
            }
          }
        }
        CAPTURE(nSame);
        CAPTURE(nTotal);
        const double matchingFraction =
            static_cast<double>(nSame) / static_cast<double>(nTotal);
        CAPTURE(matchingFraction);
        CAPTURE(minMatchingFraction);
        REQUIRE(matchingFraction >= minMatchingFraction);
      }
    }
  }
}
