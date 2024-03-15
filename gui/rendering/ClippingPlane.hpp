//
// Created by hcaramizaru on 2/25/24.
//

#ifndef SPATIALMODELEDITOR_CLIPPLANE_H
#define SPATIALMODELEDITOR_CLIPPLANE_H

#include "ShaderProgram.hpp"
#include <stack>

class QOpenGLMouseTracker;

namespace rendering {

class ClippingPlane {

public:
  ClippingPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d,
                QOpenGLMouseTracker &OpenGLWidget, bool active = false);
  ClippingPlane(const QVector3D &normal, const QVector3D &point,
                QOpenGLMouseTracker &OpenGLWidget, bool active = false);

  ~ClippingPlane();
  ClippingPlane &operator=(const ClippingPlane &other) = delete;

  void SetClipPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d);
  void SetClipPlane(QVector3D normal, QVector3D &point);

  /**
   * @brief: Translate the plane alongside the normal
   *
   * @param value the value used for translation
   */

  void TranslateClipPlane(GLfloat value);
  void Enable();
  void Disable();
  bool getStatus() const;

  void
  UpdateClipPlane(std::unique_ptr<rendering::ShaderProgram> &program) const;

protected:
  GLfloat a, b, c, d;
  uint32_t planeIndex;
  bool status;
  static std::stack<uint32_t> available;

  QOpenGLMouseTracker &OpenGLWidget;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_CLIPPLANE_H
