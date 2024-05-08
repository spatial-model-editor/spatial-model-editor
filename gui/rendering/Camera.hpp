//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include <math.h>

#include "Node.hpp"
#include "ShaderProgram.hpp"

namespace rendering {

constexpr float PI = 3.1415926535f;

class Camera : public Node {
public:
  Camera(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ,
         GLfloat farZ, GLfloat posX = 0.0f, GLfloat posY = 0.0f,
         GLfloat posZ = 0.0f, GLfloat rotX = 0.0f, GLfloat rotY = 0.0f,
         GLfloat rotZ = 0.0f);

  void SetFrustum(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ,
                  GLfloat farZ);

  void setRot(GLfloat rotX, GLfloat rotY, GLfloat rotZ);
  void setRot(QVector3D rotation);

  QVector3D GetForwardVector() const;
  QVector3D GetUpVector() const;

  void
  UpdateProjection(std::unique_ptr<rendering::ShaderProgram> &program) const;
  void UpdateView(std::unique_ptr<rendering::ShaderProgram> &program) const;

  GLfloat getNear() const;
  GLfloat getFar() const;
  GLfloat getFOV() const;

private:
  GLfloat m_aspectRatio;
  GLfloat m_near;
  GLfloat m_far;
  GLfloat m_range;
  GLfloat m_FOV;

  GLfloat m_projectionMatrix[4][4];

  QVector3D m_viewForward;
  QVector3D m_viewUp;
};

} // namespace rendering
