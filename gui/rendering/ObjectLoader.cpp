//
// Created by acaramizaru on 6/30/23.
//

#include "ObjectLoader.hpp"

rendering::SMesh
rendering::ObjectLoader::LoadMesh(const std::string &filename) {
  std::ifstream in(filename);
  if (in.fail()) {
    throw std::runtime_error("File not found!");
  }

  SMesh mesh;
  CGAL::IO::read_PLY(in, mesh);

  in.close();

  return mesh;
}

rendering::ObjectInfo rendering::ObjectLoader::Load(std::string filename) {

  SMesh mesh = ObjectLoader::LoadMesh(filename);

  return Load(mesh);
}

rendering::ObjectInfo rendering::ObjectLoader::Load(const SMesh &mesh) {
  ObjectInfo Obj;

  // Get vertices
  for (SMesh::Vertex_index vi : mesh.vertices()) {
    Point pt = mesh.point(vi);
    Obj.vertices.push_back(QVector4D(static_cast<float>(pt.x()),
                                     static_cast<float>(pt.y()),
                                     static_cast<float>(pt.z()), 1.0f));
  }

  // Get face indices
  for (SMesh::Face_index face_index : mesh.faces()) {
    CGAL::Vertex_around_face_circulator<SMesh> vcirc(mesh.halfedge(face_index),
                                                     mesh),
        done(vcirc);
    std::vector<uint32_t> indices;
    do
      indices.push_back(*vcirc++);
    while (vcirc != done);
    if (indices.size() > 3)
      throw std::runtime_error("The faces must be triangles!");
    else
      Obj.faces.push_back(Face(indices[0], indices[1], indices[2]));
  }

  return Obj;
}
