// TabMembranes

#pragma once

#include <QWidget>
#include <memory>

namespace Ui {
class TabMembranes;
}
namespace sbml {
class SbmlDocWrapper;
}

class TabMembranes : public QWidget {
  Q_OBJECT

 public:
  explicit TabMembranes(sbml::SbmlDocWrapper &doc, QWidget *parent = nullptr);
  ~TabMembranes();
  void loadModelData();

 private:
  std::unique_ptr<Ui::TabMembranes> ui;
  sbml::SbmlDocWrapper &sbmlDoc;

  void listMembranes_currentRowChanged(int currentRow);
};
