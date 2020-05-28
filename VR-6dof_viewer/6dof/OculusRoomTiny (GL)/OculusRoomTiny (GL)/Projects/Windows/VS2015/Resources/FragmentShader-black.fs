#version 150
uniform sampler2D bgtext;
uniform	float 	  alpha;
uniform float     black;
in      vec4      oColor;
in      vec2      oTexCoord;
out     vec4      FragColor;

///////////////////////////////////////////////
// Main
/////////////////////////////////////////////////
void main()
{
	vec3 rgbcolor = texture2D(bgtext, oTexCoord).rgb;
	float aa = 1.0;	
	if ((rgbcolor.r+rgbcolor.g+rgbcolor.b)<0.3)
	{
		aa = 0.0;
	}
	if (black > 0.0){
		FragColor = vec4(0.0,0.0,0.0,1.0);}
	else{
		FragColor = vec4(rgbcolor,aa);}
	
	
}

