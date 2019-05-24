#include <QErrorMessage>
#include <QMessageBox>
#include <QString>
#include <QFileDialog>
#include <QStringListModel>
#include <QInputDialog>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sbml.h"

#include "numerics.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // for debugging: import an image on startup
    ui->lblGeometry->importGeometry("../spatial-model-editor/two-blobs-100x100.bmp");

    compartmentMenu = std::unique_ptr<QMenu>(new QMenu());

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_action_About_triggered()
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("About");
    QString info("Spatial Model Editor\n");
    info.append("github.com/lkeegan/spatial-model-editor\n\n");
    info.append("Included libraries:\n");
    info.append("\nQt5:\t");
    info.append(QT_VERSION_STR);
    info.append("\nlibSBML:\t");
    info.append(libsbml::getLibSBMLDottedVersion());
    for(const auto& dep : {"expat", "libxml", "xerces-c", "bzip2", "zip"}){
        if(libsbml::isLibSBMLCompiledWith(dep) != 0){
            info.append("\n");
            info.append(dep);
            info.append(":\t");
            info.append(libsbml::getLibSBMLDependencyVersionOf(dep));
        }
    }
    msgBox.setText(info);
    msgBox.exec();
}

void MainWindow::on_actionE_xit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_action_Open_SBML_file_triggered()
{
    QString filename = QFileDialog::getOpenFileName();
    if(!filename.isEmpty()){
        sbml_doc.loadFile(qPrintable(filename));
    }

    ui->txtSBML->setText(sbml_doc.xml);

    ui->listReactions->clear();
    ui->listReactions->insertItems(0, sbml_doc.reactions);

    // update possible compartments in compartmentMenu
    compartmentMenu->clear();
    compartmentMenu->addAction("none");
    for (auto c : sbml_doc.compartments){
        compartmentMenu->addAction(c);
    }
    ui->btnChangeCompartment->setMenu(compartmentMenu.get());
}

void MainWindow::on_action_Save_SBML_file_triggered()
{
    QMessageBox msgBox;
    msgBox.setText("todo");
    msgBox.exec();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionGeometry_from_image_triggered()
{
    QString filename = QFileDialog::getOpenFileName();
    ui->lblGeometry->importGeometry(filename);
}

void MainWindow::on_lblGeometry_mouseClicked()
{
    QPalette sample_palette;
    sample_palette.setColor(QPalette::Window, QColor::fromRgb(ui->lblGeometry->colour));
    ui->lblCompartmentColour->setPalette(sample_palette);
    QString compID = ui->lblGeometry->compartmentID;
    ui->listSpecies->clear();
    ui->btnChangeCompartment->setText(compID);
    // update species info to this compartment
    ui->listSpecies->insertItems(0, sbml_doc.species[compID]);
    // show species tab
    ui->tabMain->setCurrentIndex(1);
}

void MainWindow::on_chkEnableSpatial_stateChanged(int arg1)
{
    ui->grpSpatial->setEnabled(arg1);
}


void MainWindow::on_chkShowSpatialAdvanced_stateChanged(int arg1)
{
    ui->grpSpatialAdavanced->setEnabled(arg1);
}

void MainWindow::on_listSpecies_currentTextChanged(const QString &currentText)
{
    if(currentText.size()>0){
        qDebug() << currentText;
        auto* spec = sbml_doc.doc->getModel()->getSpecies(qPrintable(currentText));
        ui->txtInitialConcentration->setText(QString::number(spec->getInitialConcentration()));
    }
}

void MainWindow::on_btnChangeCompartment_triggered(QAction *arg1)
{
    QString compID = arg1->text();
    ui->lblGeometry->colour_to_comp[ui->lblGeometry->colour] = compID;
    ui->lblGeometry->compartmentID = compID;
    on_lblGeometry_mouseClicked();
}

void MainWindow::on_listReactions_currentTextChanged(const QString &currentText)
{
    ui->listProducts->clear();
    ui->listReactants->clear();
    ui->lblReactionRate->clear();
    if(currentText.size()>0){
        qDebug() << currentText;
        const auto* reac = sbml_doc.doc->getModel()->getReaction(qPrintable(currentText));
        for(unsigned i=0; i<reac->getNumProducts(); ++i){
            ui->listProducts->addItem(reac->getProduct(i)->getSpecies().c_str());
        }
        for(unsigned i=0; i<reac->getNumReactants(); ++i){
            ui->listReactants->addItem(reac->getReactant(i)->getSpecies().c_str());
        }
        ui->lblReactionRate->setText(reac->getKineticLaw()->getFormula().c_str());
    }
}
