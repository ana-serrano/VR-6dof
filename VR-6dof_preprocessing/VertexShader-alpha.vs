#version 150
uniform sampler2D fdepth;
uniform mat4 mWVP;
uniform mat4 mW;
uniform vec3 Center;
in      vec4 Pos;
in      vec4 Col;
in      vec2 Coord;
out     vec2 oCoord;
out     vec4 oCol;
out     vec3 worldPos;
out     vec3 ViewD;

void main()
{
float imgdepth = texture2D(fdepth, Coord).r;
//For google jump depth decoding - may be different for different depth encodings
imgdepth = 0.299999999999999999f / (imgdepth + 0.1f);
vec4 pp = vec4( Pos.x, Pos.y, Pos.z, 1 );
worldPos = (mW * (vec4( Pos.x * imgdepth, Pos.y * imgdepth, Pos.z * imgdepth, 1))).xyz;
gl_Position = (mWVP * (vec4( Pos.x , Pos.y , Pos.z , 1)));
ViewD = normalize(Center-worldPos);
oCoord   = Coord;
oCol.rgb = vec3(imgdepth);
oCol.a    = 1.0f;
}
