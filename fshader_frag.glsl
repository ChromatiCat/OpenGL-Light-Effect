#version 330 core

in vec3 N;
in vec3 V;

uniform vec3 lightPos;
uniform int isShadow;

uniform mat4 modelMatrix;
uniform mat4 viewProjMatrix;
uniform mat4 viewMatrix;

out vec4 fragmentColor;

void main()
{
	if(isShadow == 1){
		fragmentColor = vec4(0.0, 0.0, 0.1, 1.0);
	}
	else{
		mat4 modelViewMatrix = viewMatrix * modelMatrix;
		vec4 lightPos_cameraspace = modelViewMatrix * vec4(lightPos, 1.0);
		vec3 lightPos_cameraspace_3 = lightPos_cameraspace.xyz / lightPos_cameraspace.w;

		// 设置三维物体的材质属性
		vec3 ambiColor = vec3(0.2, 0.2, 0.2);
		vec3 diffColor = vec3(0.9, 0.9, 0.9);
		vec3 specColor = vec3(0.5, 0.5, 0.3);

		// 计算N，L，V，R四个向量并归一化
		vec3 N_norm = normalize(N);
		vec3 L_norm = normalize(lightPos_cameraspace_3 - V);
		vec3 V_norm = normalize(-V);
		vec3 R_norm = reflect(-L_norm, N_norm);

		// 计算漫反射系数和镜面反射系数
		float lambertian = clamp(dot(L_norm, N_norm), 0.0, 1.0);
		float specular = clamp(dot(R_norm, V_norm), 0.0, 1.0);
	
		float shininess = 10.0;
		fragmentColor = vec4(ambiColor + 
			 diffColor * lambertian +
			 specColor * pow(specular, shininess), 1.0);
	}
}