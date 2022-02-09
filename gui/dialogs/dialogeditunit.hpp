#pragma once

#include "sme/model_units.hpp"
#include <QDialog>
#include <memory>

namespace Ui {
class DialogEditUnit;
}

class DialogEditUnit : public QDialog {
  Q_OBJECT

public:
  explicit DialogEditUnit(const sme::model::Unit &unit,
                          const QString &unitType = {},
                          QWidget *parent = nullptr);
  ~DialogEditUnit();
  const sme::model::Unit &getUnit() const;

private:
  std::unique_ptr<Ui::DialogEditUnit> ui;
  sme::model::Unit u;
  bool validMultipler{true};
  bool validScale{true};
  bool validExponent{true};
  void updateLblBaseUnits();
  void setIsValidState(QWidget *widget, bool valid,
                       const QString &errorMessage = {});
  void txtName_textEdited(const QString &text);
  void txtMultiplier_textEdited(const QString &text);
  void txtScale_textEdited(const QString &text);
  void txtExponent_textEdited(const QString &text);
};
