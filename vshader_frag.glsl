#version 330 core

in vec3 vPosition;
in vec3 vNormal;

uniform mat4 modelMatrix;
uniform mat4 viewProjMatrix;
uniform mat4 viewMatrix;

out vec3 N;
out vec3 V;

void main()
{
	//计算v2,v4以确保经变换矩阵作用后的向量的第四个分量为1
	vec4 v1 = modelMatrix*vec4(vPosition, 1.0);  
	vec4 v2 = vec4(v1.xyz / v1.w, 1.0);
	vec4 v3 = viewProjMatrix*v2;
	vec4 v4 = vec4(v3.xyz / v3.w, 1.0);
	gl_Position = v4;

	mat4 modelViewMatrix = viewMatrix * modelMatrix;
	// 将顶点变换到相机坐标系下
	vec4 vertPos_cameraspace = modelViewMatrix * vec4(vPosition, 1.0);
	
	// 计算并将向量V,N传入片元着色器
	V = vertPos_cameraspace.xyz / vertPos_cameraspace.w;
	N = (modelViewMatrix * vec4(vNormal, 0.0)).xyz;
}