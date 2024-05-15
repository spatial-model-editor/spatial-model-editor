//
// Created by hcaramizaru on 2/25/24.
//

#include "ClippingPlane.hpp"
#include "ShaderProgram.hpp"
#include "qopenglmousetracker.hpp"
#include "sme/logger.hpp"
#include <QVector3D>

rendering::ClippingPlane::ClippingPlane(uint32_t planeIndex, bool active)
    : Node("ClippingPlane " + std::to_string(planeIndex),
           QVector3D(0.0f, 0.0f, 0.0f), QVector3D(0.0f, 0.0f, 0.0f),
           QVector3D(1.0f, 1.0f, 1.0f)),
      m_planeIndex(planeIndex), m_active(active) {}

std::set<std::shared_ptr<rendering::ClippingPlane>>
rendering::ClippingPlane::BuildClippingPlanes() {

  std::set<std::shared_ptr<ClippingPlane>> planes;

  for (uint32_t i = 0; i < MAX_NUMBER_PLANES; i++) {
    planes.insert(std::shared_ptr<ClippingPlane>(new ClippingPlane(i, true)));
  }

  return planes;
}

std::tuple<QVector3D, QVector3D>
rendering::ClippingPlane::fromAnalyticalToVectorial(float a, float b, float c,
                                                    float d) {

  GLfloat length = QVector3D(a, b, c).length();
  QVector3D normal(a / length, b / length, c / length);

  GLfloat p = d / length;
  QVector3D point(normal * p);

  return {point, normal};
}

std::tuple<float, float, float, float>
rendering::ClippingPlane::fromVectorialToAnalytical(QVector3D position,
                                                    QVector3D direction) {

  direction.normalize();

  return {direction.x(), direction.y(), direction.z(),
          -QVector3D::dotProduct(direction, position)};
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

std::tuple<QVector3D, QVector3D>
rendering::ClippingPlane::GetClipPlane() const {

  return fromAnalyticalToVectorial(m_a, m_b, m_c, m_d);
}

void rendering::ClippingPlane::TranslateAlongsideNormal(GLfloat value) {

  auto [point, normal] = fromAnalyticalToVectorial(m_a, m_b, m_c, m_d);

  point += normal * value;
  SetClipPlane(normal, point);
}

void rendering::ClippingPlane::Enable() { m_active = true; }
void rendering::ClippingPlane::Disable() { m_active = false; }
bool rendering::ClippingPlane::getStatus() const { return m_active; }

void rendering::ClippingPlane::UpdateClipPlane(
    std::unique_ptr<rendering::ShaderProgram> &program) const {

  if (m_active) {

    //    assert(m_dirty == false);

    auto [position, normal] = fromAnalyticalToVectorial(m_a, m_b, m_c, m_d);
    DecomposedTransform globalTransform = getGlobalTransform();
    QVector4D normalRotated =
        globalTransform.rotation * QVector4D(normal, 1.0f);
    QVector3D positionTranslated = globalTransform.position + position;

    auto [a, b, c, d] = fromVectorialToAnalytical(positionTranslated,
                                                  normalRotated.toVector3D());

    program->EnableClippingPlane(m_planeIndex);
    program->SetClippingPlane(a, b, c, d, m_planeIndex);
  } else {
    program->DisableClippingPlane(m_planeIndex);
  }
}
