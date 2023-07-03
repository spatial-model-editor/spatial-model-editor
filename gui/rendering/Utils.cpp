//
// Created by acaramizaru on 6/30/23.
//

#include "Utils.hpp"

#include <stdio.h>

void Utils::TraceGLError(string tag)
{
  GLenum errorCode = glGetError();
  if (errorCode != GL_NO_ERROR)
  {
    fprintf(stderr, "%s: %d\n", tag.data(), errorCode);
  }
}
