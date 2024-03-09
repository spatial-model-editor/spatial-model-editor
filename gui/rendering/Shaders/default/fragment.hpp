//
// Created by acaramizaru on 8/30/23.
//

#pragma once

namespace rendering {
namespace shader {
namespace default_ {

const char text_fragment[] =
    "#version 150\n"
    "\n"
    "#extension GL_ARB_explicit_attrib_location : enable\n"
    "uniform vec4 in_Color;\n"
    "//in vec4 out_geo_Color;\n"
    "out vec4 frag_Color;\n"
    "in vec3 barycentric_coord;\n"
    "in vec3 distance;\n"
    "uniform float thickness;\n"
    "uniform vec3 background_color;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "   float barycentric_distance = min(barycentric_coord.x, "
    "min(barycentric_coord.y, barycentric_coord.z));\n"
    "   float dist = min(distance.x, min(distance.y, distance.z));\n"
    "   float derivative = fwidth(barycentric_distance);\n"
    "   float alpha = smoothstep(thickness, thickness+derivative, "
    "barycentric_distance); // calculate alpha\n"
    "   //float alpha = exp2(-2.0*barycentric_distance*barycentric_distance);\n"
    "   if(alpha > 0.5) { discard; }\n"
    "   ////if(dist > 0.1) { discard; }\n"
    "   frag_Color = vec4((1-alpha)*in_Color.xyz + (alpha)*background_color, "
    "1.0);\n"
    "   //frag_Color = in_Color;\n"
    "   //frag_Color = vec4(1.0, 0.0, 0.0, 0.5);\n"
    "}\n";
}
} // namespace shader
} // namespace rendering
