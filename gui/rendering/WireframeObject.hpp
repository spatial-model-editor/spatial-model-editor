//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_WIREFRAMEOBJECT_H
#define SPATIALMODELEDITOR_WIREFRAMEOBJECT_H

#include "ObjectInfo.hpp"
#include "ObjectLoader.hpp"
#include "ShaderProgram.hpp"
#include "Utils.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include <vector>

#include <QOpenGLWidget>
#include <QtOpenGL>

namespace rendering {

class WireframeObject : protected QOpenGLFunctions {

public:
  WireframeObject(rendering::ObjectInfo info, QColor color,
                  rendering::SMesh &mesh, QOpenGLWidget *Widget,
                  Vector3 position = rendering::Vector3(0.0f, 0.0f, 0.0f),
                  Vector3 rotation = rendering::Vector3(0.0f, 0.0f, 0.0f),
                  Vector3 scale = rendering::Vector3(1.0f, 1.0f, 1.0f));
  WireframeObject(const WireframeObject &cpy) = delete;
  ~WireframeObject();

  void Render(std::unique_ptr<rendering::ShaderProgram> &program,
              float lineWidth = 1);

  void SetRotation(GLfloat rotationX, GLfloat rotationY, GLfloat rotationZ);
  void SetRotation(rendering::Vector3 rotation);
  rendering::Vector3 GetRotation();

  void SetPosition(GLfloat positionX, GLfloat positionY, GLfloat positionZ);
  void SetPosition(rendering::Vector3 position);
  rendering::Vector3 GetPosition();

  void SetScale(GLfloat scaleX, GLfloat scaleY, GLfloat scaleZ);
  void SetScale(rendering::Vector3 scale);
  rendering::Vector3 GetScale();

  void SetColor(QColor color);
  rendering::SMesh GetMesh();

private:
  std::vector<rendering::Vector4> m_vertices;
  std::vector<GLuint> m_indices;
  QColor m_color;

  QOpenGLContext *m_openGLContext;

  std::vector<GLfloat> m_verticesBuffer;
  std::vector<uint8_t> m_colorBuffer;

  rendering::SMesh m_mesh;

  std::unique_ptr<QOpenGLVertexArrayObject> m_vao;
  GLuint m_vbo;
  GLuint m_colorBufferId;
  GLuint m_elementBufferId;

  rendering::Vector3 m_rotation;
  rendering::Vector3 m_position;
  rendering::Vector3 m_scale;

  void CreateVBO(void);
  void DestroyVBO(void);

  void UpdateVBOColor();
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_WIREFRAMEOBJECT_H
