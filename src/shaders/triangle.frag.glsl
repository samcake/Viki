#version 450

float  rn(float xx){         
    float x0=floor(xx);
    float x1=x0+1;
    float v0 = fract(sin (x0*.014686)*31718.927+x0);
    float v1 = fract(sin (x1*.014686)*31718.927+x1);          

    return (v0*(1-fract(xx))+v1*(fract(xx)))*2-1*sin(xx);
}

  
vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

float PI = 2.0 * acos(0.0);

struct Camera {
	vec3 _eye;

	vec3 _right;
	vec3 _up;
	vec3 _back;

	float _focal;
	float _sensorHeight;
	float _aspectRatio;
};

layout(binding = 0) uniform UniformBufferObject {
    Camera _cam;
} ubo;

vec3 forward = normalize(vec3(0.05, -0.05, -1.0));

vec3 worldUp = vec3(0.0, 1.0, 0.0);
vec3 right = normalize(cross(worldUp, -forward));
vec3 up = normalize(cross(-forward, right));

Camera cam = Camera(vec3(0.1, 0.50, -0.2), right, up, -forward, 0.036, 0.056, 16.0/9.0);

vec3 eyeFromClipSpace(Camera c, vec2 clipPos) {
	return vec3(clipPos.x*cam._aspectRatio*cam._sensorHeight * 0.5, clipPos.y * cam._sensorHeight * 0.5, -cam._focal);
}

vec3 worldFromEyeSpace(Camera c, vec3 eyePos) {
	return ubo._cam._right * eyePos.x + ubo._cam._up * eyePos.y + ubo._cam._back * eyePos.z + ubo._cam._eye;
}
vec3 worldFromEyeSpaceDir(Camera c, vec3 eyeDir) {
	return ubo._cam._right * eyeDir.x + ubo._cam._up * eyeDir.y + ubo._cam._back * eyeDir.z;
}

//float tileScale = 1.0/0.036;
float tileScale = 10.0;

float MAX_DEPTH = 10.0;
vec4 sceneFloor = vec4(0.0, 1.0, 0.0, -0.500);

float sdf_plane(vec3 pos, vec4 plane) {
	return dot(plane.xyz, pos) - plane.w;
}

int NUM_SPHERES = 6;
vec4 sceneSpheres[] = {
	vec4(1.0, 0.4, -2.0, 0.5),
	vec4(-1.0, 0.0, -2.0, 0.3),
	vec4(0.0, 0.1, -4.0, 0.3),
	vec4(0.0, -0.1, -3.0, 0.2),

	vec4(-1.0, 0.4, -3.5, 0.5),
	vec4(0.5, 0.8, -2.0, 0.4),
};

float sdf_sphere(vec3 pos, vec4 sphere) {
	return length(sphere.xyz - pos) - sphere.w;
}

vec2 sceneDistanceAt(vec3 pos) {
	vec2 touch = vec2(10000.0, -1);

	for (int p = 0; p < NUM_SPHERES; p++) {
		float dist = sdf_sphere(pos, sceneSpheres[p]);
		if (dist <= touch.x) {
			touch.x = max(0.0, dist);
			touch.y = float(p) + 1.0;
		}
	}

	{
		float dist = sdf_plane(pos, sceneFloor);
		if (dist <= touch.x) {
			touch.x = max(0.0, dist);
			touch.y = 0.0;
		}
	}

	return touch;
}

vec3 calcNormal(int p, vec3 pos ) {
	vec3 n = vec3(0, 1, 0);

	if (p > 0) {
		n = normalize(pos - sceneSpheres[p-1].xyz);
	}
	

/*
	{
		float dist = sdf_plane(pos, sceneFloor);
		if (dist <= touch.x) {
			touch.x = max(0.0, dist);
			touch.y = 0.0;
		}
	}
*/
	return n;
}

