//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_OBJECTINFO_H
#define SPATIALMODELEDITOR_OBJECTINFO_H

#include "Vector4.hpp"
#include <vector>

using namespace std;

struct Face
{
  GLint vertexIndices[3];
};

struct ObjectInfo
{
  vector<Vector4> vertices;
  vector<Face> faces;
};


#endif // SPATIALMODELEDITOR_OBJECTINFO_H
