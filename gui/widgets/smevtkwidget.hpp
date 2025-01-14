#pragma once

#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkRenderer.h>

class SmeVtkWidget : public QVTKOpenGLNativeWidget {
  Q_OBJECT
protected:
  explicit SmeVtkWidget(QWidget *parent = nullptr);
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
  vtkNew<vtkAxesActor> axesActor;
  vtkNew<vtkOrientationMarkerWidget> axesWidget;
};
