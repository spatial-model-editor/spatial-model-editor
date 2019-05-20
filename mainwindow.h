#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_action_About_triggered();

    void on_actionE_xit_triggered();

    void on_action_Open_SBML_file_triggered();

    void on_action_Save_SBML_file_triggered();

    void on_actionAbout_Qt_triggered();

    void on_actionGeometry_from_image_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
