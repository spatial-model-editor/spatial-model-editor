//
// Created by acaramizaru on 6/30/23.
//

#include "Utils.hpp"

#include <iostream>
#include <fstream>

void Utils::TraceGLError(std::string tag)
{
  GLenum errorCode = glGetError();
  if (errorCode != GL_NO_ERROR)
  {
    std::cerr << tag << ":" << errorCode;
  }
}

std::string Utils::LoadFile(std::string filename)
{
  try
  {
    std::ifstream fileIn(filename);
    std::stringstream buffer;
    buffer << fileIn.rdbuf();
    return buffer.str();
  }
  catch(...)
  {
    std::cerr<<"Error when loading file:"<< filename;
  }
}
