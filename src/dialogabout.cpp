#include "dialogabout.hpp"

#include <llvm/Config/llvm-config.h>

#include "dune.hpp"
#include "logger.hpp"
#include "qcustomplot.h"
#include "sbml.hpp"
#include "symbolic.hpp"
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

  QString libraries("<ul>");
  libraries.append(
      dep("dune-copasi", "https://gitlab.dune-project.org/copasi/", "master"));
  libraries.append(dep("libSBML[experimental]",
                       "http://sbml.org/Software/libSBML",
                       libsbml::getLibSBMLDottedVersion()));
  libraries.append(dep("Qt", "https://qt.io", QT_VERSION_STR));
  libraries.append(dep("QCustomPlot", "https://www.qcustomplot.com",
                       QCUSTOMPLOT_VERSION_STR));
  libraries.append(dep("spdlog", "https://github.com/gabime/spdlog",
                       SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH));
  libraries.append(dep("fmt", "https://github.com/fmtlib/fmt",
                       QString::number(FMT_VERSION)));
  libraries.append(dep("SymEngine", "https://github.com/symengine/symengine",
                       SYMENGINE_VERSION));
  libraries.append(dep("LLVM core", "https://llvm.org", LLVM_VERSION_STRING));
  libraries.append(dep("GMP", "https://gmplib.org", __GNU_MP_VERSION,
                       __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL));
  libraries.append(
      dep("Triangle", "http://www.cs.cmu.edu/~quake/triangle.html", "1.6"));
  libraries.append(
      dep("muParser", "https://github.com/beltoforion/muparser", MUP_VERSION));
  libraries.append(dep("libTIFF", "http://www.libtiff.org/", TIFFGetVersion()));
  for (const auto& d : {"expat", "libxml", "xerces-c", "bzip2", "zip"}) {
    if (libsbml::isLibSBMLCompiledWith(d) != 0) {
      libraries.append(dep(d, "", libsbml::getLibSBMLDependencyVersionOf(d)));
    }
  }
  libraries.append(
      dep("ExprTk", "https://github.com/ArashPartow/exprtk", "master"));
  libraries.append("</ul>");
  ui->txtLibraries->setText(libraries);
}
