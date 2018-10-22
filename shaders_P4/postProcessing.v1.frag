#version 330 core
//Color de salida
out vec4 outColor;  

//Variables Variantes
in vec2 texCoord;

//Uniforms
uniform float focalDistance;
uniform float maxDistanceFactor;
uniform float zNear;
uniform float zFar;
uniform mat3 mask;

//Texturas
uniform sampler2D colorTex;
uniform sampler2D vertexTex; //Para muestrear la posición de cada vértice proyectado
uniform sampler2D depthTex;



//matrix de convolución
#define MASK_SIZE 9u
const vec2 texIdx[MASK_SIZE] = vec2[](
	vec2(-1.0,1.0), vec2(0.0,1.0), vec2(1.0,1.0),
	vec2(-1.0,0.0), vec2(0.0,0.0), vec2(1.0,1.0),
	vec2(-1.0,-1.0), vec2(0.0,-1.0), vec2(1.0,-1.0));

float maskara[MASK_SIZE] = float[](
	float (mask[0][0]), float (mask[0][1]), float (mask[0][2]),
	float (mask[1][0]), float (mask[1][1]), float (mask[1][2]),
	float (mask[2][0]), float (mask[2][1]), float (mask[2][2]));


void main(){

 vec2 ts = vec2(1.0) / vec2 (textureSize (colorTex,0));
 vec4 color = vec4 (0.0);

//float dof = abs(texture(vertexTex,texCoord).z -focalDistance)* maxDistanceFactor;
float dof = abs((-zNear*zFar/(texture(depthTex,texCoord).r*(zNear-zFar) + zFar)) - focalDistance) * maxDistanceFactor;
//float dof = abs(texture(depthTex,texCoord).r - focalDistance) * maxDistanceFactor;

dof = clamp (dof, 0.0, 1.0);
dof *= dof;

for (uint i = 0u; i < MASK_SIZE; i++){
	vec2 iidx = texCoord + ts * texIdx[i]*dof;
	color += texture(colorTex, iidx,0.0) * maskara[i];
}

outColor = color;
}
