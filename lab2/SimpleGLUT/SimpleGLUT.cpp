#include "stdafx.h"

// standard
#include <assert.h>
#include <math.h>
#include <iostream>

// glut
#define GLUT_DISABLE_ATEXIT_HACK
#include <gl/glut.h>

using namespace std;
#define pi 3.1415926

//================================
// global variables
//================================
// screen size
int g_screenWidth = 0;
int g_screenHeight = 0;

// frame index
int g_frameIndex = 0;

// angle for rotation
int g_angle = 0;
double g_xdistance = 0.0;
double g_ydistance = 0.0;

// initial value of interpolation and time
int index = 0; //index of position
int loopIndex = 0; //index of loop

//time counter
GLfloat time = 0.0f; //time counter

// counter for spacebar
//int counter = 0;

// x, y, z position of body
int numberPositions = 6; //number of positions
GLfloat bodyPositions[6][3] = {
	{ -8.0f, 0.0f, -15.0f },{ -5.0f, 0.0f, -10.0f },
	{ 0.0f, 0.0f, -7.0f },{ 5.0f, 0.0f, -10.0f },
	{ 8.0f, 0.0f, -15.0f },{ 0.0f, 0.0f, -20.0f }
};

// Catmull Rom Spline Matrix
GLfloat splineM[16] = { 
	-0.5f, 1.0f, -0.5f, 0.0f,
	1.5f, -2.5f, 0.0f, 1.0f,
	-1.5f, 2.0f, 0.5f, 0.0f,			
	0.5f, -0.5f, 0.0f, 0.0f 
};

// intermediate result for orientations
GLfloat M1[3] = { 0 };
GLfloat M2[3] = { 0 };

// interpolate matrix of body, left leg and right leg
GLfloat bodyM[16] = { 0 };
GLfloat leftM[16] = { 0 };
GLfloat rightM[16] = { 0 };

//================================
// Print String on Screen
//================================
void drawBitmapText(char *string, float x, float y, float z)
{
	char *c;
	glRasterPos3f(x, y, z);
	for (c = string; *c != '\0'; c++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
	}
}

//================================
// init
//================================
void init(void) {
	// init something before main loop...
}

//================================
// update
//================================
void update(void) {
	// do something before rendering...

	// rotation angle
	g_angle = (g_angle + 5) % 360;

	//g_xdistance = g_xdistance + 0.01;
	//g_ydistance = g_ydistance - 0.01;
}

// blending function
GLfloat blendFunc(GLfloat T[4], GLfloat M[16], GLfloat P[4]) {
	// TM[4] = T*M
	GLfloat TM[4] = { 0 };
	TM[0] = T[0] * M[0] + T[1] * M[1] + T[2] * M[2] + T[3] * M[3];	 //column 1
	TM[1] = T[0] * M[4] + T[1] * M[5] + T[2] * M[6] + T[3] * M[7];	 //column 2
	TM[2] = T[0] * M[8] + T[1] * M[9] + T[2] * M[10] + T[3] * M[11];  //column 3
	TM[3] = T[0] * M[12] + T[1] * M[13] + T[2] * M[14] + T[3] * M[15];//column 4

	GLfloat res = TM[0] * P[0] + TM[1] * P[1] + TM[2] * P[2] + TM[3] * P[3];

	return res;
}

// normalization
void norm(GLfloat m[3]) {
	GLfloat sumsqrt = m[0] * m[0] + m[1] * m[1] + m[2] * m[2];
	if (sumsqrt != 0) // avoid being divided by 0
	{
		GLfloat base = sqrt(sumsqrt);
		m[0] = m[0] / base;
		m[1] = m[1] / base;
		m[2] = m[2] / base;
	}
}

