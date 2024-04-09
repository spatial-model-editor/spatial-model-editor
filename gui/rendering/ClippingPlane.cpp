//
// Created by hcaramizaru on 2/25/24.
//

#include "ClippingPlane.hpp"
#include "ShaderProgram.hpp"
#include "qopenglmousetracker.hpp"
#include "sme/logger.hpp"
#include <QVector3D>

rendering::ClippingPlane::ClippingPlane(uint32_t planeIndex, bool active)
    : m_planeIndex(planeIndex), m_active(active) {}

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

  m_a = a;
  m_b = b;
  m_c = c;
  m_d = d;
}

void rendering::ClippingPlane::SetClipPlane(QVector3D normal,
                                            const QVector3D &point) {

  normal.normalize();

  SetClipPlane(normal.x(), normal.y(), normal.z(),
               -QVector3D::dotProduct(normal, point));
}

void rendering::ClippingPlane::TranslateClipPlane(GLfloat value) {

  GLfloat length = QVector3D(m_a, m_b, m_c).length();
  QVector3D normal(m_a / length, m_b / length, m_c / length);

  GLfloat p = m_d / length;
  QVector3D point(normal * p);

  point += normal * value;
  SetClipPlane(normal, point);
}

void rendering::ClippingPlane::Enable() { m_active = true; }
void rendering::ClippingPlane::Disable() { m_active = false; }
bool rendering::ClippingPlane::getStatus() const { return m_active; }

void rendering::ClippingPlane::UpdateClipPlane(
    std::unique_ptr<rendering::ShaderProgram> &program) const {
  if (m_active) {
    program->EnableClippingPlane(m_planeIndex);
    program->SetClippingPlane(m_a, m_b, m_c, m_d, m_planeIndex);
  } else {
    program->DisableClippingPlane(m_planeIndex);
  }
}
