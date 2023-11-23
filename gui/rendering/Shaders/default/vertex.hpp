//
// Created by acaramizaru on 8/30/23.
//

#pragma once

namespace rendering {

const char text_vertex[] =
    "#version 140\n"
    "#extension GL_ARB_explicit_attrib_location : enable\n"
    "#extension GL_ARB_explicit_attrib_location: enable\n"
    "#define PI 3.14159265359\n"
    "layout(location=0) in vec4 in_Position;\n"
    "layout(location=1) in vec4 in_Color;\n"
    "uniform vec3 position;\n"
    "uniform vec3 rotation;\n"
    "uniform vec3 scale;\n"
    "\n"
    "uniform mat4 projection;\n"
    "\n"
    "uniform vec3 viewPosition;\n"
    "uniform vec3 viewRotation;\n"
    "out vec4 ex_Color;\n"
    "\n"
    "mat4 scaling( in float dx, in float dy, in float dz ) {\n"
    "return mat4(\n"
    "dx, 0.0, 0.0, 0.0,\n"
    "0.0, dy, 0.0, 0.0,\n"
    "0.0, 0.0, dz, 0.0,\n"
    "0.0, 0.0, 0.0, 1.0);\n"
    "}\n"
    "\n"
    "mat4 translation( in float dx, in float dy, in float dz ) {\n"
    "return mat4(\n"
    "1.0, 0.0, 0.0, dx,\n"
    "0.0, 1.0, 0.0, dy,\n"
    "0.0, 0.0, 1.0, dz,\n"
    "0.0, 0.0, 0.0, 1.0);\n"
    "}\n"
    "\n"
    "mat4 rotationX( in float angle ) {\n"
    "return mat4(\n"
    "1.0, 0.0, 0.0, 0.0,\n"
    "0.0, cos(angle), -sin(angle), 0.0,\n"
    "0.0, sin(angle), cos(angle), 0.0,\n"
    "0.0, 0.0, 0.0, 1.0);\n"
    "}\n"
    "\n"
    "mat4 rotationY( in float angle ) {\n"
    "return mat4(\n"
    "cos(angle), 0.0, sin(angle), 0.0,\n"
    "0.0, 1.0, 0.0, 0.0,\n"
    "-sin(angle), 0.0, cos(angle), 0.0,\n"
    "0.0, 0.0, 0.0, 1.0);\n"
    "}\n"
    "\n"
    "mat4 rotationZ( in float angle ) {\n"
    "return mat4(\n"
    "cos(angle), -sin(angle), 0.0, 0.0,\n"
    "sin(angle), cos(angle), 0.0, 0.0,\n"
    "0.0, 0.0, 1.0, 0.0,\n"
    "0.0, 0.0, 0.0, 1.0);\n"
    "}\n"
    "\n"
    "mat4 view( in vec3 viewPositionVec, in vec3 viewRotationVec )\n"
    "{\n"
    "float rad_k = PI / 180.0;\n"
    "mat4 viewRotationMatrix = rotationZ(-viewRotationVec.z*rad_k) * "
    "rotationY(-viewRotationVec.y*rad_k) * "
    "rotationX(-viewRotationVec.x*rad_k);\n"
    "return translation(-viewPositionVec.x, -viewPositionVec.y, "
    "-viewPositionVec.z) * viewRotationMatrix;\n"
    "}\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "float rad_k = PI / 180.0;\n"
    "mat4 rotation = rotationX(rotation.x*rad_k) * rotationY(rotation.y*rad_k) "
    "* rotationZ(rotation.z*rad_k);\n"
    "mat4 model = rotation * scaling(scale.x, scale.y, scale.z) * "
    "translation(position.x, position.y, position.z);\n"
    "mat4 MVP = model * view(viewPosition, viewRotation) * projection;\n"
    "\n"
    "gl_Position = in_Position * MVP;\n"
    "ex_Color = in_Color;\n"
    "}\n";

}