// multiple of two vectors
void vectorMult(GLfloat v1[3], GLfloat v2[3], GLfloat res[3]) {
	res[0] = v1[1] * v2[2] - v1[2] * v2[1];
	res[1] = v1[2] * v2[0] - v1[0] * v2[2];
	res[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

// 4 * 4 matrix multiple
void matrixMult(GLfloat M1[16], GLfloat M2[16], GLfloat resM[16]) {
	resM[16] = { 0 };
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			// compute resM[4i + j]
			int indexResM = i * 4 + j;
			resM[indexResM] = M1[0 * 4 + j] * M2[i * 4 + 0] + M1[1 * 4 + j] * M2[i * 4 + 1] + 
							  M1[2 * 4 + j] * M2[i * 4 + 2] + M1[3 * 4 + j] * M2[i * 4 + 3];
		}
	}
}

// interpolator by positions and spline matrix
void bodyInterpolater(GLfloat bodyPositions[6][3], GLfloat splineM[16]) {
	GLfloat tempPos[3] = { 0 };
	GLfloat tempAngel[3] = { 0 };
	GLfloat timeM1[4] = { time * time * time, time * time, time, 1 };
	GLfloat timeM2[4] = { 3 * time * time, 2 * time, 1, 0 };
	// 4 points a group
	for (int i = 0; i < 3; i++) {
		GLfloat fourPos[4] = { bodyPositions[(index + 0) % numberPositions][i],
			bodyPositions[(index + 1) % numberPositions][i],
			bodyPositions[(index + 2) % numberPositions][i],
			bodyPositions[(index + 3) % numberPositions][i] };
		tempPos[i] = blendFunc(timeM1, splineM, fourPos);
		tempAngel[i] = blendFunc(timeM2, splineM, fourPos);
	}

	norm(tempAngel);

	if (index == 0 && loopIndex == 0) // initial state
	{
		GLfloat initVector[3] = { 1, 0, 0 };
		norm(initVector);
		vectorMult(tempAngel, initVector, M1);
		norm(M1);
		vectorMult(M1, tempAngel, M2);
		norm(M2);
		loopIndex++;
	} else {
		vectorMult(tempAngel, M2, M1);
		norm(M1);
		vectorMult(M1, tempAngel, M2);
		norm(M2);
	}

	//interpolation of body matrix
	bodyM[0] = tempAngel[0]; //col1
	bodyM[1] = M1[0];
	bodyM[2] = M2[0];
	bodyM[3] = 0;
	bodyM[4] = tempAngel[1]; //col2
	bodyM[5] = M1[1];
	bodyM[6] = M2[1];
	bodyM[7] = 0;
	bodyM[8] = tempAngel[2]; //col3
	bodyM[9] = M1[2];
	bodyM[10] = M2[2];
	bodyM[11] = 0;
	bodyM[12] = tempPos[0]; //col4
	bodyM[13] = tempPos[1];
	bodyM[14] = tempPos[2];
	bodyM[15] = 1;
}

// generate and body interpolation
void bodyGenerator() {
	bodyInterpolater(bodyPositions, splineM);
	//glGetFloatv(GL_MODELVIEW_MATRIX, bodyM);
	glLoadMatrixf(bodyM);
	glutSolidSphere(1, 70, 50);
}

