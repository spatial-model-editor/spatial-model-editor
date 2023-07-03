//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_OBJECTLOADER_H
#define SPATIALMODELEDITOR_OBJECTLOADER_H

#pragma once

#include <string>
#include "ObjectInfo.hpp"

#include "Vector4.hpp"

class ObjectLoader
{
public:
  ObjectInfo Load(const char* filename);
};


#endif // SPATIALMODELEDITOR_OBJECTLOADER_H
