#include "qvoxelrenderer.hpp"
#include "sme/logger.hpp"
#include <QComboBox>
#include <QMouseEvent>
#include <QSlider>
#include <algorithm>
#include <array>
#include <vtkCamera.h>
#include <vtkPointData.h>

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
  volumeMapper->AddClippingPlane(clippingPlane);
  // initial clipping plane doesn't clip anything
  clippingPlane->SetNormal(0.0, 0.0, 1.0);
  clippingPlane->SetOrigin(0.0, 0.0, 0.0);
  renderer->AddVolume(volume);
  volumePicker->PickFromListOn();
  volumePicker->AddPickList(volume);
}

static void imageStackToVtkArray(const sme::common::ImageStack &img,
                                 vtkUnsignedCharArray *array,
                                 std::array<int, 3> &dims) {
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
  auto *values = array->GetPointer(0);
  vtkIdType i = 0;
  const auto appendPixel = [&](QRgb rgb) {
    values[i++] = static_cast<unsigned char>(qRed(rgb));
    values[i++] = static_cast<unsigned char>(qGreen(rgb));
    values[i++] = static_cast<unsigned char>(qBlue(rgb));
    // Preserve the existing component-wise rounding of the opacity input.
    values[i++] = static_cast<unsigned char>(qRed(rgb) / 3 + qGreen(rgb) / 3 +
                                             qBlue(rgb) / 3);
  };
  for (std::size_t z = 0; z < static_cast<std::size_t>(dims[2]); ++z) {
    auto z_index = std::min(z, img.volume().depth() - 1);
    const auto &slice = img[z_index];
    const auto palette = slice.colorTable();
    // iterate over y-pixels in reverse order to match VTK's coordinate system
    for (int y = dims[1] - 1; y >= 0; --y) {
      switch (slice.format()) {
      case QImage::Format_RGB32:
      case QImage::Format_ARGB32: {
        const auto *pixels =
            reinterpret_cast<const QRgb *>(slice.constScanLine(y));
        for (int x = 0; x < dims[0]; ++x) {
          appendPixel(pixels[x]);
        }
        break;
      }
      case QImage::Format_Indexed8: {
        const auto *pixels = slice.constScanLine(y);
        for (int x = 0; x < dims[0]; ++x) {
          appendPixel(palette.value(pixels[x]));
        }
        break;
      }
      default:
        for (int x = 0; x < dims[0]; ++x) {
          appendPixel(slice.pixel(x, y));
        }
        break;
      }
    }
  }
  SPDLOG_TRACE("  - assigned {} of {} values", i, array->GetNumberOfValues());
  // Direct writes do not update the array's modification time. Invalidate
  // cached scalar ranges and the GPU texture after filling the reused
  // allocation.
  array->Modified();
}

void QVoxelRenderer::setImage(const sme::common::ImageStack &img,
                              const sme::common::VoxelF &origin) {
  const std::array<double, 3> spacing{img.voxelSize().width(),
                                      img.voxelSize().height(),
                                      img.voxelSize().depth()};
  const std::array<double, 3> physicalOrigin{origin.p.x(), origin.p.y(),
                                             origin.z};
  const bool geometryChanged =
      imageData->GetPointData()->GetScalars() == nullptr ||
      imageVolume != img.volume() ||
      !std::equal(spacing.begin(), spacing.end(), imageData->GetSpacing()) ||
      !std::equal(physicalOrigin.begin(), physicalOrigin.end(),
                  imageData->GetOrigin());
  std::array<int, 3> dims{};
  imageStackToVtkArray(img, imageDataArray, dims);
  imageData->GetPointData()->SetScalars(imageDataArray);
  imageVolume = img.volume();
  if (geometryChanged) {
    imageData->SetDimensions(dims.data());
    double avgVoxelLength = pow(img.voxelSize().volume(), 1.0 / 3.0);
    SPDLOG_TRACE("Setting opacity unit distance to {} ({})", avgVoxelLength,
                 img.voxelSize().width());
    volume->GetProperty()->SetScalarOpacityUnitDistance(3, avgVoxelLength);
    imageData->SetSpacing(spacing.data());
    imageData->SetOrigin(physicalOrigin.data());
    renderer->ResetCameraClippingRange();
    renderer->ResetCamera();
  }
  if (cmbClippingPlaneNormal != nullptr &&
      slideClippingPlaneOrigin != nullptr &&
      (geometryChanged || !clippingPlaneControlsInitialized)) {
    cmbClippingPlaneNormal->setCurrentIndex(2);
    slideClippingPlaneOrigin->setValue(0);
    cmbClippingPlaneNormal_currentTextChanged(
        cmbClippingPlaneNormal->currentText());
    clippingPlaneControlsInitialized = true;
  } else {
    // Render the updated image when the clipping-control callbacks did not.
    vtkRender();
  }
}

