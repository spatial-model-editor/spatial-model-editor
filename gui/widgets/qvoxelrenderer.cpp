#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include <QPainter>
#include <vtkCamera.h>
#include <vtkPointData.h>

QVoxelRenderer::QVoxelRenderer(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent) {
  setRenderWindow(renderWindow);
  renderWindow->AddRenderer(renderer);
  // piecewise opacity function, converts 0-255 input value to 0-1 opacity float
  opacityTransferFunction->AddPoint(0, 0.0);
  opacityTransferFunction->AddPoint(255, 0.8);
  volumeProperty->SetScalarOpacity(opacityTransferFunction);
  volumeProperty->ShadeOn();
  // directly use first 3 components of data as RGB values
  volumeProperty->IndependentComponentsOff();
  // don't interpolate between voxels
  volumeProperty->SetInterpolationTypeToNearest();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  volumeMapper->SetInputData(imageData);
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
  renderer->AddVolume(volume);
  renderer->GetActiveCamera()->Azimuth(45);
  renderer->GetActiveCamera()->Elevation(30);
}

static vtkNew<vtkUnsignedCharArray>
imageStackToVtkArray(const sme::common::ImageStack &img,
                     std::array<int, 3> &dims) {
  vtkNew<vtkUnsignedCharArray> array;
  array->SetNumberOfComponents(4);
  dims[0] = img.volume().width();
  dims[1] = img.volume().height();
  dims[2] = static_cast<int>(img.volume().depth());
  SPDLOG_TRACE("{} x {} x {} ImageStack", dims[0], dims[1], dims[2]);
  auto nVoxels = static_cast<vtkIdType>(img.volume().nVoxels());
  if (dims[2] == 1) {
    SPDLOG_TRACE("  - single z-slice -> duplicating");
    // vtk needs at least two values in each dimension to render,
    // so if we only have a single z-slice in the stack we duplicate it to give
    // two identical z-slices for display purposes.
    dims[2] = 2;
    nVoxels *= 2;
  }
  SPDLOG_TRACE("  - constructing array with {} tuples of 4 x uchar", nVoxels);
  array->SetNumberOfTuples(nVoxels);
  int i = 0;
  for (std::size_t z = 0; z < static_cast<std::size_t>(dims[2]); ++z) {
    auto z_index = std::min(z, img.volume().depth() - 1);
    for (int y = 0; y < dims[1]; ++y) {
      for (int x = 0; x < dims[0]; ++x) {
        auto rgb = img[z_index].pixel(x, y);
        // red 0-255
        array->SetValue(i, static_cast<unsigned char>(qRed(rgb)));
        ++i;
        // green 0-255
        array->SetValue(i, static_cast<unsigned char>(qGreen(rgb)));
        ++i;
        // blue 0-255
        array->SetValue(i, static_cast<unsigned char>(qBlue(rgb)));
        // use average of rgb values for input to opacity function
        ++i;
        int rgb_av = qRed(rgb) / 3 + qGreen(rgb) / 3 + qBlue(rgb) / 3;
        array->SetValue(i, static_cast<unsigned char>(rgb_av));
        ++i;
      }
    }
  }
  SPDLOG_TRACE("  - assigned {} of {} values", i, array->GetNumberOfValues());
  return array;
}

void QVoxelRenderer::setImage(const sme::common::ImageStack &img) {
  if (!isVisible()) {
    return;
  }
  std::array<int, 3> dims{};
  imageDataArray = imageStackToVtkArray(img, dims);
  imageData->SetDimensions(dims.data());
  imageData->SetSpacing(1.0, 1.0, 1.0);
  imageData->SetOrigin(0.0, 0.0, 0.0);
  imageData->GetPointData()->SetScalars(imageDataArray);
  renderer->ResetCameraClippingRange();
  renderer->ResetCamera();
  renderWindow->Render();
}

void QVoxelRenderer::setPhysicalSize(const sme::common::VolumeF &size,
                                     const QString &units) {
  lengthUnits = units;
  std::array<int, 3> dims{};
  imageData->GetDimensions(dims.data());
  imageData->SetSpacing(size.width() / dims[0], size.height() / dims[1],
                        size.depth() / dims[2]);
  renderer->ResetCameraClippingRange();
  renderer->ResetCamera();
  renderWindow->Render();
}
