//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_OBJECTLOADER_H
#define SPATIALMODELEDITOR_OBJECTLOADER_H


#include <CGAL/Surface_mesh/Surface_mesh.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <iostream>
#include <fstream>

#include <string>
#include "ObjectInfo.hpp"

#include "Vector4.hpp"

typedef CGAL::Exact_predicates_inexact_constructions_kernel   Kernel;
typedef Kernel::Point_3                                       Point;

typedef CGAL::Surface_mesh<Point>                             SMesh;

typedef boost::graph_traits<SMesh>::vertex_descriptor      vertex_descriptor;
typedef boost::graph_traits<SMesh>::face_descriptor        face_descriptor;

class ObjectLoader
{
public:
  ObjectInfo Load(std::string filename);
  ObjectInfo Load(SMesh);
};


#endif // SPATIALMODELEDITOR_OBJECTLOADER_H