void QVoxelRenderer::setClippingPlaneOriginSlider(QSlider *slider) {
  slideClippingPlaneOrigin = slider;
  clippingPlaneControlsInitialized = false;
  if (slideClippingPlaneOrigin == nullptr) {
    return;
  }
  slideClippingPlaneOrigin->setMinimum(0);
  slideClippingPlaneOrigin->setMaximum(100);
  slideClippingPlaneOrigin->setValue(0);
  connect(slideClippingPlaneOrigin, &QSlider::valueChanged, this,
          &QVoxelRenderer::slideClippingPlaneOrigin_valueChanged);
}

void QVoxelRenderer::setClippingPlaneNormalCombobox(QComboBox *combobox) {
  cmbClippingPlaneNormal = combobox;
  clippingPlaneControlsInitialized = false;
  if (cmbClippingPlaneNormal == nullptr) {
    return;
  }
  cmbClippingPlaneNormal->clear();
  cmbClippingPlaneNormal->addItems({"x", "y", "z"});
  cmbClippingPlaneNormal->setCurrentIndex(2); // default to z-normal
  connect(cmbClippingPlaneNormal, &QComboBox::currentTextChanged, this,
          &QVoxelRenderer::cmbClippingPlaneNormal_currentTextChanged);
}

void QVoxelRenderer::renderOnClippingPaneChange(SmeVtkWidget *smeVtkWidget) {
  smeVtkWidgetToRenderOnClippingPlaneChange = smeVtkWidget;
}

void QVoxelRenderer::mousePressEvent(QMouseEvent *ev) {
  if (ev->buttons() == Qt::NoButton || imageVolume.nVoxels() == 0) {
    return;
  }
  auto *interactor = renderWindow->GetInteractor();
  if (interactor == nullptr) {
    return;
  }
  const auto *pos = interactor->GetEventPosition();
  if (volumePicker->Pick(pos[0], pos[1], 0, renderer) == 0 ||
      volumePicker->GetVolume() != volume) {
    return;
  }
  std::array<int, 3> ijk{};
  volumePicker->GetPointIJK(ijk.data());
  const auto *dims = imageData->GetDimensions();
  for (int axis = 0; axis < 3; ++axis) {
    if (ijk[axis] < 0 || ijk[axis] >= dims[axis]) {
      return;
    }
  }
  const auto idx = 4 * imageData->ComputePointId(ijk.data());
  const auto col =
      qRgb(imageDataArray->GetValue(idx), imageDataArray->GetValue(idx + 1),
           imageDataArray->GetValue(idx + 2));
  // Undo the display's y reversal and map the duplicated slice of a 2D image
  // back to its only source slice. Validate signed indices before converting z.
  const sme::common::Voxel voxel{
      ijk[0], imageVolume.height() - 1 - ijk[1],
      std::min(static_cast<std::size_t>(ijk[2]), imageVolume.depth() - 1)};
  SPDLOG_DEBUG("voxel ({},{},{}) -> color {:x}", voxel.p.x(), voxel.p.y(),
               voxel.z, col);
  Q_EMIT mouseClicked(col, voxel);
}

void QVoxelRenderer::slideClippingPlaneOrigin_valueChanged(int value) {
  double fractionalValue =
      static_cast<double>(value) / slideClippingPlaneOrigin->maximum();
  double origin =
      minClippingPlaneOrigin +
      fractionalValue * (maxClippingPlaneOrigin - minClippingPlaneOrigin);
  clippingPlane->SetOrigin(origin, origin, origin);
  vtkRender();
  if (smeVtkWidgetToRenderOnClippingPlaneChange != nullptr) {
    smeVtkWidgetToRenderOnClippingPlaneChange->vtkRender();
  }
}

void QVoxelRenderer::cmbClippingPlaneNormal_currentTextChanged(
    const QString &text) {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  if (text == "x") {
    x = 1.0;
  } else if (text == "y") {
    y = 1.0;
  } else if (text == "z") {
    z = 1.0;
  }
  clippingPlane->SetNormal(x, y, z);
  std::array<int, 3> dim{};
  imageData->GetDimensions(dim.data());
  double dx;
  double dy;
  double dz;
  imageData->GetSpacing(dx, dy, dz);
  double ox;
  double oy;
  double oz;
  imageData->GetOrigin(ox, oy, oz);
  // Expand the clipping range slightly so "no clipping" doesn't slice geometry
  // that lies numerically on, or very close to, the image boundary.
  const double axisSpacing = x * dx + y * dy + z * dz;
  const double clipPadding = axisSpacing;
  minClippingPlaneOrigin = x * ox + y * oy + z * oz;
  maxClippingPlaneOrigin =
      x * (ox + dim[0] * dx) + y * (oy + dim[1] * dy) + z * (oz + dim[2] * dz);
  minClippingPlaneOrigin -= clipPadding;
  maxClippingPlaneOrigin += clipPadding;
  if (slideClippingPlaneOrigin != nullptr) {
    slideClippingPlaneOrigin_valueChanged(slideClippingPlaneOrigin->value());
  }
}