vec3 calcNormal(in vec3 pos ) {
#if 0
    vec2 e = vec2(1.0,-1.0)*0.5773*0.0005;
    return normalize( e.xyy*sceneDistanceAt( pos + e.xyy ).x + 
					  e.yyx*sceneDistanceAt( pos + e.yyx ).x + 
					  e.yxy*sceneDistanceAt( pos + e.yxy ).x + 
					  e.xxx*sceneDistanceAt( pos + e.xxx ).x );
#else
    // inspired by klems - a way to prevent the compiler from inlining map() 4 times
    vec3 n = vec3(0.0);
    for( int i=0; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*sceneDistanceAt(pos+0.0005*e).x;
    }
    return normalize(n);
#endif    
}

vec3 sceneSky(vec3 worldDir) {
	float vertical = dot(worldDir, vec3(0.0, 1.0, 0.0));
	float horizontal = dot(worldDir, normalize(vec3(0.5, 0.0, -1.0)));
	
	return vec3(horizontal * 0.7, horizontal * 0.7, 1.0 - vertical * vertical);
}


vec3 LIGHT_DIR = normalize(vec3(-1, 1, 0));

vec3 sceneColorAt(int p, vec3 worldPos, vec3 worldDir, vec4 rayFrag) {
	
	vec3 n = calcNormal(p, rayFrag.xyz);
	float ldotn = max(0, dot(n, LIGHT_DIR));

	//from the index of the prim found to the scenePrimMateria
//	return abs(n);
	//return hsv2rgb(abs(fract(rayFrag.xyz * tileScale)));
//	return (abs(fract(rayFrag.xyz * tileScale)));
//	return vec3((rn(float(p)) + float(p)) * 1.0 / 30.0);
//	return hsv2rgb(vec3(p / float(NUM_SPHERES + 1), 1.0, 1.0 - (rayFrag.w * rayFrag.w) / (MAX_DEPTH * MAX_DEPTH)));
	return hsv2rgb(vec3(p / float(NUM_SPHERES + 1), ldotn, ldotn));
}

int MAX_RAY_STEPS = 200;
float TOUCH_EPSILON = 0.001;
vec4 rayTraceScene(vec3 rayOri, vec3 rayDir, out int touched) {
	touched = -1;
	float rayLength = 0.0;
	vec3 rayPos = rayOri;
	int i = 0;
	for (; i < MAX_RAY_STEPS; i++) {
		vec2 stepDist = sceneDistanceAt(rayPos);
		if (stepDist.x < TOUCH_EPSILON) {
			touched = int(stepDist.y);
			break;
		}
		rayLength += stepDist.x;
//		rayPos = rayOri + rayDir * (rayLength);
		rayPos += rayDir * (stepDist.x);
	}
	return vec4(rayPos, rayLength);
}

layout(location = 0) out vec4 outputColor;
layout(location = 0) in vec4 inWorldPos;
layout(location = 1) in vec3 inWorldDir;
layout(location = 2) in vec2 inUV;

void main()
{	
	vec3 worldPos = inWorldPos.xyz;
	vec3 worldDir = normalize(inWorldDir);
	

	if (inUV.x > 0) {
		vec3 eyePos = eyeFromClipSpace( cam, inUV.xy);
	    worldPos = worldFromEyeSpace( cam, eyePos);

		worldDir = normalize(worldFromEyeSpaceDir(cam, eyePos));
	}	

	int touched = -1;
	vec4 rayFrag = rayTraceScene(worldPos, worldDir, touched);


	if (touched < 0) {
		outputColor = vec4(sceneSky(worldDir), 1.0);
		return;
	}
	//outputColor = vec4(hsv2rgb(vec3(i / float(MAX_RAY_STEPS), 1.0, 1.0)), 1.0);
	//outputColor = vec4(hsv2rgb(vec3(rayDepth / float(MAX_DEPTH), 1.0, 1.0)), 1.0);
	outputColor = vec4(sceneColorAt(touched, worldPos, worldDir, rayFrag), 1.0);

	// shadow!
	vec4 shadowFrag = rayTraceScene(rayFrag.xyz + LIGHT_DIR * 2.0 * TOUCH_EPSILON, LIGHT_DIR, touched);

	if (touched >= 0) {
		outputColor.xyz *= 0.5;
	//	outputColor = vec4(hsv2rgb(vec3(touched / float(NUM_SPHERES + 1), 1.0, 1.0)), 1.0);
	}
}
