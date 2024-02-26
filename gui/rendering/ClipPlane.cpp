//
// Created by hcaramizaru on 2/25/24.
//

#include "ClipPlane.hpp"
#include <QVector3D>

std::stack<uint32_t> rendering::ClipPlane::available({0, 1, 2, 3, 4, 5, 6, 7});

rendering::ClipPlane::ClipPlane(GLfloat a, GLfloat b, GLfloat c, GLfloat d)
    : a(a), b(b), c(c), d(d) {

  assert(fabs(QVector3D(a, b, c).length() - 1.0f) < 0.001f);

  assert(available.empty() != 0);

  planeIndex = available.top();
  available.pop();
}

rendering::ClipPlane::~ClipPlane() { available.push(planeIndex); }

void rendering::ClipPlane::SetClipPlane(GLfloat a, GLfloat b, GLfloat c,
                                        GLfloat d) {
  this->a = a;
  this->b = b;
  this->c = c;
  this->d = d;
}

void rendering::ClipPlane::Enable() { status = true; }
void rendering::ClipPlane::Disable() { status = false; }
bool rendering::ClipPlane::getStatus() { return status; }

void rendering::ClipPlane::UpdateClipPlane(
    std::unique_ptr<rendering::ShaderProgram> &program) const {}
