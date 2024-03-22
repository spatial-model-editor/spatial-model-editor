//
// Created by hcaramizaru on 2/25/24.
//

#include "ClippingPlane.hpp"
#include "ShaderProgram.hpp"
#include "qopenglmousetracker.hpp"
#include "sme/logger.hpp"
#include <QVector3D>

rendering::ClippingPlane::ClippingPlane(uint32_t planeIndex, bool active)
    : planeIndex(planeIndex), active(active) {}

std::set<std::shared_ptr<rendering::ClippingPlane>>
rendering::ClippingPlane::BuildClippingPlanes() {

  std::set<std::shared_ptr<ClippingPlane>> planes;

  for (uint32_t i = 0; i < MAX_NUMBER_PLANES; i++) {
    planes.insert(std::shared_ptr<ClippingPlane>(new ClippingPlane(i, true)));
  }

  return planes;
}

void rendering::ClippingPlane::SetClipPlane(GLfloat a, GLfloat b, GLfloat c,
                                            GLfloat d) {

  this->a = a;
  this->b = b;
  this->c = c;
  this->d = d;
}

void rendering::ClippingPlane::SetClipPlane(QVector3D normal,
                                            const QVector3D &point) {

  normal.normalize();

  SetClipPlane(normal.x(), normal.y(), normal.z(),
               -QVector3D::dotProduct(normal, point));
}

void rendering::ClippingPlane::TranslateClipPlane(GLfloat value) {

  GLfloat length = QVector3D(a, b, c).length();
  QVector3D normal(a / length, b / length, c / length);

  GLfloat p = d / length;
  QVector3D point(normal * p);

  point += normal * value;
  SetClipPlane(normal, point);
}

void rendering::ClippingPlane::Enable() { active = true; }
void rendering::ClippingPlane::Disable() { active = false; }
bool rendering::ClippingPlane::getStatus() const { return active; }

void rendering::ClippingPlane::UpdateClipPlane(
    std::unique_ptr<rendering::ShaderProgram> &program) const {
  if (active) {
    program->EnableClippingPlane(planeIndex);
    program->SetClippingPlane(a, b, c, d, planeIndex);
  } else {
    program->DisableClippingPlane(planeIndex);
  }
}
