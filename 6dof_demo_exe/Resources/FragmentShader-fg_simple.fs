#version 150
uniform sampler2D fgtext;
uniform	float 	  desat;
uniform float colored;
in      vec4      oColor2;
in      vec2      oTexCoord2;
out     vec4      FragColor;
uniform sampler2D alphamask;

///////////////////////////////////////////////
// Main
/////////////////////////////////////////////////
void main()
{
	vec3 fgbcolor = texture2D(fgtext, oTexCoord2).rgb;
	float mask = texture2D(alphamask, oTexCoord2).r;		
   
   vec3 gray = vec3(dot(vec3(0.2126,0.7152,0.0722), fgbcolor));
   vec3 desaturated = vec3(mix(fgbcolor, gray, desat));
   desaturated = vec3(mix(fgbcolor, desaturated, 0.5));
   if (colored == 0)
	{				
		FragColor = vec4(desaturated,mask);
	}
	else
	{
			
		vec3 green = vec3(0.1,0.9,0.7);
		vec3 greenish = vec3(mix(desaturated, green, 0.5));		
		FragColor = vec4(greenish, mask);
	
	}
}
