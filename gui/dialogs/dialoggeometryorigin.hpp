#pragma once

#include "sme/voxel.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogGeometryOrigin;
}

class DialogGeometryOrigin : public QDialog {
  Q_OBJECT

public:
  explicit DialogGeometryOrigin(const sme::common::VoxelF &origin,
                                const QString &lengthUnit,
                                QWidget *parent = nullptr);
  ~DialogGeometryOrigin() override;
  [[nodiscard]] const sme::common::VoxelF &getOrigin() const;

private:
  std::unique_ptr<Ui::DialogGeometryOrigin> ui;
  sme::common::VoxelF origin;

  void acceptIfValid();
};
