#pragma once

#include "sme/gmsh.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogImportGeometryGmsh;
}

class DialogImportGeometryGmsh : public QDialog {
  Q_OBJECT

public:
  explicit DialogImportGeometryGmsh(int maxVoxelsPerDimension = 50,
                                    const QString &filename = {},
                                    QWidget *parent = nullptr);
  ~DialogImportGeometryGmsh() override;

  [[nodiscard]] const sme::common::ImageStack &getImage() const;
  [[nodiscard]] const std::optional<sme::mesh::GMSHMesh> &getMesh() const;

private:
  std::unique_ptr<Ui::DialogImportGeometryGmsh> ui;
  std::optional<sme::mesh::GMSHMesh> mesh;
  sme::common::ImageStack image;

  void spinMaxDimension_valueChanged(int value);
  void chkIncludeBackground_stateChanged(int state);
  void chkGrid_stateChanged(int state);
  void chkScale_stateChanged(int state);

  void setStatusError(const QString &message);
  void setStatusInfo(const QString &message);
  void loadMesh(const QString &filename);
  void voxelizeMesh();
};
