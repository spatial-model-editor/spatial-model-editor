//
// Created by acaramizaru on 6/30/23.
//

#include "ObjectLoader.hpp"

ObjectInfo ObjectLoader::Load(std::string filename)
{

  std::ifstream in(filename);

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

  std::tie(Positions, foundPosition) = mesh.property_map<SMesh::Vertex_index, Point>("Position");
  assert( !foundPosition );

  std::tie(Faces, foundFaces)  = mesh.property_map<SMesh::Face_index, std::vector<double>>("Faces");
  assert( !foundFaces );

  for(vertex_descriptor vd: vertices(mesh))
    std::cout << Positions[vd] << std::endl;

  for(face_descriptor fd: faces(mesh))
    std::cout << Faces[fd][0] << std::endl;

  return Obj;
}



