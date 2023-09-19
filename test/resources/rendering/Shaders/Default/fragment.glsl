#version 140

#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_explicit_attrib_location: enable

in vec4 ex_Color;
out vec4 out_Color;

void main(void)
{
  out_Color = ex_Color;
  //out_Color = vec4(1.0, 0.0, 0.0, 1.0);
}
