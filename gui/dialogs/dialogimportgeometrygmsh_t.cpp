#include "catch_wrapper.hpp"
#include "dialogimportgeometrygmsh.hpp"
#include "qt_test_utils.hpp"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <set>

using namespace sme::test;

namespace {

struct DialogImportGeometryGmshWidgets {
  explicit DialogImportGeometryGmshWidgets(
      const DialogImportGeometryGmsh *dialog) {
    GET_DIALOG_WIDGET(QLineEdit, txtFilename);
    GET_DIALOG_WIDGET(QPushButton, btnBrowse);
    GET_DIALOG_WIDGET(QSpinBox, spinMaxDimension);
    GET_DIALOG_WIDGET(QCheckBox, chkIncludeBackground);
    GET_DIALOG_WIDGET(QDialogButtonBox, buttonBox);
  }

  QLineEdit *txtFilename{};
  QPushButton *btnBrowse{};
  QSpinBox *spinMaxDimension{};
  QCheckBox *chkIncludeBackground{};
  QDialogButtonBox *buttonBox{};
};

std::size_t countUniqueColors(const sme::common::ImageStack &img) {
  std::set<QRgb> colors;
  for (std::size_t z = 0; z < img.volume().depth(); ++z) {
    for (int y = 0; y < img.volume().height(); ++y) {
      for (int x = 0; x < img.volume().width(); ++x) {
        colors.insert(img[z].pixel(x, y));
      }
    }
  }
  return colors.size();
}

QString writeTmpFile(const QString &filename, const QString &contents) {
  QFile::remove(filename);
  QFile f(filename);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return {};
  }
  if (const auto bytes = contents.toUtf8(); f.write(bytes) != bytes.size()) {
    return {};
  }
  return filename;
}

} // namespace

TEST_CASE(
    "DialogImportGeometryGmsh",
    "[gui/dialogs/importgeometrygmsh][gui/dialogs][gui][importgeometrygmsh]") {
  SECTION("invalid file keeps ok disabled") {
    auto filename =
        writeTmpFile("tmp_invalid_gmsh_for_dialog.msh", "not a gmsh mesh");
    REQUIRE(QFile::exists(filename));

    DialogImportGeometryGmsh dia(20);
    dia.show();
    waitFor(&dia);
    DialogImportGeometryGmshWidgets widgets(&dia);

    widgets.txtFilename->setText(filename);
    sendKeyEvents(widgets.txtFilename, {"Enter"});

    REQUIRE(dia.getImage().empty());
    REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
            false);

    QFile::remove(filename);
  }

  SECTION("valid file voxelizes and updates when resolution changes") {
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

    auto filename = writeTmpFile("tmp_single_tetra_for_dialog.msh", gmsh);
    REQUIRE(QFile::exists(filename));

    DialogImportGeometryGmsh dia(20);
    dia.show();
    waitFor(&dia);
    DialogImportGeometryGmshWidgets widgets(&dia);

    widgets.txtFilename->setText(filename);
    sendKeyEvents(widgets.txtFilename, {"Enter"});

    REQUIRE(!dia.getImage().empty());
    REQUIRE(widgets.buttonBox->button(QDialogButtonBox::Ok)->isEnabled() ==
            true);
    REQUIRE(dia.getImage().volume().width() == 20);
    REQUIRE(dia.getImage().volume().height() == 20);
    REQUIRE(dia.getImage().volume().depth() == 20);
    REQUIRE(countUniqueColors(dia.getImage()) == 2);

    widgets.chkIncludeBackground->setChecked(false);
    REQUIRE(countUniqueColors(dia.getImage()) == 1);
    widgets.spinMaxDimension->setValue(10);
    REQUIRE(dia.getImage().volume().width() == 10);
    REQUIRE(dia.getImage().volume().height() == 10);
    REQUIRE(dia.getImage().volume().depth() == 10);

    QFile::remove(filename);
  }
}
