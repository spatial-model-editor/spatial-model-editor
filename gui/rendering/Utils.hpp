//
// Created by acaramizaru on 6/30/23.
//

#pragma once

#include <stdio.h>
#include <string>

#include "sme/logger.hpp"
#include <QOpenGLFunctions>
#include <QtOpenGL>

void CheckOpenGLError(std::string tag);
std::string GetCallstack(int skip);

namespace rendering {

class Utils {

  static std::string PrintGLErrorDescription(unsigned int glErr);

public:
  static void TraceGLError(const std::string &tag, const std::string &file,
                           int line);
  static std::string LoadFile(std::string &filename);

  static std::string Backtrace(const std::string &sectionName = "",
                               int skip = 1);

  static void GLDebugMessageCallback(GLenum source, GLenum type, GLuint id,
                                     GLenum severity, const GLchar *msg);
};

} // namespace rendering
