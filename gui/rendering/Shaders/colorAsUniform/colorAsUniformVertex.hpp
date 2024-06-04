//
// Created by hcaramizaru on 6/3/24.
//

#ifndef SPATIALMODELEDITOR_COLORASUNIFORMVERTEXCPU_H
#define SPATIALMODELEDITOR_COLORASUNIFORMVERTEXCPU_H

namespace rendering::shader::colorAsUniform {

const char text_vertex_color_as_uniform[] =
    "#version 150\n"
    "#extension GL_ARB_explicit_attrib_location : enable\n"
    "#define PI 3.14159265359\n"
    "#define NR_CLIP_PLANES 8\n"
    "layout(location=0) in vec4 in_Position;\n"
    "uniform vec4 in_Color;\n"
    "uniform mat4 model;\n"
    "uniform mat4 modelOffset;\n"
    "\n"
    "uniform mat4 projection;\n"
    "\n"
    "uniform mat4 view;\n"
    "uniform vec4 clipPlane[NR_CLIP_PLANES];\n"
    "uniform bool activeClipPlane[NR_CLIP_PLANES]=\n"
    "bool[NR_CLIP_PLANES](false, false, false, false, false, false, false, "
    "false);\n"
    "out gl_PerVertex {\n"
    "    vec4 gl_Position;\n"
    "    float gl_ClipDistance[8];\n"
    "};\n"
    "void main(void)\n"
    "{\n"
    "float rad_k = PI / 180.0;\n"
    "mat4 MV = model * view;\n"
    "mat4 MVP =  MV * projection;\n"
    "\n"
    "gl_Position = in_Position * modelOffset * MVP;\n"
    "for (int i=0; i <  activeClipPlane.length(); i++)\n"
    " if(activeClipPlane[i]) {\n"
    "  gl_ClipDistance[i] = dot( in_Position * modelOffset * model, "
    "clipPlane[i]);\n"
    "}\n"
    "}\n";

}

#endif // SPATIALMODELEDITOR_COLORASUNIFORMVERTEXCPU_H
