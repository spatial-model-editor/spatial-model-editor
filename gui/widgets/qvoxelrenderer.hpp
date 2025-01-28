#pragma once

#include "sme/image_stack.hpp"
#include "smevtkwidget.hpp"
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPiecewiseFunction.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

class QVoxelRenderer : public SmeVtkWidget {
  Q_OBJECT
public:
  explicit QVoxelRenderer(QWidget *parent = nullptr);
  void setImage(const sme::common::ImageStack &img);
  void setPhysicalSize(const sme::common::VolumeF &size, const QString &units);

signals:
  void mouseClicked(QRgb col, sme::common::Voxel voxel);

protected:
  void mousePressEvent(QMouseEvent *ev) override;

private:
  vtkNew<vtkVolume> volume;
  vtkNew<vtkVolumeProperty> volumeProperty;
  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  vtkNew<vtkPiecewiseFunction> opacityTransferFunction;
  vtkNew<vtkImageData> imageData;
  vtkNew<vtkUnsignedCharArray> imageDataArray;
  QString lengthUnits{};
};
