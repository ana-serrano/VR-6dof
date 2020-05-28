#version 150
uniform sampler2D fgdepth;
uniform mat4 matWVP2;
uniform mat4 matW2;
uniform vec3 SphCenter2;
uniform vec3 eyepos;
in      vec4 Position2;
in      vec4 Color2;
in      vec2 TexCoord2;
out     vec2 oTexCoord2;
out     vec4 oColor2;
out     vec3 ViewDir2;
out     vec3 world_Position2;
out     vec3 CurrViewDir;

void main()
{
float imgdepth = texture2D(fgdepth, TexCoord2).r;
//Google jump encoding
imgdepth = 0.299999999999999999f / (imgdepth + 0.003f);
vec4 pos = vec4( Position2.x * imgdepth, Position2.y * imgdepth, Position2.z * imgdepth, 1 );
gl_Position = (matWVP2 * pos);
world_Position2 = (matW2 * pos).xyz;
ViewDir2 = normalize(SphCenter2-world_Position2);
CurrViewDir = normalize(eyepos-world_Position2);
oTexCoord2   = TexCoord2;
oColor2.rgb = vec3(imgdepth);
oColor2.a    = 1.0f;
}