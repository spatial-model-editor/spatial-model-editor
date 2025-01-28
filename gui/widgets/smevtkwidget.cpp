#include "smevtkwidget.hpp"
#include <vtkCamera.h>

SmeVtkWidget::SmeVtkWidget(QWidget *parent) : QVTKOpenGLNativeWidget(parent) {
  setRenderWindow(renderWindow);
  renderWindow->AddRenderer(renderer);
  axesWidget->SetOrientationMarker(axesActor);
  axesWidget->SetInteractor(renderWindow->GetInteractor());
  // set relative size of axes widget
  axesWidget->SetViewport(0.0, 0.0, 0.3, 0.3);
  axesWidget->SetEnabled(1);
  axesWidget->InteractiveOn();
  // inherit widget background color
  [this]() {
    float r{0};
    float g{0};
    float b{0};
    palette().color(QWidget::backgroundRole()).getRgbF(&r, &g, &b);
    renderer->SetBackground(r, g, b);
  }();
  renderer->GetActiveCamera()->Azimuth(45);
  renderer->GetActiveCamera()->Elevation(30);
}
