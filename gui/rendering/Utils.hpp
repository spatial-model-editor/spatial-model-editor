//
// Created by acaramizaru on 6/30/23.
//

#ifndef SPATIALMODELEDITOR_UTILS_H
#define SPATIALMODELEDITOR_UTILS_H

#include <stdio.h>
#include <string>

#include "sme/logger.hpp"
#include <QOpenGLFunctions>
#include <QtOpenGL>

#ifdef QT_DEBUG
#define CheckOpenGLError(tag)                                                  \
  rendering::Utils::TraceGLError(tag, __FILE__, __LINE__)
#define GetCallstack(skip)                                                     \
  (std::string("Callstack:\n") + rendering::Utils::Backtrace(skip))
#else
#define CheckOpenGLError(tag)
#define GetCallstack(skip) std::string("Callstack:\n")
#endif

namespace rendering {

class Utils {

  static std::string PrintGLErrorDescription(unsigned int glErr);

public:
  static void TraceGLError(const std::string& tag,const std::string& file, int line);
  static std::string LoadFile(std::string &filename);

  static std::string Backtrace(int skip = 1);

  static void GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                                     GLenum severity, const GLchar *msg);
};

} // namespace rendering

#endif // SPATIALMODELEDITOR_UTILS_H