// generate and interpolate left leg: T1 * R1 * T2
void leftGenerator() {
	// T1, col order
	GLfloat T1[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 1.0f
	};

	// R1, by z-axis
	GLfloat tempAngel = (sin(4 * pi * time - pi / 2) * pi) / 4;
	GLfloat R[16] = {
		(GLfloat)cos(tempAngel), (GLfloat)sin(tempAngel), 0.0f, 0.0f,
		(GLfloat)-sin(tempAngel), (GLfloat)cos(tempAngel), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// T2
	GLfloat T2[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 1.0f,
	};

	// leftM = T2 * R * T1
	matrixMult(bodyM, R, leftM);
	matrixMult(leftM, T1, leftM);
	matrixMult(leftM, T2, leftM);

	// generate left leg
	// glGetFloatv(GL_MODELVIEW_MATRIX, leftM);
	glLoadMatrixf(leftM);
	glScalef(0.4f, 2.5f, 0.4f);
	glutSolidCube(1.0);
}

// generate and interpolate left leg: T1 * R1 * T2
void rightGenerator() {
	// T1, col order
	GLfloat T1[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 1.0f
	};

	// R1, by z-axis
	GLfloat tempAngel = -(sin(4 * pi * time - pi / 2) * pi) / 4;
	GLfloat R[16] = {
		(GLfloat)cos(tempAngel), (GLfloat)sin(tempAngel), 0.0f, 0.0f,
		(GLfloat)-sin(tempAngel), (GLfloat)cos(tempAngel), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// T2
	GLfloat T2[16] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, -0.5f, 1.0f,
	};

	// leftM = T2 * R * T1
	matrixMult(bodyM, R, leftM);
	matrixMult(leftM, T1, leftM);
	matrixMult(leftM, T2, leftM);

	// generate left leg
	// glGetFloatv(GL_MODELVIEW_MATRIX, leftM);
	glLoadMatrixf(leftM);
	glScalef(0.4f, 2.5f, 0.4f);
	glutSolidCube(1.0);
}

//================================
// render
//================================
void render(void) {
	// clear buffer
	glClearColor(0.5, 0.5, 0.5, 0.7);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// render state
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	// modelview matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -5.0f);

	//helper and 6 points
	drawBitmapText("Press Spacebar to Exit", -3.5, 1.8, 0.0);
	/*glBegin(GL_POINTS);
	glVertex3f(-5.0f, -3.0f, -10.0f);
	glVertex3f(-3.0f, -1.0f, -10.0f);
	glVertex3f(-1.0f, 1.0f, -10.0f);
	glVertex3f(1.0f, -1.0f, -10.0f);
	glVertex3f(3.0f, -2.0f, -10.0f);
	glVertex3f(5.0f, -3.0f, -10.0f);
	glEnd();*/

	// enable lighting
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	// light source attributes
	GLfloat LightAmbient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat LightDiffuse[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat LightSpecular[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	GLfloat LightPosition[] = { 5.0f, 5.0f, 5.0f, 1.0f };

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);

	// surface material attributes
	GLfloat material_Ka[] = { 0.11f, 0.06f, 0.11f, 1.0f };
	GLfloat material_Kd[] = { 0.43f, 0.47f, 0.54f, 1.0f };
	GLfloat material_Ks[] = { 0.33f, 0.33f, 0.52f, 1.0f };
	GLfloat material_Ke[] = { 0.1f , 0.0f , 0.1f , 1.0f };
	GLfloat material_Se = 10;

	glMaterialfv(GL_FRONT, GL_AMBIENT, material_Ka);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, material_Kd);
	glMaterialfv(GL_FRONT, GL_SPECULAR, material_Ks);
	glMaterialfv(GL_FRONT, GL_EMISSION, material_Ke);
	glMaterialf(GL_FRONT, GL_SHININESS, material_Se);

	// render objects
	bodyGenerator();
	leftGenerator();
	rightGenerator();

	// disable lighting
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	// swap back and front buffers
	glutSwapBuffers();
}

//================================
// keyboard input
//================================
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case ' ':
		exit(0);
		break;
	default:
		break;
	}
	glutPostRedisplay();
}

//================================
// reshape : update viewport and projection matrix when the window is resized
//================================
void reshape(int w, int h) {
	// screen size
	g_screenWidth = w;
	g_screenHeight = h;

	// viewport
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	// projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (GLfloat)w / (GLfloat)h, 1.0, 2000.0);
}

//================================
// timer : triggered every 16ms ( about 60 frames per second )
//================================
void timer(int value) {
	// render
	glutPostRedisplay();

	// increase frame index
	g_frameIndex++;

	// time++
	time += 0.005f;
	//index++
	if (time >= 1.0f) {
		time = 0.0f;
		if (index < numberPositions - 1) {
			index++;
		}
		else {
			index = 0;
		}
	}
	//update();


	// reset timer
	// 16 ms per frame ( about 60 frames per second )
	glutTimerFunc(16, timer, 0);
}

//================================
// main
//================================
int main(int argc, char** argv) {
	// create opengL window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(1600, 900);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("Lab2");

	// init
	init();

	// set callback functions
	glutDisplayFunc(render);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(16, timer, 0);

	// main loop
	glutMainLoop();

	return 0;
}