//
// Created by acaramizaru on 6/30/23.
//

#include "ObjectLoader.hpp"

ObjectInfo ObjectLoader::Load(std::string filename)
{

  std::ifstream in(filename);
  if(in.fail())
  {
    throw std::runtime_error("File not found!");
  }

  SMesh mesh;
  CGAL::IO::read_PLY(in, mesh);

  return Load(mesh);
}

ObjectInfo ObjectLoader::Load(SMesh mesh)
{
  ObjectInfo Obj;

  SMesh::Property_map<SMesh::Vertex_index, Point> Positions;
  SMesh::Property_map<SMesh::Face_index, std::vector<double>> Faces;

  bool foundPosition;
  bool foundFaces;

//  std::tie(Positions, foundPosition) = mesh.property_map<SMesh::Vertex_index, Point>("vertex");
//  //assert( foundPosition );
//
//  std::tie(Faces, foundFaces)  = mesh.property_map<SMesh::Face_index, std::vector<double>>("Faces");
//  //assert( foundFaces );

//  cout<<"vertex_descriptor:";
//  for(vertex_descriptor vd: vertices(mesh))
//    std::cout << Positions[vd] << std::endl;
//
//  cout<<"face_descriptor";
//  for(face_descriptor fd: faces(mesh))
//    std::cout << Faces[fd][0] << std::endl;

  std::vector<float> verts;
  std::vector<uint32_t> indices;

  //Get vertices
  for (SMesh::Vertex_index vi : mesh.vertices()) {
    Point pt = mesh.point(vi);
    Obj.vertices.push_back(Vector4(pt.x(), pt.y(), pt.z()));
  }

  //Get face indices
  for (SMesh::Face_index face_index : mesh.faces()) {
    CGAL::Vertex_around_face_circulator<SMesh> vcirc(mesh.halfedge(face_index), mesh), done(vcirc);
    std::vector<uint32_t> indices;
    do indices.push_back(*vcirc++); while (vcirc != done);
    if(indices.size()>3)
      throw std::runtime_error("The faces must be triangles!");
    else
      Obj.faces.push_back(Face(indices[0], indices[1], indices[2]));
  }

  return Obj;
}



