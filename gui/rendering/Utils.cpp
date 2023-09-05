//
// Created by acaramizaru on 6/30/23.
//

#include "Utils.hpp"

#include <fstream>
#include <iostream>

std::string rendering::Utils::PrintGLErrorDescription(unsigned int glErr) {
  static std::string GLerrorDescription[] = {
      "GL_INVALID_ENUM",                 // 0x0500
      "GL_INVALID_VALUE",                // 0x0501
      "GL_INVALID_OPERATION",            // 0x0502
      "GL_STACK_OVERFLOW",               // 0x0503
      "GL_STACK_UNDERFLOW",              // 0x0504
      "GL_OUT_OF_MEMORY",                // 0x0505
      "GL_INVALID_FRAMEBUFFER_OPERATION" // 0x0506
  };

  return std::string("\n[OpenGL Error]\n") + std::string("\t[") +
         std::to_string(glErr) + std::string("] : ") +
         GLerrorDescription[glErr - GL_INVALID_ENUM] + std::string("\n");
}

void rendering::Utils::TraceGLError(std::string tag, std::string file,
                                    int line) {

  GLenum errLast = GL_NO_ERROR;

  while (true) {
    GLenum err = glGetError();

    if (err == GL_NO_ERROR) {
      break;
    }

    /*
     *  Normally, the error is reset by the call to glGetError(), but
     *  if glGetError() itself returns an error, we risk looping forever
     *  here, so we check that we get a different error than the last
     *  time.
     */

    if (err == errLast) {
      SPDLOG_ERROR("OpenGL error state couldn't be reset");
      break;
    }

    errLast = err;

    std::string errorMessage = PrintGLErrorDescription(err);
    std::string location = std::string("\t[File] : ") + file +
                           std::string("\t[Line] : ") + std::to_string(line) +
                           std::string("\t[OpenGL command] :") + tag +
                           std::string("\n");

    SPDLOG_ERROR(errorMessage + location);
  }
}

std::string rendering::Utils::LoadFile(std::string filename) {
  try {
    std::ifstream fileIn(filename);
    std::stringstream buffer;
    buffer << fileIn.rdbuf();
    return buffer.str();
  } catch (...) {
    SPDLOG_CRITICAL(std::string("Error when loading file:") + filename);
  }
}
