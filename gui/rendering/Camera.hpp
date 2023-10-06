//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_CAMERA_H
#define SPATIALMODELEDITOR_CAMERA_H

#include <math.h>

#include "ShaderProgram.hpp"

namespace rendering {

constexpr float PI = 3.1415926535f;

class Camera {
public:
  Camera(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ,
         GLfloat farZ, GLfloat posX = 0.0f, GLfloat posY = 0.0f,
         GLfloat posZ = 0.0f, GLfloat rotX = 0.0f, GLfloat rotY = 0.0f,
         GLfloat rotZ = 0.0f);

  void SetFrustum(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ,
                  GLfloat farZ);

  void SetPosition(GLfloat posX, GLfloat posY, GLfloat posZ);
  void SetPosition(QVector3D position);
  QVector3D GetPosition() const;

  void SetRotation(GLfloat rotX, GLfloat rotY, GLfloat rotZ);
  void SetRotation(QVector3D rotation);
  QVector3D GetRotation() const;

  QVector3D GetForwardVector() const;
  QVector3D GetUpVector() const;

  void UpdateProjection(std::unique_ptr<rendering::ShaderProgram> &program) const;
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

  QVector3D m_viewPosition;
  QVector3D m_viewRotation;

  QVector3D m_viewForward;
  QVector3D m_viewUp;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_CAMERA_H
