#version 150
uniform sampler2D fgdepth;
uniform sampler2D frontdepth;
uniform mat4 matWVP2;
in      vec4 Position2;
in      vec4 Color2;
in      vec2 TexCoord2;
out     vec2 oTexCoord2;
out     vec4 oColor2;
void main()
{
float bdepth = texture2D(fgdepth, TexCoord2).r;
float fdepth = texture2D(frontdepth,TexCoord2).r;
float imgdepth = min(bdepth,fdepth);
//Google Jump encoding
imgdepth = 0.299999999999999999f / (imgdepth + 0.002f);
vec4 pos = vec4( Position2.x * imgdepth, Position2.y * imgdepth, Position2.z * imgdepth, 1 );
gl_Position = (matWVP2 * pos);
oTexCoord2   = TexCoord2;
oColor2.rgb = vec3(imgdepth);
oColor2.a    = 1.0f;
}