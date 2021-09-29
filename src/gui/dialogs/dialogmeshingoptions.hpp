#pragma once
#include <QDialog>
#include <memory>

namespace Ui {
class DialogMeshingOptions;
}

class DialogMeshingOptions : public QDialog {
  Q_OBJECT

public:
  explicit DialogMeshingOptions(std::size_t boundarySimplificationType,
                                QWidget *parent = nullptr);
  ~DialogMeshingOptions() override;
  std::size_t getBoundarySimplificationType() const;

private:
  std::unique_ptr<Ui::DialogMeshingOptions> ui;
};
