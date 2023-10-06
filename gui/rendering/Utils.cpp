//
// Created by acaramizaru on 6/30/23.
//

#include "Utils.hpp"

#include <fstream>
#include <iostream>
#include <qopengldebug.h>
#include <sstream>

#if defined(Q_OS_UNIX) && defined(QT_DEBUG)
#include <cxxabi.h>   // for __cxa_demangle
#include <dlfcn.h>    // for dladdr
#include <execinfo.h> // for backtrace
#endif

#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef QT_DEBUG
// Callback function for printing debug statements
void rendering::Utils::GLDebugMessageCallback(GLenum source, GLenum type,
                                              GLuint id, GLenum severity,
                                              const GLchar *msg) {
  std::string _source;
  std::string _type;
  std::string _severity;

  switch (source) {

  case QOpenGLDebugMessage::InvalidSource:
    _source = "InvalidSource";
    break;

  case QOpenGLDebugMessage::APISource:
    _source = "APISource";
    break;

  case QOpenGLDebugMessage::WindowSystemSource:
    _source = "WindowSystemSource";
    break;

  case QOpenGLDebugMessage::ShaderCompilerSource:
    _source = "ShaderCompilerSource";
    break;

  case QOpenGLDebugMessage::ThirdPartySource:
    _source = "ThirdPartySource";
    break;

  case QOpenGLDebugMessage::ApplicationSource:
    _source = "ApplicationSource";
    break;

  case QOpenGLDebugMessage::OtherSource:
    _source = "OtherSource";
    break;

    //  case QOpenGLDebugMessage::LastSource:
    //    _source = "LastSource";
    //    break;

  default:
    _source = "UNKNOWN";
    break;
  }

  switch (type) {

  case QOpenGLDebugMessage::InvalidType:
    _type = "InvalidType";
    break;

  case QOpenGLDebugMessage::ErrorType:
    _type = "ErrorType";
    break;

  case QOpenGLDebugMessage::DeprecatedBehaviorType:
    _type = "DeprecatedBehaviorType";
    break;

  case QOpenGLDebugMessage::UndefinedBehaviorType:
    _type = "UndefinedBehaviorType";
    break;

  case QOpenGLDebugMessage::PortabilityType:
    _type = "PortabilityType";
    break;

  case QOpenGLDebugMessage::PerformanceType:
    _type = "PerformanceType";
    break;

  case QOpenGLDebugMessage::OtherType:
    _type = "OtherType";
    break;

  case QOpenGLDebugMessage::MarkerType:
    _type = "MarkerType";
    break;

  case QOpenGLDebugMessage::GroupPushType:
    _type = "GroupPushType";
    break;

  case QOpenGLDebugMessage::GroupPopType:
    _type = "GroupPopType";
    break;

    //  case QOpenGLDebugMessage::LastType:
    //    _type = "LastType";
    //    break;

  default:
    _type = "UNKNOWN";
    break;
  }

  switch (severity) {

  case QOpenGLDebugMessage::InvalidSeverity:
    _severity = "InvalidSeverity";
    break;

  case QOpenGLDebugMessage::HighSeverity:
    _severity = "HighSeverity";
    break;

  case QOpenGLDebugMessage::MediumSeverity:
    _severity = "MediumSeverity";
    break;

  case QOpenGLDebugMessage::LowSeverity:
    _severity = "LowSeverity";
    break;

  case QOpenGLDebugMessage::NotificationSeverity:
    _severity = "NotificationSeverity";
    break;

    //  case QOpenGLDebugMessage::LastSeverity:
    //    _severity = "LastSeverity";
    //    break;

  default:
    _severity = "UNKNOWN";
    break;
  }

  SPDLOG_ERROR(std::string("id:") + std::to_string(id) + std::string(" type:") +
               _type + std::string(" severity:") + _severity +
               std::string(" source:") + _source + std::string(" message:") +
               msg);
}
#endif

#if defined(Q_OS_UNIX) && defined(QT_DEBUG)
// This function produces a stack backtrace with demangled function & method
// names. require -rdynamic linker option.
std::string rendering::Utils::Backtrace(const std::string &sectionName,
                                        int skip) {
  void *callstack[128];
  const int nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
  char buf[1024];
  int nFrames = backtrace(callstack, nMaxFrames);
  typedef char *ptrChar;
  std::shared_ptr<ptrChar[]> symbolsPtr = std::shared_ptr<ptrChar[]>(
      backtrace_symbols(callstack, nFrames), [](char **pi) { free(pi); });
  std::ostringstream trace_buf;
  for (int i = skip; i < nFrames; i++) {
    printf("%s\n", symbolsPtr[i]);

    Dl_info info;
    if (dladdr(callstack[i], &info) && info.dli_sname) {
      std::shared_ptr<char[]> demangledPtr;
      int status = -1;
      if (info.dli_sname[0] == '_')
        demangledPtr = std::shared_ptr<char[]>(
            abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status),
            [](char *pi) { free(pi); });
      snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n", i,
               int(2 + sizeof(void *) * 2), callstack[i],
               status == 0                 ? demangledPtr.get()
               : info.dli_sname == nullptr ? symbolsPtr[i]
                                           : info.dli_sname,
               (char *)callstack[i] - (char *)info.dli_saddr);
    } else {
      snprintf(buf, sizeof(buf), "%-3d %*p %s\n", i,
               int(2 + sizeof(void *) * 2), callstack[i], symbolsPtr[i]);
    }
    trace_buf << buf;
  }
  if (nFrames == nMaxFrames)
    trace_buf << "[truncated]\n";
  return sectionName + trace_buf.str();
}
#else
std::string Backtrace(int skip = 1) { return std::string(); }
#endif

#ifdef QT_DEBUG
void CheckOpenGLError(std::string tag) {
  rendering::Utils::TraceGLError(tag, __FILE__, __LINE__);
}
std::string GetCallstack(int skip) {
  return rendering::Utils::Backtrace("Callstack:\n", skip);
}
#else
void CheckOpenGLError(std::string tag) {}
std::string GetCallstack(int skip) { return std::string("Callstack:\n"); }
#endif

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

  return std::string("\n[OpenGL Error] :") + std::string(" [") +
         std::to_string(glErr) + std::string("] : ") +
         GLerrorDescription[glErr - GL_INVALID_ENUM] + std::string("\n");
}

void rendering::Utils::TraceGLError(const std::string &tag,
                                    const std::string &file, int line) {

  GLenum errLast = GL_NO_ERROR;

  while (true) {
    QOpenGLFunctions *glFuncs = QOpenGLContext::currentContext()->functions();
    GLenum err = glFuncs->glGetError();

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
    std::string location = std::string("[File]: ") + file +
                           std::string("\n[Line]: ") + std::to_string(line) +
                           std::string("\n[OpenGL command]: ") + tag +
                           std::string("\n");

    SPDLOG_ERROR(std::string("\n---->") + errorMessage + location +
                 GetCallstack(0) + std::string("<----\n"));
  }
}

std::string rendering::Utils::LoadFile(std::string &filename) {

  std::stringstream buffer;

  try {
    std::ifstream fileIn(filename);

    buffer << fileIn.rdbuf();
  } catch (...) {
    SPDLOG_CRITICAL(std::string("Error when loading file:") + filename);
  }
  return buffer.str();
}
