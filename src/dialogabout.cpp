#include "dialogabout.hpp"

#include <expat.h>
#include <fmt/core.h>
#include <gmp.h>
#include <llvm/Config/llvm-config.h>
#include <muParserDef.h>
#include <sbml/common/libsbml-version.h>
#include <spdlog/version.h>
#include <symengine/symengine_config.h>
#include <tiffvers.h>

#include "ui_dialogabout.h"
#include "version.hpp"

static QString dep(const QString& name, const QString& url,
                   const QString& version) {
  return QString("<li><a href=\"%2\">%1</a>: %3</li>").arg(name, url, version);
}

static QString dep(const QString& name, const QString& url, int major,
                   int minor, int patch) {
  auto version = QString("%1.%2.%3</li>")
                     .arg(QString::number(major), QString::number(minor),
                          QString::number(patch));
  return dep(name, url, version);
}

DialogAbout::DialogAbout(QWidget* parent)
    : QDialog(parent), ui(new Ui::DialogAbout) {
  ui->setupUi(this);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogAbout::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogAbout::reject);

  ui->lblAbout->setText(QString("<h3>Spatial Model Editor %1</h3>")
                            .arg(SPATIAL_MODEL_EDITOR_VERSION));

  QString libraries("<p>Included libraries:</p><ul>");
  libraries.append(
      dep("dune-copasi", "https://gitlab.dune-project.org/copasi/", "0.1.0"));
  libraries.append(dep("libSBML[experimental]",
                       "http://sbml.org/Software/libSBML",
                       libsbml::getLibSBMLDottedVersion()));
  libraries.append(dep("Qt", "https://qt.io", QT_VERSION_STR));
  libraries.append(dep("QCustomPlot", "https://www.qcustomplot.com", "2.0.1"));
  libraries.append(dep("spdlog", "https://github.com/gabime/spdlog",
                       SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH));
  libraries.append(dep("fmt", "https://github.com/fmtlib/fmt",
                       FMT_VERSION / 10000, (FMT_VERSION % 10000) / 100,
                       FMT_VERSION % 100));
  libraries.append(dep("SymEngine", "https://github.com/symengine/symengine",
                       SYMENGINE_VERSION));
  libraries.append(dep("LLVM core", "https://llvm.org", LLVM_VERSION_STRING));
  libraries.append(dep("GMP", "https://gmplib.org", __GNU_MP_VERSION,
                       __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL));
  libraries.append(
      dep("Triangle", "http://www.cs.cmu.edu/~quake/triangle.html", "1.6"));
  libraries.append(
      dep("muParser", "https://github.com/beltoforion/muparser", MUP_VERSION));
  libraries.append(dep("libTIFF", "http://www.libtiff.org/",
                       QString(TIFFLIB_VERSION_STR).left(23).right(6)));
  libraries.append(dep("expat", "https://libexpat.github.io/",
                       XML_MAJOR_VERSION, XML_MINOR_VERSION,
                       XML_MICRO_VERSION));
  libraries.append("</ul>");
  ui->lblLibraries->setText(libraries);
}
