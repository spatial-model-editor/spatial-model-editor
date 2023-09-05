//
// Created by acaramizaru on 6/30/23.
//

#include "Vector4.hpp"

rendering::Vector4::Vector4() : Vector4(0.0f, 0.0f, 0.0f) {}

rendering::Vector4::Vector4(GLfloat x, GLfloat y, GLfloat z)
    : Vector4(x, y, z, 1.0f) {}

rendering::Vector4::Vector4(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  this->x = x;
  this->y = y;
  this->z = z;
  this->w = w;
}

std::vector<GLfloat> rendering::Vector4::ToArray() {
  std::vector<GLfloat> v = {x, y, z, w};
  return v;
}

rendering::Vector4 rendering::Vector4::operator+(Vector4 rhs) const {
  return Vector4(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z,
                 this->w + rhs.w);
}

rendering::Vector4 rendering::Vector4::operator-(Vector4 rhs) const {
  return Vector4(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z,
                 this->w - rhs.w);
}

rendering::Vector4 rendering::Vector4::operator*(Vector4 rhs) const {
  return rendering::Vector4(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z,
                            this->w * rhs.w);
}

rendering::Vector4 rendering::Vector4::operator*(GLfloat rhs) const {
  return rendering::Vector4(this->x * rhs, this->y * rhs, this->z * rhs,
                            this->w * rhs);
}

bool rendering::Vector4::operator==(const Vector4 &rhs) const {
  return x == rhs.x && y == rhs.y && z == rhs.z;
}
