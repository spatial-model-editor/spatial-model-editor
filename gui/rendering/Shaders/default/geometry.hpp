//
// Created by hcaramizaru on 3/7/24.
//

#ifndef SPATIALMODELEDITOR_GEOMETRY_HPP
#define SPATIALMODELEDITOR_GEOMETRY_HPP

namespace rendering {
namespace shader {
namespace default_ {

const char text_geometry[] =
    "#version 150 core\n"
    "\n"
    "layout(triangles) in;\n"
    "layout(triangle_strip, max_vertices = 3) out;\n"
    "\n"
    "//in vec4 out_vert_Color[];\n"
    "//out vec4 out_geo_Color;\n"
    "//barycentric coordinate inside the triangle\n"
    "out vec3 barycentric_coord;\n"
    "out vec3 distance;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    vec2 p0 = gl_in[0].gl_Position.xy;\n"
    "    p0 = p0 / gl_in[0].gl_Position.w;\n"
    "    vec2 p1 = gl_in[1].gl_Position.xy;\n"
    "    p1 = p1 / gl_in[1].gl_Position.w;\n"
    "    vec2 p2 = gl_in[2].gl_Position.xy;\n"
    "    p2 = p2 / gl_in[2].gl_Position.w;\n"
    "\n"
    "\n"
    "    gl_Position = gl_in[0].gl_Position;\n"
    "    vec2 v10 = (p1 - p0);   \n"
    "    vec2 v20 = (p2 - p0);   \n"
    "    // Compute 2D area of triangle.\n"
    "    float area0 = abs(v10.x*v20.y - v10.y*v20.x);\n"
    "    // Compute distance from vertex to line in 2D coords\n"
    "    float h0 = area0/length(v10-v20); \n"
    "    h0 *= gl_in[0].gl_Position.w;\n"
    "    barycentric_coord = vec3(1, 0.0, 0.0);\n"
    "    distance = vec3(h0, 0.0, 0.0);\n"
    "    EmitVertex();\n"
    "\n"
    "    gl_Position = gl_in[1].gl_Position;\n"
    "    vec2 v01 = (p0 - p1);   \n"
    "    vec2 v21 = (p2 - p1);   \n"
    "    // Compute 2D area of triangle.\n"
    "    float area1 = abs(v01.x*v21.y - v01.y*v21.x);\n"
    "    // Compute distance from vertex to line in 2D coords\n"
    "    float h1 = area1/length(v01-v21); \n"
    "    h1 *= gl_in[1].gl_Position.w;\n"
    "    barycentric_coord = vec3(0.0, 1, 0.0);\n"
    "    distance = vec3(0.0, h1, 0.0);\n"
    "    EmitVertex();\n"
    "\n"
    "    gl_Position = gl_in[2].gl_Position;\n"
    "    vec2 v02 = (p0 - p2);   \n"
    "    vec2 v12 = (p1 - p2);   \n"
    "    // Compute 2D area of triangle.\n"
    "    float area2 = abs(v02.x*v12.y - v02.y*v12.x);\n"
    "    // Compute distance from vertex to line in 2D coords\n"
    "    float h2 = area2/length(v02-v12);\n"
    "    h2 *= gl_in[2].gl_Position.w;\n"
    "    barycentric_coord = vec3(0.0, 0.0, 1);\n"
    "    distance = vec3(0.0, 0.0, h2);\n"
    "    EmitVertex();\n"
    "\n"
    "    EndPrimitive();\n"
    "}";

}
} // namespace shader
} // namespace rendering

#endif // SPATIALMODELEDITOR_GEOMETRY_HPP
