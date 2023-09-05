//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_VECTOR3_H
#define SPATIALMODELEDITOR_VECTOR3_H

#include "IArray.hpp"
#include <QtOpenGL>

namespace rendering {

class Vector3 : public IArray<GLfloat> {
public:
  Vector3(void);
  Vector3(GLfloat x, GLfloat y, GLfloat z);
  std::vector<GLfloat> ToArray() override;

  Vector3 operator+(Vector3) const;
  Vector3 operator-(Vector3) const;
  Vector3 operator*(Vector3) const;
  Vector3 operator*(GLfloat) const;

  GLfloat x;
  GLfloat y;
  GLfloat z;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_VECTOR3_H
