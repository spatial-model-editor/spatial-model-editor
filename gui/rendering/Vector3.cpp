//
// Created by acaramizaru on 6/30/23.
//

#include "Vector3.hpp"

rendering::Vector3::Vector3() : Vector3(0.0f, 0.0f, 0.0f) {}

rendering::Vector3::Vector3(GLfloat x, GLfloat y, GLfloat z) {
  this->x = x;
  this->y = y;
  this->z = z;
}

std::vector<GLfloat> rendering::Vector3::ToArray() const{
  std::vector<GLfloat> v = {x, y, z};
  return v;
}

rendering::Vector3 rendering::Vector3::operator+(const Vector3 rhs) const {
  return Vector3(this->x + rhs.x, this->y + rhs.y, this->z + rhs.z);
}

rendering::Vector3 rendering::Vector3::operator-(const Vector3 rhs) const {
  return Vector3(this->x - rhs.x, this->y - rhs.y, this->z - rhs.z);
}

rendering::Vector3 rendering::Vector3::operator*(const Vector3 rhs) const {
  return Vector3(this->x * rhs.x, this->y * rhs.y, this->z * rhs.z);
}

rendering::Vector3 rendering::Vector3::operator*(const GLfloat rhs) const {
  return Vector3(this->x * rhs, this->y * rhs, this->z * rhs);
}
