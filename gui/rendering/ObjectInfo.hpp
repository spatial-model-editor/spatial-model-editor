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
  GLint vertexIndices[3] = {0,0,0};
  Face(int indx0,int indx1,int indx2) {
    vertexIndices[0] = indx0;
    vertexIndices[1] = indx1;
    vertexIndices[2] = indx2;
  }
};

struct ObjectInfo
{
  vector<Vector4> vertices;
  vector<Face> faces;
};


#endif // SPATIALMODELEDITOR_OBJECTINFO_H
