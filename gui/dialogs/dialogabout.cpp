#include "dialogabout.hpp"
#include "sme/symbolic.hpp"
#include "sme/version.hpp"
#include "ui_dialogabout.h"
#include <CGAL/version_macros.h>
#include <boost/version.hpp>
#include <bzlib.h>
#include <cereal/version.hpp>
#include <dune/copasi/config.h>
#include <expat.h>
#include <fmt/core.h>
#include <gmp.h>
#include <mpfr.h>
#include <muParserDef.h>
#include <oneapi/tbb/version.h>
#include <opencv2/opencv.hpp>
#include <pagmo/config.hpp>
#include <qcustomplot.h>
#include <sbml/common/libsbml-version.h>
#include <spdlog/version.h>
#include <symengine/symengine_config.h>
#include <tiffvers.h>
#include <zlib.h>

static QString dep(const QString &name, const QString &url,
                   const QString &version) {
  return QString("<li><a href=\"%2\">%1</a>: %3</li>").arg(name, url, version);
}

static QString dep(const QString &name, const QString &url, int major,
                   int minor) {
  auto version =
      QString("%1.%2</li>").arg(QString::number(major), QString::number(minor));
  return dep(name, url, version);
}

static QString dep(const QString &name, const QString &url, int major,
                   int minor, int patch) {
  auto version = QString("%1.%2.%3</li>")
                     .arg(QString::number(major), QString::number(minor),
                          QString::number(patch));
  return dep(name, url, version);
}

DialogAbout::DialogAbout(QWidget *parent)
    : QDialog(parent), ui{std::make_unique<Ui::DialogAbout>()} {
  ui->setupUi(this);

  ui->lblLogo->setPixmap(QPixmap(":/icon/icon128.png"));
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
          &DialogAbout::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
          &DialogAbout::reject);

  ui->lblAbout->setText(QString("<h3>Spatial Model Editor %1</h3>")
                            .arg(sme::common::SPATIAL_MODEL_EDITOR_VERSION));

  QString libraries("<p>Included libraries:</p><ul>");
  libraries.append(dep("dune-copasi",
                       "https://gitlab.dune-project.org/copasi/dune-copasi",
                       DUNE_COPASI_VERSION_MAJOR, DUNE_COPASI_VERSION_MINOR,
                       DUNE_COPASI_VERSION_REVISION));
  libraries.append(dep("libSBML", "https://sbml.org/software/libsbml/",
                       libsbml::getLibSBMLDottedVersion()));
  libraries.append(dep("Qt", "https://qt.io", QT_VERSION_STR));
  libraries.append(dep("QCustomPlot", "https://www.qcustomplot.com",
                       QCUSTOMPLOT_VERSION_STR));
  libraries.append(dep("spdlog", "https://github.com/gabime/spdlog",
                       SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH));
  libraries.append(dep("fmt", "https://github.com/fmtlib/fmt",
                       FMT_VERSION / 10000, (FMT_VERSION % 10000) / 100,
                       FMT_VERSION % 100));
  libraries.append(dep("SymEngine", "https://github.com/symengine/symengine",
                       SYMENGINE_VERSION));
  libraries.append(dep("LLVM core", "https://llvm.org",
                       sme::common::Symbolic::getLLVMVersion()));
  libraries.append(dep("GMP", "https://gmplib.org", __GNU_MP_VERSION,
                       __GNU_MP_VERSION_MINOR, __GNU_MP_VERSION_PATCHLEVEL));
  libraries.append(dep("MPFR", "https://www.mpfr.org/", MPFR_VERSION_MAJOR,
                       MPFR_VERSION_MINOR, MPFR_VERSION_PATCHLEVEL));
  libraries.append(dep("Boost", "https://www.boost.org/",
                       BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000,
                       BOOST_VERSION % 100));
  libraries.append(dep("CGAL", "https://www.cgal.org/", CGAL_VERSION_MAJOR,
                       CGAL_VERSION_MINOR, CGAL_VERSION_PATCH));
  libraries.append(dep("muParser", "https://github.com/beltoforion/muparser",
                       mu::ParserVersion.c_str()));
  libraries.append(dep("libTIFF", "http://www.libtiff.org/",
                       QString(TIFFLIB_VERSION_STR).left(23).right(6)));
  libraries.append(dep("expat", "https://libexpat.github.io/",
                       XML_MAJOR_VERSION, XML_MINOR_VERSION,
                       XML_MICRO_VERSION));
  libraries.append(dep("oneTBB", "https://github.com/oneapi-src/oneTBB",
                       TBB_VERSION_MAJOR, TBB_VERSION_MINOR,
                       TBB_VERSION_PATCH));
  libraries.append(dep("OpenCV", "https://github.com/opencv/opencv",
                       CV_MAJOR_VERSION, CV_MINOR_VERSION,
                       CV_SUBMINOR_VERSION));
  libraries.append(dep("cereal", "https://uscilab.github.io/cereal",
                       CEREAL_VERSION_MAJOR, CEREAL_VERSION_MINOR,
                       CEREAL_VERSION_PATCH));
  libraries.append(dep("zlib", "https://zlib.net/", ZLIB_VER_MAJOR,
                       ZLIB_VER_MINOR, ZLIB_VER_REVISION));
  libraries.append(dep("bzip2", "https://www.sourceware.org/bzip2",
                       BZ_API(BZ2_bzlibVersion)()));
  libraries.append("</ul>");
  ui->lblLibraries->setText(libraries);
}

DialogAbout::~DialogAbout() = default;
