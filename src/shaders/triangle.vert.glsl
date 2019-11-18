#version 450

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


vec3 forward = normalize(vec3(0.05, -0.05, -1.0));

vec3 worldUp = vec3(0.0, 1.0, 0.0);
vec3 right = normalize(cross(worldUp, -forward));
vec3 up = normalize(cross(-forward, right));

Camera cam = Camera(vec3(0.1, 0.50, -0.2), right, up, -forward, 0.036, 0.056, 16.0/9.0);

vec3 eyeFromClipSpace(Camera c, vec2 clipPos) {
	return vec3(clipPos.x*cam._aspectRatio*cam._sensorHeight * 0.5, clipPos.y * cam._sensorHeight * 0.5, -cam._focal);
}

vec3 worldFromEyeSpace(Camera c, vec3 eyePos) {
	return cam._right * eyePos.x + cam._up * eyePos.y + cam._back * eyePos.z + cam._eye;
}
vec3 worldFromEyeSpaceDir(Camera c, vec3 eyeDir) {
	return cam._right * eyeDir.x + cam._up * eyeDir.y + cam._back * eyeDir.z;
}
const vec3 vertices[] = 
{
	vec3(1.0, 1.0, 0),
	vec3(1.0, -1.0, 0),
	vec3(-1.0, 1.0, 0),
	vec3(-1.0, -1.0, 0),
} ;

layout(location = 0) out vec4 outWorldPos;
layout(location = 1) out vec3 outWorldDir;
layout(location = 2) out vec2 outUV;

void main()
{
	vec3 clipPos = vertices[gl_VertexIndex];
	outUV = clipPos.xy;
	gl_Position = vec4(clipPos.x, -clipPos.y, 0.0, 1.0);

	vec3 eyePos = eyeFromClipSpace( cam, clipPos.xy);
	vec3 worldPos = worldFromEyeSpace( cam, eyePos);

	outWorldPos = vec4(worldPos, 1.0);
	outWorldDir = worldFromEyeSpaceDir(cam, eyePos);
}

