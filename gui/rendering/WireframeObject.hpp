//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_WIREFRAMEOBJECT_H
#define SPATIALMODELEDITOR_WIREFRAMEOBJECT_H

#include "ObjectInfo.hpp"
#include "ShaderProgram.hpp"
#include "Utils.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include <vector>

#include <QtOpenGL>

class WireframeObject: protected QOpenGLFunctions
{
public:
  WireframeObject(ObjectInfo info, Vector4 color);
  ~WireframeObject(void);

  void Render(ShaderProgram* program, float lineWidth=1);

  void SetRotation(GLfloat rotationX, GLfloat rotationY, GLfloat rotationZ);
  void SetRotation(Vector3 rotation);
  Vector3 GetRotation();

  void SetPosition(GLfloat positionX, GLfloat positionY, GLfloat positionZ);
  void SetPosition(Vector3 position);
  Vector3 GetPosition();

  void SetScale(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
  void SetScale(Vector3 scale);
  Vector3 GetScale();

  void SetScaleGeometry(float scale);
  float GetScaleGeometry();

private:
  vector<Vector4> m_vertices;
  vector<GLuint> m_indices;
  Vector4 m_color;

  vector<GLfloat> m_verticesBuffer;
  vector<GLfloat> m_colorBuffer;

//  GLuint m_vao;
  QOpenGLVertexArrayObject *m_vao;
  GLuint m_vbo;
  GLuint m_colorBufferId;
  GLuint m_elementBufferId;

  Vector3 m_rotation;
  Vector3 m_position;
  Vector3 m_scale;

  float m_scaleGeometry;

  void CreateVBO(void);
  void DestroyVBO(void);
};


#endif // SPATIALMODELEDITOR_WIREFRAMEOBJECT_H
