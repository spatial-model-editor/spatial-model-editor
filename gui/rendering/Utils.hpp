//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_UTILS_H
#define SPATIALMODELEDITOR_UTILS_H

#include <stdio.h>
#include <string>

#include <QtOpenGL>
#include "sme/logger.hpp"

#ifdef QT_DEBUG
    #define CheckOpenGLError(tag)      rendering::Utils::TraceGLError(tag, __FILE__, __LINE__)
#else
    #define CheckOpenGLError(tag)
#endif

namespace rendering {

    class Utils {

      static std::string PrintGLErrorDescription(unsigned int glErr);

    public:
      static void TraceGLError(std::string tag, std::string file, int line);
      static std::string LoadFile(std::string filename);
    };

}

#endif // SPATIALMODELEDITOR_UTILS_H
