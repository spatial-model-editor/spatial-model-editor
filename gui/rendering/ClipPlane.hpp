//
// Created by hcaramizaru on 2/25/24.
//

#ifndef SPATIALMODELEDITOR_CLIPPLANE_H
#define SPATIALMODELEDITOR_CLIPPLANE_H

#include "ShaderProgram.hpp"
#include "qopenglmousetracker.hpp"
#include <stack>

namespace rendering {

class ClipPlane {

public:
  friend class QOpenGLMouseTracker;

  ~ClipPlane();
  void SetClipPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d);
  void Enable();
  void Disable();
  bool getStatus();

protected:
  ClipPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d);

  void
  UpdateClipPlane(std::unique_ptr<rendering::ShaderProgram> &program) const;

  GLfloat a, b, c, d;
  uint32_t planeIndex;
  bool status;
  static std::stack<uint32_t> available;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_CLIPPLANE_H
