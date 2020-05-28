#version 150
uniform sampler2D depthbg;
uniform sampler2D depthfg;
uniform sampler2D depthfront;
uniform mat4 matWVP;
in      vec4 Position;
in      vec4 Color;
in      vec2 TexCoord;
out     vec2 oTexCoord;
out     vec4 oColor;
void main()
{
float dbbg = texture2D(depthbg, TexCoord).r;
float dbg = texture2D(depthfg, TexCoord).r;
float dfront = texture2D(depthfront, TexCoord).r;
float imgdepth = min(dbbg, dbg);
imgdepth = min(imgdepth, dfront);
//Google jump encoding
imgdepth = 0.299999999999999999f / (imgdepth + 0.001f);
vec4 pos = vec4( Position.x * imgdepth, Position.y * imgdepth, Position.z * imgdepth, 1 );
gl_Position = (matWVP * pos);
oTexCoord   = TexCoord;
oColor.rgb = vec3(imgdepth);
oColor.a    = 1.0f;
}