#include "include/Angel.h"
#include "include/TriMesh.h"

#pragma comment(lib, "glew32.lib")

#include <cstdlib>
#include <iostream>

using namespace std;

GLuint programID;
GLuint vertexArrayID;
GLuint vertexBufferID;
GLuint vertexNormalID;
GLuint vertexIndexBuffer;

GLuint vPositionID;
GLuint vNormalID;
GLuint viewMatrixID;
GLuint viewProjMatrixID;
GLuint modelMatrixID;
GLuint shadowFlagID;
GLuint lightPosID;

// 相机参数
float radius = 2.0;
float rotateAngle = 0.0;
float upAngle = 45;

// 正交投影参数
float scale = 1.0;
float n = 0.1, f = 10;

TriMesh* mesh = new TriMesh();
float lightPos[3] = { 2.0, 2.0, 2.0 };

//////////////////////////////////////////////////////////////////////////
// 相机

namespace Camera
{
	//正交投影变换
	mat4 ortho(const GLfloat left, const GLfloat right,
		const GLfloat bottom, const GLfloat top,
		const GLfloat zNear, const GLfloat zFar)
	{
		mat4 c;
		c[0][0] = 2 / (right - left);
		c[1][1] = 2 / (top - bottom);
		c[2][2] = -2 / (zFar - zNear);
		c[3][0] = (right + left) / (left - right);
		c[3][1] = (top + bottom) / (bottom - top);
		c[3][2] = (zFar + zNear) / (zNear - zFar);
		c[3][3] = 1;
		return c;
	}

	//透视投影变换
	mat4 perspective(const GLfloat fovy, const GLfloat aspect,
		const GLfloat zNear, const GLfloat zFar)
	{
		GLfloat top = tan(fovy*DegreesToRadians / 2) * zNear;
		GLfloat right = top * aspect;
		mat4 c;
		c[0][0] = zNear / right;
		c[1][1] = zNear / top;
		c[2][2] = -(zFar + zNear) / (zFar - zNear);
		c[2][3] = (-2.0*zFar*zNear) / (zFar - zNear);
		c[3][2] = -1.0;
		c[3][3] = 0.0;
		return c;
	}

	//观察变换
	mat4 lookAt(const vec4& eye, const vec4& at, const vec4& up)
	{
		vec4 n(normalize(eye - at));
		vec4 u(normalize(cross(up, n)), 0);
		vec4 v(normalize(cross(n, u)), 0);
		return mat4(u, v, n, vec4(0.0, 0.0, 0.0, 1.0))* Translate(-eye);
	}

	mat4 modelMatrix(1.0);
	mat4 viewMatrix(1.0);
	mat4 projMatrix(1.0);
}

//////////////////////////////////////////////////////////////////////////
// OpenGL 初始化

void init()
{
	glClearColor(0.8f, 0.8f, 0.8f, 1);

	programID = InitShader("vshader_frag.glsl", "fshader_frag.glsl");

	// 从顶点着色器和片元着色器中获取变量的位置
	vPositionID = glGetAttribLocation(programID, "vPosition");
	vNormalID = glGetAttribLocation(programID, "vNormal");
	viewMatrixID = glGetUniformLocation(programID, "viewMatrix");
	viewProjMatrixID = glGetUniformLocation(programID, "viewProjMatrix");
	modelMatrixID = glGetUniformLocation(programID, "modelMatrix");
	lightPosID = glGetUniformLocation(programID, "lightPos");
	shadowFlagID = glGetUniformLocation(programID, "isShadow");

	// 读取外部三维模型
	mesh->read_off("sphere.off");

	vector<vec3f> vs = mesh->v();
	vector<vec3i> fs = mesh->f();
	vector<vec3f> ns;

	//由于模型在原坐标系中圆心在原点，在此将其平移到y=0平面以上
	vec3f buttom = vs[0];
	for (int i = 0; i < vs.size(); ++i)
		if (vs[i].y < buttom.y) buttom = vs[i];

	for (int i = 0; i < vs.size(); ++i)
	{
		vs[i].y = vs[i].y - buttom.y;
		ns.push_back(normalize(vs[i] - vec3(0.0, -buttom.y, 0.0)));
	}

	// 生成VAO
	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	// 生成VBO，并绑定顶点数据
	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(vec3f), vs.data(), GL_STATIC_DRAW);

	// 生成VBO，并绑定法向量数据
	glGenBuffers(1, &vertexNormalID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexNormalID);
	glBufferData(GL_ARRAY_BUFFER, ns.size() * sizeof(vec3f), ns.data(), GL_STATIC_DRAW);

	// 生成VBO，并绑定顶点索引
	glGenBuffers(1, &vertexIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fs.size() * sizeof(vec3i), fs.data(), GL_STATIC_DRAW);

	// OpenGL相应状态设置
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}

