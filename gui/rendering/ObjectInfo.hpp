//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include <QVector4D>
#include <QtOpenGL>
#include <array>
#include <vector>

namespace rendering {

using Face = std::array<GLuint, 3>;

struct ObjectInfo {
  std::vector<QVector4D> vertices;
  std::vector<rendering::Face> faces;
};

} // namespace rendering
