// Logger class
//  - first include ostream overloads
//  - then include spdlog and bundled fmt/ostr
//  - https://github.com/gabime/spdlog

#pragma once

#include "ostream_overloads.hpp"

// note: ostream_overloads must be included before spdlog
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"
