#include "dialogabout.hpp"
#include "dependency_versions.hpp"
#include "sme/version.hpp"
#include "ui_dialogabout.h"
#include <QDialogButtonBox>
#include <QPixmap>
#include <QString>

static QString dep(const QString &name, const QString &url,
                   const QString &version) {
  return QString("<li><a href=\"%2\">%1</a>: %3</li>").arg(name, url, version);
}

static QString dep(const sme::common::DependencyVersion &dependency) {
  return dep(QString::fromStdString(dependency.name),
             QString::fromStdString(dependency.url),
             QString::fromStdString(dependency.version));
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

  QString libraries{"<p>Included libraries:</p><ul>"};
  for (const auto &library : sme::common::getCoreDependencyVersions()) {
    libraries.append(dep(library));
  }
  for (const auto &library : sme::gui::getGuiDependencyVersions()) {
    libraries.append(dep(library));
  }
  libraries.append("</ul>");
  ui->lblLibraries->setText(libraries);
}

DialogAbout::~DialogAbout() = default;
