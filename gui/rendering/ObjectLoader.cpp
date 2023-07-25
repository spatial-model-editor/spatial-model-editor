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

//  SMesh::Property_map<SMesh::Vertex_index, Point> Positions;
//  SMesh::Property_map<SMesh::Face_index, std::vector<double>> Faces;

//  bool foundPosition;
//  bool foundFaces;

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

  std::cerr << "vertices" << std::endl;
  for(SMesh::Vertex_index vd : mesh.vertices()){
    std::cerr << vd << std::endl;
  }

  std::cerr << "faces" << std::endl;
  for(SMesh::Face_index fd : mesh.faces()) {
    std::cerr << fd << std::endl;
  }

  return Obj;
}



