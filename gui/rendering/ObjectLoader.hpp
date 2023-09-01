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

namespace rendering {

    typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
    typedef Kernel::Point_3 Point;

    typedef CGAL::Surface_mesh<Point> SMesh;

    class ObjectLoader {
    public:
      static ObjectInfo Load(std::string filename);
      static ObjectInfo Load(SMesh);
      static SMesh LoadMesh(std::string filename);
    };

}

#endif // SPATIALMODELEDITOR_OBJECTLOADER_H
