//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_CAMERA_H
#define SPATIALMODELEDITOR_CAMERA_H

#include <math.h>

#include "ShaderProgram.hpp"
#include "Vector3.hpp"

#define PI 3.1415926535

class Camera
{
public:
  Camera(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ, GLfloat farZ);

  void SetPosition(GLfloat posX, GLfloat posY, GLfloat posZ);
  void SetPosition(Vector3 position);
  Vector3 GetPosition();

  void SetRotation(GLfloat rotX, GLfloat rotY, GLfloat rotZ);
  void SetRotation(Vector3 rotation);
  Vector3 GetRotation();

  Vector3 GetForwardVector();
  Vector3 GetUpVector();

  void UpdateProjection(ShaderProgram* program);
  void UpdateView(ShaderProgram* program);
private:
  GLfloat m_aspectRatio;
  GLfloat m_near;
  GLfloat m_far;
  GLfloat m_range;
  GLfloat m_FOV;

  GLfloat m_projectionMatrix[4][4];

  Vector3 m_viewPosition;
  Vector3 m_viewRotation;

  Vector3 m_viewForward;
  Vector3 m_viewUp;
};

#endif // SPATIALMODELEDITOR_CAMERA_H
