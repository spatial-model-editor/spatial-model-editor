#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include <QMouseEvent>
#include <vtkCamera.h>
#include <vtkPointData.h>
#include <vtkWorldPointPicker.h>

QVoxelRenderer::QVoxelRenderer(QWidget *parent) : SmeVtkWidget(parent) {
  // piecewise opacity function, converts 0-255 input value to 0-1 opacity float
  opacityTransferFunction->AddPoint(0, 0.2);
  opacityTransferFunction->AddPoint(255, 0.2);
  volume->GetProperty()->SetScalarOpacity(opacityTransferFunction);
  volume->GetProperty()->ShadeOn();
  // directly use first 3 components of data as RGB values
  volume->GetProperty()->IndependentComponentsOff();
  // don't interpolate between voxels
  volume->GetProperty()->SetInterpolationTypeToNearest();
  volume->SetMapper(volumeMapper);
  volumeMapper->SetInputData(imageData);
  renderer->AddVolume(volume);
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
    // iterate over y-pixels in reverse order to match VTK's coordinate system
    for (int y = dims[1] - 1; y >= 0; --y) {
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
  double avgVoxelLength = pow(img.voxelSize().volume(), 1.0 / 3.0);
  SPDLOG_TRACE("Setting opacity unit distance to {} ({})", avgVoxelLength,
               img.voxelSize().width());
  volume->GetProperty()->SetScalarOpacityUnitDistance(3, avgVoxelLength);
  imageData->SetSpacing(img.voxelSize().width(), img.voxelSize().height(),
                        img.voxelSize().depth());
  imageData->SetOrigin(0.0, 0.0, 0.0);
  imageData->GetPointData()->SetScalars(imageDataArray);
  renderer->ResetCameraClippingRange();
  renderer->ResetCamera();
  renderWindow->Render();
}

void QVoxelRenderer::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() == Qt::NoButton) {
    return;
  }
  auto *interactor = renderWindow->GetInteractor();
  auto *picker = interactor->GetPicker();
  const auto *pos = interactor->GetEventPosition();
  if (picker->Pick(pos[0], pos[1], 0, renderer)) {
    std::array<double, 3> picked{};
    picker->GetPickPosition(picked.data());
    std::array<int, 3> dim{};
    imageData->GetDimensions(dim.data());
    auto x = static_cast<int>(picked[0]);
    auto y = static_cast<int>(picked[1]);
    auto z = static_cast<std::size_t>(picked[2]);
    if (static_cast<int>(z) < dim[2] && y >= 0 && y < dim[1] && x >= 0 &&
        x < dim[0]) {
      static constexpr vtkIdType stride = 4;
      auto idx =
          stride * static_cast<vtkIdType>(z * dim[1] * dim[0] + y * dim[1] + x);
      SPDLOG_DEBUG("Index in imageDataArray: {}", idx);
      auto col =
          qRgb(imageDataArray->GetValue(idx), imageDataArray->GetValue(idx + 1),
               imageDataArray->GetValue(idx + 2));
      SPDLOG_DEBUG("voxel ({},{},{}) -> colour {:x}", x, y, z, col);
      emit mouseClicked(col, sme::common::Voxel{x, y, z});
    }
  }
}
