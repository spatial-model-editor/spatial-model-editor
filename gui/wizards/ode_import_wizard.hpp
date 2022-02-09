#pragma once

#include "sme/model.hpp"
#include <QWizard>
#include <memory>

namespace Ui {
class OdeImportWizard;
}

class OdeImportWizard : public QWizard {
  Q_OBJECT

public:
  explicit OdeImportWizard(sme::model::Model &smeModel,
                           QWidget *parent = nullptr);
  ~OdeImportWizard() override;
  [[nodiscard]] double getInitialPixelDepth() const;
  [[nodiscard]] const std::vector<sme::model::ReactionRescaling> &
  getReactionRescalings() const;

private:
  std::unique_ptr<Ui::OdeImportWizard> ui;
  sme::model::Model &model;
  double initialDepth{1.0};
  std::vector<sme::model::ReactionRescaling> reactionRescalings{};
  void updateDepth(const QString &text);
};
