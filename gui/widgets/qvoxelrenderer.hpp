#pragma once

#include "sme/image_stack.hpp"
#include "smevtkwidget.hpp"
#include <QComboBox>
#include <QSlider>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

class QVoxelRenderer : public SmeVtkWidget {
  Q_OBJECT
public:
  explicit QVoxelRenderer(QWidget *parent = nullptr);
  void setImage(const sme::common::ImageStack &img);
  void setClippingPlaneOriginSlider(QSlider *slider);
  void setClippingPlaneNormalCombobox(QComboBox *combobox);
  void renderOnClippingPaneChange(SmeVtkWidget *smeVtkWidget);
  inline vtkPlane *getClippingPlane() { return clippingPlane.Get(); }

signals:
  void mouseClicked(QRgb col, sme::common::Voxel voxel);

protected:
  void mousePressEvent(QMouseEvent *ev) override;

private:
  vtkNew<vtkVolume> volume;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  vtkNew<vtkImageData> imageData;
  vtkNew<vtkUnsignedCharArray> imageDataArray;
  vtkNew<vtkPlane> clippingPlane;
  QString lengthUnits{};
  SmeVtkWidget *smeVtkWidgetToRenderOnClippingPlaneChange{nullptr};
  double maxClippingPlaneOrigin{0.0};
  QSlider *slideClippingPlaneOrigin{nullptr};
  QComboBox *cmbClippingPlaneNormal{nullptr};
  void slideClippingPlaneOrigin_valueChanged(int value);
  void cmbClippingPlaneNormal_currentTextChanged(const QString &text);
};
