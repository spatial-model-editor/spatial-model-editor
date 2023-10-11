#version 140
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_attrib_location: enable
#define PI 3.14159265359
layout(location=0) in vec4 in_Position;
layout(location=1) in vec4 in_Color;
uniform vec3 position;
uniform vec3 rotation;
uniform vec3 scale;

uniform mat4 projection;

uniform vec3 viewPosition;
uniform vec3 viewRotation;
out vec4 ex_Color;

mat4 scaling( in float dx, in float dy, in float dz ) {
  return mat4(
    dx, 0.0, 0.0, 0.0,
    0.0, dy, 0.0, 0.0,
    0.0, 0.0, dz, 0.0,
    0.0, 0.0, 0.0, 1.0);
}

mat4 translation( in float dx, in float dy, in float dz ) {
  return mat4(
    1.0, 0.0, 0.0, dx,
    0.0, 1.0, 0.0, dy,
    0.0, 0.0, 1.0, dz,
    0.0, 0.0, 0.0, 1.0);
}

mat4 rotationX( in float angle ) {
  return mat4(
    1.0, 0.0, 0.0, 0.0,
    0.0, cos(angle), -sin(angle), 0.0,
    0.0, sin(angle), cos(angle), 0.0,
    0.0, 0.0, 0.0, 1.0);
}

mat4 rotationY( in float angle ) {
  return mat4(
    cos(angle), 0.0, sin(angle), 0.0,
    0.0, 1.0, 0.0, 0.0,
    -sin(angle), 0.0, cos(angle), 0.0,
    0.0, 0.0, 0.0, 1.0);
}

mat4 rotationZ( in float angle ) {
  return mat4(
    cos(angle), -sin(angle), 0.0, 0.0,
    sin(angle), cos(angle), 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
}

mat4 view( in vec3 viewPositionVec, in vec3 viewRotationVec )
{
  float rad_k = PI / 180.0;
  mat4 viewRotationMatrix = rotationZ(-viewRotationVec.z*rad_k) * rotationY(-viewRotationVec.y*rad_k) * rotationX(-viewRotationVec.x*rad_k);
  return translation(-viewPositionVec.x, -viewPositionVec.y, -viewPositionVec.z) * viewRotationMatrix;
}

void main(void)
{
  float rad_k = PI / 180.0;
  mat4 rotation = rotationX(rotation.x*rad_k) * rotationY(rotation.y*rad_k) * rotationZ(rotation.z*rad_k);
  mat4 model = rotation * scaling(scale.x, scale.y, scale.z) * translation(position.x, position.y, position.z);
  mat4 MVP = model * view(viewPosition, viewRotation) * projection;

  gl_Position = in_Position * MVP;
  ex_Color = in_Color;
}
