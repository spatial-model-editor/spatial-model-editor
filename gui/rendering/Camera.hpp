//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_CAMERA_H
#define SPATIALMODELEDITOR_CAMERA_H

#include <math.h>

#include "ShaderProgram.hpp"
#include "Vector3.hpp"

namespace rendering {

#define PI 3.1415926535f

class Camera {
public:
  Camera(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ,
         GLfloat farZ, GLfloat posX = 0.0f, GLfloat posY = 0.0f,
         GLfloat posZ = 0.0f, GLfloat rotX = 0.0f, GLfloat rotY = 0.0f,
         GLfloat rotZ = 0.0f);

  void SetFrustum(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ,
                  GLfloat farZ);

  void SetPosition(GLfloat posX, GLfloat posY, GLfloat posZ);
  void SetPosition(rendering::Vector3 position);
  rendering::Vector3 GetPosition();

  void SetRotation(GLfloat rotX, GLfloat rotY, GLfloat rotZ);
  void SetRotation(rendering::Vector3 rotation);
  rendering::Vector3 GetRotation();

  rendering::Vector3 GetForwardVector();
  rendering::Vector3 GetUpVector();

  // Translate Camera - world axis
  //  void MoveForward(float elapsedTime);
  //  void MoveBackward(float elapsedTime);
  //  void MoveRight(float elapsedTime);
  //  void MoveLeft(float elapsedTime);
  //  void MoveUp(float elapsedTime);
  //  void MoveDown(float elapsedTime);
  //  void MoveInDirection(rendering::Vector3 direction, float elapsedTime);

  void UpdateProjection(std::unique_ptr<rendering::ShaderProgram>& program);
  void UpdateView(std::unique_ptr<rendering::ShaderProgram>& program);

  GLfloat getNear();
  GLfloat getFar();
  GLfloat getFOV();

private:
  GLfloat m_aspectRatio;
  GLfloat m_near;
  GLfloat m_far;
  GLfloat m_range;
  GLfloat m_FOV;

  GLfloat m_projectionMatrix[4][4];

  rendering::Vector3 m_viewPosition;
  rendering::Vector3 m_viewRotation;

  rendering::Vector3 m_viewForward;
  rendering::Vector3 m_viewUp;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_CAMERA_H
