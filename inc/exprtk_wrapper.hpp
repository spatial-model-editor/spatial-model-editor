// Include exprtk library after
//  - disabling some features via #defines
//     - reduces build time and executable size
//     - disable undesired language features
//  - disabling GCC compiler warnings
//  - note: consider enabling `enhanced_features`
//     - would increase compilation time & executable size
//     - but potentially faster evaluation of expressions at runtime

#pragma once

#define exprtk_disable_comments
#define exprtk_disable_break_continue
#define exprtk_disable_sc_andor
#define exprtk_disable_return_statement
#define exprtk_disable_enhanced_features
#define exprtk_disable_string_capabilities
#define exprtk_disable_superscalar_unroll
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#define exprtk_disable_caseinsensitivity

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "exprtk/exprtk.hpp"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
