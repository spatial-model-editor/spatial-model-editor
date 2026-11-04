#pragma once

#include <QTimer>
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

class SmeVtkWidget : public QVTKOpenGLNativeWidget {
  Q_OBJECT
public:
  explicit SmeVtkWidget(QWidget *parent = nullptr);
  ~SmeVtkWidget() override;
  void syncCamera(SmeVtkWidget *smeVtkWidget);
  void vtkRender();

protected:
  void showEvent(QShowEvent *event) override;

  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
  vtkNew<vtkAxesActor> axesActor;
  vtkNew<vtkOrientationMarkerWidget> axesWidget;

private:
  QTimer cameraRenderTimer;
  vtkSmartPointer<vtkCamera> observedCamera;
  unsigned long cameraObserverTag{0};
  unsigned long renderObserverTag{0};
  bool cameraRenderPending{false};
  void observeCamera();
  void scheduleCameraRender();
  void cameraRendered();
};
