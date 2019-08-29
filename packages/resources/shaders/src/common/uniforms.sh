// lighting
uniform vec4 directional_lightdir[1];
uniform vec4 directional_color[1];
uniform vec4 directional_intensity[1];

uniform vec4 ambient_mode;        // ambient_mode.x 
							      //  = 0  ratio factor of main light,color use main light color 
								  //       ambient_mode.y = factor in this case 
								  //  = 1  classic ambient mode, use skycolor 
								  //  = 2  gradient, interpolate with { skycolor,midcolor,groundcolor } 
uniform vec4 ambient_skycolor;    // classic ambient color ,gradient skycolor 
uniform vec4 ambient_midcolor;
uniform vec4 ambient_groundcolor;

uniform vec4 u_eyepos;
uniform vec4 u_lightPos;

// lightmap - shadow

//postprocess
SAMPLER2D(s_mianview, 0);