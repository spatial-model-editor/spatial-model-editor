//
// Created by hcaramizaru on 2/25/24.
//

#ifndef SPATIALMODELEDITOR_CLIPPLANE_H
#define SPATIALMODELEDITOR_CLIPPLANE_H

#include "ShaderProgram.hpp"
#include <set>
#include <vector>

class QOpenGLMouseTracker;

namespace rendering {

class ClippingPlane {

public:
  void
  UpdateClipPlane(std::unique_ptr<rendering::ShaderProgram> &program) const;

  /**
   * @brief Create a vector that contains the minimum number of planes possible.
   * ( MAX_NUMBER_PLANES )
   */
  static std::set<std::shared_ptr<rendering::ClippingPlane>>
  BuildClippingPlanes();

  void SetClipPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d);
  void SetClipPlane(QVector3D normal, const QVector3D &point);

  /**
   * @brief: Translate the plane alongside the normal
   *
   * @param value how much it gets in translation
   */
  void TranslateClipPlane(GLfloat value);

  /**
   * @brief: toggle between using and not the plane
   */
  void Enable();
  void Disable();

  /**
   *
   * @return if the plane influences the scene currently
   */
  bool getStatus() const;

protected:
  ClippingPlane(uint32_t planeIndex, bool active = false);

  GLfloat a;
  GLfloat b;
  GLfloat c;
  GLfloat d;
  uint32_t planeIndex;

  // active( true ) or disabled ( false )
  bool active;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_CLIPPLANE_H
