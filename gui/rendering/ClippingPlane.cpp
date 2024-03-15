//
// Created by hcaramizaru on 2/25/24.
//

#include "ClippingPlane.hpp"
#include <QVector3D>

#include "qopenglmousetracker.hpp"

#include "ShaderProgram.hpp"

std::stack<uint32_t> generate(int32_t n) {

  assert(n > 0);
  std::stack<uint32_t> st;
  for (int32_t i = n - 1; i >= 0; i--) {
    st.push(i);
  }
  return st;
}

std::stack<uint32_t> rendering::ClippingPlane::available =
    generate(MAX_NUMBER_PLANES);

rendering::ClippingPlane::ClippingPlane(GLfloat a, GLfloat b, GLfloat c,
                                        GLfloat d,
                                        QOpenGLMouseTracker &OpenGLWidget,
                                        bool active)
    : a(a), b(b), c(c), d(d), status(active), OpenGLWidget(OpenGLWidget) {

  assert(!available.empty());

  this->OpenGLWidget.addClippingPlane(this);

  planeIndex = available.top();
  available.pop();
}

rendering::ClippingPlane::ClippingPlane(const QVector3D &normal,
                                        const QVector3D &point,
                                        QOpenGLMouseTracker &OpenGLWidget,
                                        bool active)
    : a(normal.x()), b(normal.y()), c(normal.z()),
      d(-QVector3D::dotProduct(normal, point)), status(active),
      OpenGLWidget(OpenGLWidget) {

  assert(!available.empty());

  this->OpenGLWidget.addClippingPlane(this);

  planeIndex = available.top();
  available.pop();
}

rendering::ClippingPlane::~ClippingPlane() {

  Disable();
  available.push(planeIndex);
  this->OpenGLWidget.deleteClippingPlane(this);
}

void rendering::ClippingPlane::SetClipPlane(GLfloat a, GLfloat b, GLfloat c,
                                            GLfloat d) {
  this->a = a;
  this->b = b;
  this->c = c;
  this->d = d;
}

void rendering::ClippingPlane::SetClipPlane(QVector3D normal,
                                            QVector3D &point) {

  normal.normalize();

  SetClipPlane(normal.x(), normal.y(), normal.z(),
               -QVector3D::dotProduct(normal, point));
}

void rendering::ClippingPlane::TranslateClipPlane(GLfloat value) {

  GLfloat length = QVector3D(a, b, c).length();
  QVector3D normal(a / length, b / length, c / length);

  GLfloat p = d / length;
  QVector3D point(normal * p);

  point += normal * value;
  SetClipPlane(normal, point);
}

void rendering::ClippingPlane::Enable() { status = true; }
void rendering::ClippingPlane::Disable() { status = false; }
bool rendering::ClippingPlane::getStatus() const { return status; }

void rendering::ClippingPlane::UpdateClipPlane(
    std::unique_ptr<rendering::ShaderProgram> &program) const {
  if (status) {
    program->EnableClippingPlane(planeIndex);
    program->SetClippingPlane(a, b, c, d, planeIndex);
  } else {
    program->DisableClippingPlane(planeIndex);
  }
}
