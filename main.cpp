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

// �������
float radius = 2.0;
float rotateAngle = 0.0;
float upAngle = 45;

// ����ͶӰ����
float scale = 1.0;
float n = 0.1, f = 10;

TriMesh* mesh = new TriMesh();
float lightPos[3] = { 2.0, 2.0, 2.0 };

//////////////////////////////////////////////////////////////////////////
// ���

namespace Camera
{
	//����ͶӰ�任
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

	//͸��ͶӰ�任
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

	//�۲�任
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
// OpenGL ��ʼ��

void init()
{
	glClearColor(0.8f, 0.8f, 0.8f, 1);

	programID = InitShader("vshader_frag.glsl", "fshader_frag.glsl");

	// �Ӷ�����ɫ����ƬԪ��ɫ���л�ȡ������λ��
	vPositionID = glGetAttribLocation(programID, "vPosition");
	vNormalID = glGetAttribLocation(programID, "vNormal");
	viewMatrixID = glGetUniformLocation(programID, "viewMatrix");
	viewProjMatrixID = glGetUniformLocation(programID, "viewProjMatrix");
	modelMatrixID = glGetUniformLocation(programID, "modelMatrix");
	lightPosID = glGetUniformLocation(programID, "lightPos");
	shadowFlagID = glGetUniformLocation(programID, "isShadow");

	// ��ȡ�ⲿ��άģ��
	mesh->read_off("sphere.off");

	vector<vec3f> vs = mesh->v();
	vector<vec3i> fs = mesh->f();
	vector<vec3f> ns;

	//����ģ����ԭ����ϵ��Բ����ԭ�㣬�ڴ˽���ƽ�Ƶ�y=0ƽ������
	vec3f buttom = vs[0];
	for (int i = 0; i < vs.size(); ++i)
		if (vs[i].y < buttom.y) buttom = vs[i];

	for (int i = 0; i < vs.size(); ++i)
	{
		vs[i].y = vs[i].y - buttom.y;
		ns.push_back(normalize(vs[i] - vec3(0.0, -buttom.y, 0.0)));
	}

	// ����VAO
	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	// ����VBO�����󶨶�������
	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(vec3f), vs.data(), GL_STATIC_DRAW);

	// ����VBO�����󶨷���������
	glGenBuffers(1, &vertexNormalID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexNormalID);
	glBufferData(GL_ARRAY_BUFFER, ns.size() * sizeof(vec3f), ns.data(), GL_STATIC_DRAW);

	// ����VBO�����󶨶�������
	glGenBuffers(1, &vertexIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fs.size() * sizeof(vec3i), fs.data(), GL_STATIC_DRAW);

	// OpenGL��Ӧ״̬����
	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}

//////////////////////////////////////////////////////////////////////////
// ��Ⱦ

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);

	// ����۲�任����
	float eyex = radius*sin(rotateAngle*DegreesToRadians)*cos(upAngle*DegreesToRadians);
	float eyey = radius*sin(upAngle*DegreesToRadians);
	float eyez = radius*cos(rotateAngle*DegreesToRadians)*cos(upAngle*DegreesToRadians);
	vec4 eye(eyex,eyey,eyez, 1.0); // ��Դ����y-zƽ��ĶԳƷ���
	vec4 at(0, 0, 0, 1);   // ԭ��
	vec4 up(0, 1, 0, 0);      // Ĭ�Ϸ���
	Camera::viewMatrix = Camera::lookAt(eye, at, up);

	// ������Ӱ�任����
	float lx = lightPos[0];
	float ly = lightPos[1];
	float lz = lightPos[2];
	mat4 shadowProjMatrix
	(-ly, 0.0, 0.0, 0.0,
		lx, 0.0, lz, 1.0,
		0.0, 0.0, -ly, 0.0,
		0.0, 0.0, 0.0, -ly);
	Camera::modelMatrix = shadowProjMatrix;
	
	// ����ͶӰ�任����
	Camera::projMatrix= Camera::ortho(-scale, scale, -scale, scale, n, f);

	// ����Ӧ��������ɫ��
	mat4 viewProjMatrix = Camera::projMatrix * Camera::viewMatrix;
	glUniformMatrix4fv(modelMatrixID, 1, GL_TRUE, Camera::modelMatrix);
	glUniformMatrix4fv(viewMatrixID, 1, GL_TRUE, Camera::viewMatrix);
	glUniformMatrix4fv(viewProjMatrixID, 1, GL_TRUE, viewProjMatrix);

	// ����Դλ�ô��붥����ɫ��
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

	// ������Ӱ��־����Ϊ1����������Ӱ(����Դλ������ˮƽ�·��򲻻���)
	if (lightPos[1] > 0) {
		glUniform1i(shadowFlagID, 1);

		glDrawElements(
			GL_TRIANGLES,
			int(mesh->f().size() * 3),
			GL_UNSIGNED_INT,
			(void*)0
		);
	}

	// ������Ӱ��־����Ϊ0������modelMatrixΪ��λ���󣬲���������
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
// �������ô���

void reshape(GLsizei w, GLsizei h)
{
	glViewport(0, 0, w, h);
}

//////////////////////////////////////////////////////////////////////////
// �����Ӧ���������ƹ�Դλ��

void mouse( int x, int y)
{
		lightPos[0] = x - 250;
		lightPos[1] = 250 - y;
		glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////
// ������Ӧ����

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 033:	// ESC�� �� 'q' ���˳���Ϸ
		exit(EXIT_SUCCESS);
		break;
	case 'q':
		exit(EXIT_SUCCESS);
		break;
	case 'w'://w��s���ƹ۲�ĸ����Ƕ�
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
	glutCreateWindow("�ƿ���_2016150107_ʵ����");

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