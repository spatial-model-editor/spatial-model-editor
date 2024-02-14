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

rendering::ObjectInfo Load(const sme::mesh::C3t3 c3t3) {

  // Iterate over the cells
  for (sme::mesh::C3t3::Cells_in_complex_iterator it =
           c3t3.cells_in_complex_begin();
       it != c3t3.cells_in_complex_end(); ++it) {
    // Access the vertices of the current cell
    const sme::mesh::C3t3::Cell_handle &cell = it;
    for (int i = 0; i < 4; ++i) { // Assuming tetrahedral cells with 4 vertices
      // Access the i-th vertex of the cell
      sme::mesh::C3t3::Vertex_handle vertex_handle = cell->vertex(i);
      // Access the index of the vertex
      auto vertex_index = vertex_handle->index();
      // Do something with the vertex index, such as printing it
      std::cout << "Vertex " << i << " index: " << vertex_index << std::endl;
    }
  }

  rendering::ObjectInfo rendering::ObjectLoader::Load(const SMesh &mesh) {

    //  assert(!"Not yet adapted!");

    ObjectInfo Obj;

    // Get vertices
    Obj.vertices.reserve(mesh.vertices().size());
    for (SMesh::Vertex_index vi : mesh.vertices()) {
      Point pt = mesh.point(vi);
      Obj.vertices.emplace_back(QVector4D(static_cast<float>(pt.x()),
                                          static_cast<float>(pt.y()),
                                          static_cast<float>(pt.z()), 1.0f));
    }

    //  // Get face indices
    //  Obj.faces.reserve(mesh.faces().size());
    //  for (SMesh::Face_index face_index : mesh.faces()) {
    //    CGAL::Vertex_around_face_circulator<SMesh> vcirc(
    //        mesh.halfedge(face_index),mesh
    //        ),
    //        done(vcirc);
    //    Obj.faces.emplace_back(*vcirc++, *vcirc++, *vcirc++);
    //    if (vcirc != done)
    //      throw std::runtime_error("The faces must be triangles!");
    //  }

    return Obj;
  }
