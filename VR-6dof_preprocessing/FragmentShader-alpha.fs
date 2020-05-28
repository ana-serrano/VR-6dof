#version 150
in      vec4      oCol;
in      vec2      oCoord;
in      vec3      worldPos;
in      vec3      ViewD;
out     vec4      FragColor;


///////////////////////////////////////////////
// Main
/////////////////////////////////////////////////
void main()
{
	float alpha = 0.5;	
	vec3 dFdxPos = dFdx(worldPos);
	vec3 dFdyPos = dFdy(worldPos);
	vec3 facenormal = normalize(cross(dFdxPos, dFdyPos));
	float ang = dot(ViewD, facenormal);	
	alpha = abs(ang);
	FragColor = vec4(alpha,alpha,alpha,1.0);
	
}