#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QString>
#include <sbml/SBMLTypes.h>
#include<vector>
#include<string>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_About_triggered()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("About");
    QString versions("Spatial Model Editor\n\nIncluded libraries:\n\nlibSBML:\t");
    versions.append(libsbml::getLibSBMLDottedVersion());
    for(const auto& dep : {"libxml", "bzip2", "zip"}){
        versions.append("\n");
        versions.append(dep);
        versions.append(":\t");
        versions.append(libsbml::getLibSBMLDependencyVersionOf(dep));
    }
    msgBox.setText(versions);
    msgBox.exec();
}
