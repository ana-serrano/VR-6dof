#version 150
uniform sampler2D depthbg;
uniform mat4 matWVP;
in      vec4 Position;
in      vec4 Color;
in      vec2 TexCoord;
out     vec2 oTexCoord;
out     vec4 oColor;
void main()
{
float imgdepth =  texture2D(depthbg, TexCoord).r;
//Google jump encoding
imgdepth = 0.299999999999999999f / (imgdepth + 0.001f);
vec4 pos = vec4( Position.x * imgdepth, Position.y * imgdepth, Position.z * imgdepth, 1 );
gl_Position = (matWVP * pos);
oTexCoord   = TexCoord;
oColor.rgb = vec3(imgdepth);
oColor.a    = 1.0f;
}