//////////////////////////////////////////////////////////////////////////
// 渲染

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);

	// 计算观察变换矩阵
	float eyex = radius*sin(rotateAngle*DegreesToRadians)*cos(upAngle*DegreesToRadians);
	float eyey = radius*sin(upAngle*DegreesToRadians);
	float eyez = radius*cos(rotateAngle*DegreesToRadians)*cos(upAngle*DegreesToRadians);
	vec4 eye(eyex,eyey,eyez, 1.0); // 光源关于y-z平面的对称方向
	vec4 at(0, 0, 0, 1);   // 原点
	vec4 up(0, 1, 0, 0);      // 默认方向
	Camera::viewMatrix = Camera::lookAt(eye, at, up);

	// 计算阴影变换矩阵
	float lx = lightPos[0];
	float ly = lightPos[1];
	float lz = lightPos[2];
	mat4 shadowProjMatrix
	(-ly, 0.0, 0.0, 0.0,
		lx, 0.0, lz, 1.0,
		0.0, 0.0, -ly, 0.0,
		0.0, 0.0, 0.0, -ly);
	Camera::modelMatrix = shadowProjMatrix;
	
	// 计算投影变换矩阵
	Camera::projMatrix= Camera::ortho(-scale, scale, -scale, scale, n, f);

	// 将对应矩阵传入着色器
	mat4 viewProjMatrix = Camera::projMatrix * Camera::viewMatrix;
	glUniformMatrix4fv(modelMatrixID, 1, GL_TRUE, Camera::modelMatrix);
	glUniformMatrix4fv(viewMatrixID, 1, GL_TRUE, Camera::viewMatrix);
	glUniformMatrix4fv(viewProjMatrixID, 1, GL_TRUE, viewProjMatrix);

	// 将光源位置传入顶点着色器
	glUniform3fv(lightPosID, 1, lightPos);

	glEnableVertexAttribArray(vPositionID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glVertexAttribPointer(
		vPositionID,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	glEnableVertexAttribArray(vNormalID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexNormalID);
	glVertexAttribPointer(
		vNormalID,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);

	// 设置阴影标志变量为1，并绘制阴影(若光源位于球心水平下方则不绘制)
	if (lightPos[1] > 0) {
		glUniform1i(shadowFlagID, 1);

		glDrawElements(
			GL_TRIANGLES,
			int(mesh->f().size() * 3),
			GL_UNSIGNED_INT,
			(void*)0
		);
	}

	// 设置阴影标志变量为0，更改modelMatrix为单位矩阵，并绘制球体
	glUniform1i(shadowFlagID, 0);
	Camera::modelMatrix = mat4(1.0);
	glUniformMatrix4fv(modelMatrixID, 1, GL_TRUE, Camera::modelMatrix);

	glDrawElements(
		GL_TRIANGLES,
		int(mesh->f().size() * 3),
		GL_UNSIGNED_INT,
		(void*)0
	);

	glDisableVertexAttribArray(vPositionID);
	glUseProgram(0);

	glutSwapBuffers();
}

//////////////////////////////////////////////////////////////////////////
// 重新设置窗口

void reshape(GLsizei w, GLsizei h)
{
	glViewport(0, 0, w, h);
}

//////////////////////////////////////////////////////////////////////////
// 鼠标响应函数：控制光源位置

void mouse( int x, int y)
{
		lightPos[0] = x - 250;
		lightPos[1] = 250 - y;
		glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////
// 键盘响应函数

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 033:	// ESC键 和 'q' 键退出游戏
		exit(EXIT_SUCCESS);
		break;
	case 'q':
		exit(EXIT_SUCCESS);
		break;
	case 'w'://w和s控制观察的俯仰角度
		upAngle += 0.1;
		break;
	case 's':
		upAngle -= 0.1;
	}
	glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////

void idle(void)
{
	glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////

void clean()
{
	glDeleteBuffers(1, &vertexBufferID);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &vertexArrayID);

	if (mesh) {
		delete mesh;
		mesh = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowSize(500, 500);
	glutCreateWindow("黄俊粤_2016150107_实验三");

	glewInit();
	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMotionFunc(mouse);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	glutMainLoop();

	clean();

	return 0;
}