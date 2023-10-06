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

rendering::ObjectInfo
rendering::ObjectLoader::Load(const std::string &filename) {

  SMesh mesh = ObjectLoader::LoadMesh(filename);

  return Load(mesh);
}

rendering::ObjectInfo rendering::ObjectLoader::Load(const SMesh &mesh) {
  ObjectInfo Obj;

  // Get vertices
  Obj.vertices.reserve(mesh.vertices().size());
  for (SMesh::Vertex_index vi : mesh.vertices()) {
    Point pt = mesh.point(vi);
    Obj.vertices.emplace_back(QVector4D(static_cast<float>(pt.x()),
                                     static_cast<float>(pt.y()),
                                     static_cast<float>(pt.z()), 1.0f));
  }

  // Get face indices
  Obj.faces.reserve(mesh.faces().size());
  for (SMesh::Face_index face_index : mesh.faces()) {
    CGAL::Vertex_around_face_circulator<SMesh> vcirc(mesh.halfedge(face_index),
                                                     mesh),
        done(vcirc);
    Obj.faces.emplace_back(*vcirc++, *vcirc++, *vcirc++);
    if (vcirc != done)
      throw std::runtime_error("The faces must be triangles!");
  }

  return Obj;
}
