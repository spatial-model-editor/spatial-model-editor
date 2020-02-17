#pragma once

#include <QDialog>
#include <memory>

namespace Ui {
class DialogIntegratorOptions;
}

namespace simulate {
struct IntegratorOptions;
}

class DialogIntegratorOptions : public QDialog {
  Q_OBJECT

 public:
  explicit DialogIntegratorOptions(const simulate::IntegratorOptions& options,
                                   QWidget* parent = nullptr);
  ~DialogIntegratorOptions();
  simulate::IntegratorOptions getIntegratorOptions() const;
  std::size_t getOrder() const;
  double getMaxRelErr() const;
  double getMaxAbsErr() const;
  double getMaxDt() const;

 private:
  std::unique_ptr<Ui::DialogIntegratorOptions> ui;
  void resetToDefaults();
};
