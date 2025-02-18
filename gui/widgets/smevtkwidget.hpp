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
public:
  explicit SmeVtkWidget(QWidget *parent = nullptr);
  void syncCamera(SmeVtkWidget *smeVtkWidget);

protected:
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
  vtkNew<vtkAxesActor> axesActor;
  vtkNew<vtkOrientationMarkerWidget> axesWidget;

private:
  void renderOnInteractorModified(vtkRenderWindowInteractor *interactor);
};
