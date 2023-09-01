//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_VECTOR4_H
#define SPATIALMODELEDITOR_VECTOR4_H

#include "IArray.hpp"
#include <QtOpenGL>

namespace rendering {

    class Vector4 : public IArray<GLfloat> {
    public:
      Vector4(void);
      Vector4(GLfloat x, GLfloat y, GLfloat z);
      Vector4(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

      Vector4 operator+(Vector4) const;
      Vector4 operator-(Vector4) const;
      Vector4 operator*(Vector4) const;
      Vector4 operator*(GLfloat) const;

      GLfloat x;
      GLfloat y;
      GLfloat z;
      GLfloat w;

      virtual std::vector<GLfloat> ToArray(void);
    };
}

#endif // SPATIALMODELEDITOR_VECTOR4_H
