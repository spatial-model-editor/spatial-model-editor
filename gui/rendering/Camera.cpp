//
// Created by acaramizaru on 6/30/23.
//

#include "Camera.hpp"
#include <QtOpenGL>


Camera::Camera(GLfloat FOV, GLfloat width, GLfloat height, GLfloat nearZ, GLfloat farZ)
{
  m_aspectRatio = width / height;
  m_near = nearZ;
  m_far = farZ;

  m_range = nearZ - farZ;
  m_FOV = FOV;

  const GLfloat tanHalfFOV = tanf((m_FOV / 2.0) * PI / 180.0);

  m_projectionMatrix[0][0] = 1.0f / (tanHalfFOV * m_aspectRatio);
  m_projectionMatrix[0][1] = 0.0f;
  m_projectionMatrix[0][2] = 0.0f;
  m_projectionMatrix[0][3] = 0.0f;

  m_projectionMatrix[1][0] = 0.0f;
  m_projectionMatrix[1][1] = 1.0f / tanHalfFOV;
  m_projectionMatrix[1][2] = 0.0f;
  m_projectionMatrix[1][3] = 0.0f;

  m_projectionMatrix[2][0] = 0.0f;
  m_projectionMatrix[2][1] = 0.0f;
  m_projectionMatrix[2][2] = (-m_near - m_far) / m_range;
  m_projectionMatrix[2][3] = 2.0f * m_far * m_near / m_range;

  m_projectionMatrix[3][0] = 0.0f;
  m_projectionMatrix[3][1] = 0.0f;
  m_projectionMatrix[3][2] = 1.0f;
  m_projectionMatrix[3][3] = 0.0f;

  m_viewPosition = Vector3(0.0f, 0.0f, 0.0f);
  m_viewRotation = Vector3(0.0f, 0.0f, 0.0f);
}

void Camera::SetPosition(GLfloat posX, GLfloat posY, GLfloat posZ)
{
  m_viewPosition.x = posX;
  m_viewPosition.y = posY;
  m_viewPosition.z = posZ;
}

void Camera::SetPosition(Vector3 position)
{
  SetPosition(position.x, position.y, position.z);
}

Vector3 Camera::GetPosition()
{
  return m_viewPosition;
}

void Camera::SetRotation(GLfloat rotX, GLfloat rotY, GLfloat rotZ)
{
  m_viewRotation.x = rotX;
  m_viewRotation.y = rotY;
  m_viewRotation.z = rotZ;

  GLfloat _forwardX, _forwardY, _forwardZ;
  GLfloat forwardX, forwardY, forwardZ;

  GLfloat _upX, _upY, _upZ;
  GLfloat upX, upY, upZ;

  const GLfloat toRadian = PI / 180.0f;
  const GLfloat radRotX = rotX * toRadian;
  const GLfloat radRotY = rotY * toRadian;
  const GLfloat radRotZ = rotZ * toRadian;

  const GLfloat sinX = sinf(radRotX);
  const GLfloat cosX = cosf(radRotX);

  const GLfloat sinY = sinf(radRotY);
  const GLfloat cosY = cosf(radRotY);

  const GLfloat sinZ = sinf(radRotZ);
  const GLfloat cosZ = cosf(radRotZ);

  _forwardY = sinX;
  _forwardZ = cosX;

  _forwardX = _forwardZ * sinY;
  forwardZ = _forwardZ * cosY;

  forwardX = _forwardX * cosZ + _forwardY * sinZ;
  forwardY = _forwardX * sinZ - _forwardY * cosZ;

  m_viewForward.x = forwardX;
  m_viewForward.y = forwardY;
  m_viewForward.z = forwardZ;

  _upY = -cosX;
  _upZ = sinX;

  _upX = _upZ * sinY;
  upZ = _upZ * cosY;

  upX = _upX * cosZ + _upY * sinZ;
  upY = _upX * sinZ - _upY * cosZ;

  m_viewUp.x = upX;
  m_viewUp.y = upY;
  m_viewUp.z = upZ;
}

void Camera::SetRotation(Vector3 rotation)
{
  SetRotation(rotation.x, rotation.y, rotation.z);
}

Vector3 Camera::GetRotation()
{
  return m_viewRotation;
}

Vector3 Camera::GetForwardVector()
{
  return m_viewForward;
}

Vector3 Camera::GetUpVector()
{
  return m_viewUp;
}

void Camera::UpdateProjection(ShaderProgram* program)
{
  program->SetProjection(m_projectionMatrix[0]);
}

void Camera::UpdateView(ShaderProgram* program)
{
  program->SetViewPosition(m_viewPosition.x, m_viewPosition.y, m_viewPosition.z);
  program->SetViewRotation(m_viewRotation.x, m_viewRotation.y, m_viewRotation.z);
}
