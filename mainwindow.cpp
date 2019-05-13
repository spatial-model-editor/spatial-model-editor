#include <QMessageBox>
#include <QString>
#include <QFileDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sbml/SBMLTypes.h>

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
    for(const auto& dep : {"expat", "libxml", "xerces-c", "bzip2", "zip"}){
        if(libsbml::isLibSBMLCompiledWith(dep) != 0){
            versions.append("\n");
            versions.append(dep);
            versions.append(":\t");
            versions.append(libsbml::getLibSBMLDependencyVersionOf(dep));
        }
    }
    msgBox.setText(versions);
    msgBox.exec();
}

void MainWindow::on_actionE_xit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionOpen_SBML_file_triggered()
{
    QString filename = QFileDialog::getOpenFileName();
}
