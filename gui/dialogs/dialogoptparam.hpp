#pragma once
#include "sme/optimize_options.hpp"
#include <QDialog>
#include <QStringList>
#include <memory>

namespace Ui {
class DialogOptParam;
}

class DialogOptParam : public QDialog {
  Q_OBJECT

public:
  explicit DialogOptParam(
      const std::vector<sme::simulate::OptParam> &defaultOptParams,
      const sme::simulate::OptParam *initialOptParam = nullptr,
      QWidget *parent = nullptr);
  ~DialogOptParam() override;
  [[nodiscard]] const sme::simulate::OptParam &getOptParam() const;

private:
  const std::vector<sme::simulate::OptParam> &m_defaultOptParams;
  sme::simulate::OptParam m_optParam{};
  std::unique_ptr<Ui::DialogOptParam> ui;
  void cmbParameter_currentIndexChanged(int index);
};
