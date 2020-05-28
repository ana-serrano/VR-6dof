#version 150
uniform sampler2D bgtext;
uniform	float 	  desat;
uniform float colored;
in      vec4      oColor;
in      vec2      oTexCoord;
out     vec4      FragColor;


///////////////////////////////////////////////
// Main
/////////////////////////////////////////////////
void main()
{
	vec3 rgbcolor = texture2D(bgtext, oTexCoord).rgb;
	vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), rgbcolor));
	vec3 desaturated = vec3(mix(rgbcolor, gray, desat));
	if (colored == 0)
	{				
		FragColor = vec4(desaturated, 1.0);	
	}
	else
	{
		vec3 purple = vec3(0.4,0.1,0.8);
		vec3 purpleish = vec3(mix(desaturated, purple, 0.5));		
		FragColor = vec4(purpleish, 1.0);
	
	}
}