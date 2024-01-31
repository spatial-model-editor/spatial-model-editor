//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_OBJECTINFO_H
#define SPATIALMODELEDITOR_OBJECTINFO_H

#include <QVector4D>
#include <QtOpenGL>
#include <vector>

namespace rendering {

struct Face {
  GLint vertexIndices[3] = {0, 0, 0};
  Face(int indx0, int indx1, int indx2) {
    vertexIndices[0] = indx0;
    vertexIndices[1] = indx1;
    vertexIndices[2] = indx2;
  }
};

struct ObjectInfo {
  std::vector<QVector4D> vertices;
  // A vector of submeshes. Each submesh contains a vector of indexes of all
  // faces.
  std::vector<std::vector<rendering::Face>> submeshes_indexes;
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_OBJECTINFO_H
