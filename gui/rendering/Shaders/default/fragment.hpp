//
// Created by acaramizaru on 8/30/23.
//

#pragma once

namespace rendering {

const char text_fragment[] =
    "#version 140\n"
    "\n"
    "#extension GL_ARB_explicit_attrib_location : enable\n"
    "#extension GL_ARB_explicit_attrib_location: enable\n"
    "in vec4 ex_Color;\n"
    "out vec4 out_Color;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "out_Color = ex_Color;\n"
    "//out_Color = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";
}
