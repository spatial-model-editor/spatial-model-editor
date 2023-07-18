//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_UTILS_H
#define SPATIALMODELEDITOR_UTILS_H

#include <stdio.h>
#include <string>
//#define MAX_FILE_SIZE 8192

#include <QtOpenGL>

using namespace std;

class Utils
{
public:
  static void TraceGLError(string tag);
  static std::string LoadFile(string filename);
};


#endif // SPATIALMODELEDITOR_UTILS_H
