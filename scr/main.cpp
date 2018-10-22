#include "BOX.h"
#include "auxiliar.h"
#include "PLANE.h"

#include <windows.h>

#include <gl/glew.h>
#include <gl/gl.h>
#define SOLVE_FGLUT_WARNING
#include <gl/freeglut.h> 
#include <iostream>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>

#define RAND_SEED 31415926
#define SCREEN_SIZE 500,500

//////////////////////////////////////////////////////////////
// Datos que se almacenan en la memoria de la CPU
//////////////////////////////////////////////////////////////

//Matrices
glm::mat4	proj = glm::mat4(1.0f);
glm::mat4	view = glm::mat4(1.0f);
glm::mat4	model = glm::mat4(1.0f);

//Distintas matrices de convolución
float maskFactor = float(1.0 / 14.0);


const glm::mat3	emboss = glm::mat3(-2.0f, -1.0f, 0.0f,
									-1.0f, 1.0f, 1.0f,
									0.0f, 1.0f, 2.0f);

const glm::mat3	edgeDetect = glm::mat3(0.0f, 1.0f, 0.0f,
										1.0f, -4.0f, 1.0f,
										0.0f, 1.0f, 0.0f);

const glm::mat3	edgeEnhance = glm::mat3(0.0f, 0.0f, 0.0f,
										-1.0f, 1.0f, 0.0f,
										0.0f, 0.0f, 0.0f);

const glm::mat3	sharpen = glm::mat3(0.0f, -1.0f, 0.0f,
									-1.0f, 5.0f, -1.0f,
									0.0f, -1.0f, 0.0f);

//Mascara de convolución por defecto
glm::mat3	mask = emboss;


//////////////////////////////////////////////////////////////
// Variables que nos dan acceso a Objetos OpenGL
//////////////////////////////////////////////////////////////
float angle = 0.0f;

//Variables DOF
float fD = -25.0f;
float mDF = 1.0f/5.0f;

//Variable Motion Blur
float red = 0.8f;
float green = 0.8f;
float blue = 0.8f;
float alpha = 0.3f;

//Variables DOF y resizeFunc
float zNear = 1.0f;
float zFar = 50.0f;

//VAO
unsigned int vao;

//VBOs que forman parte del objeto
unsigned int posVBO;
unsigned int colorVBO;
unsigned int normalVBO;
unsigned int texCoordVBO;
unsigned int triangleIndexVBO;

unsigned int colorTexId;
unsigned int emiTexId;

unsigned int planeVAO;
unsigned int planeVertexVBO;


//Por definir
unsigned int vshader;
unsigned int fshader;
unsigned int program;

//Variables Uniform 
int uModelViewMat;		//Identificadores
int uModelViewProjMat;
int uNormalMat;
int uFocalDistance;
int uMaxDistanceFactor;
int uMask;
int uZNear;
int uZFar;


//Texturas Uniform
int uColorTex;
int uEmiTex;

//Atributos
int inPos;
int inColor;
int inNormal;
int inTexCoord;

//shader de postproceso
unsigned int postProccesVShader;
unsigned int postProccesFShader;
unsigned int postProccesProgram;
//configuracion de textura
unsigned int uVertexTexPP;
unsigned int vertexBuffTexId;


//Uniform
unsigned int uColorTexPP;
unsigned int uDepthTexPP;
//Atributos
int inPosPP;

//FRAME BUFFER Y PROFUNDIDAS Y TEXTURA
unsigned int fbo;
unsigned int colorBuffTexId;
unsigned int depthBuffTexId;


//////////////////////////////////////////////////////////////
// Funciones auxiliares
//////////////////////////////////////////////////////////////

//Declaración de CB
void renderFunc();
void resizeFunc(int width, int height);
void idleFunc();
void keyboardFunc(unsigned char key, int x, int y);
void mouseFunc(int button, int state, int x, int y);

void renderCube(); //Renderiza un cubo y se llama desde el renderizado para poder renderizar muchos cubos

//Funciones de inicialización y destrucción
void initContext(int argc, char** argv);
void initOGL();
void initShaderFw(const char *vname, const char *fname); //inicializa shaders de la practica anterior 
void initObj();
void initPlane();
void destroy();
void initFBO();
void resizeFBO(unsigned int w, unsigned int h);
void initShaderPP(const char *vname, const char *fname);



//Carga el shader indicado, devuele el ID del shader
//!Por implementar
GLuint loadShader(const char *fileName, GLenum type);

//Crea una textura, la configura, la sube a OpenGL, 
//y devuelve el identificador de la textura 
//!!Por implementar
unsigned int loadTex(const char *fileName);

//////////////////////////////////////////////////////////////
// Nuevas variables auxiliares
//////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////
// Nuevas funciones auxiliares
//////////////////////////////////////////////////////////////
//!!Por implementar


int main(int argc, char** argv)
{
	std::locale::global(std::locale("spanish"));// acentos ;)

	initContext(argc, argv);
	initOGL();
	initShaderFw("../shaders_P4/fwRendering.v1.vert", "../shaders_P4/fwRendering.v1.frag");
	initObj();


	//Inicializ de shaders de PP
	initShaderPP("../shaders_P4/postProcessing.v1.vert", "../shaders_P4/postProcessing.v1.frag");
	//despues de inic los shaders, se inic el plano
	initPlane();
	initFBO();
	resizeFBO(SCREEN_SIZE); //variable o constante definida con DEFINE que indica el tamaño de pantalla

	glutMainLoop();

	destroy();

	return 0;
}

//////////////////////////////////////////
// Funciones auxiliares 
void initContext(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	//glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(SCREEN_SIZE);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("Prácticas GLSL");

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cout << "Error: " << glewGetErrorString(err) << std::endl;
		exit(-1);
	}

	const GLubyte *oglVersion = glGetString(GL_VERSION);
	std::cout << "This system supports OpenGL Version: " << oglVersion << std::endl;

	glutReshapeFunc(resizeFunc);
	glutDisplayFunc(renderFunc);
	glutIdleFunc(idleFunc);
	glutKeyboardFunc(keyboardFunc);
	glutMouseFunc(mouseFunc);
}

void initOGL()
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);

	glFrontFace(GL_CCW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);

	proj = glm::perspective(glm::radians(60.0f), 1.0f, 1.0f, 50.0f);
	view = glm::mat4(1.0f);
	view[3].z = -25.0f; 
}


void destroy() //Liberacion de recursos
{
	glDetachShader(program, vshader);
	glDetachShader(program, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	glDeleteProgram(program);

	glDetachShader(postProccesProgram, postProccesVShader);
	glDetachShader(postProccesProgram, postProccesFShader);
	glDeleteShader(postProccesVShader);
	glDeleteShader(postProccesFShader);
	glDeleteProgram(postProccesProgram);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &planeVertexVBO);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &planeVAO);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &colorBuffTexId);
	glDeleteTextures(1, &depthBuffTexId);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	if (inPos != -1) glDeleteBuffers(1, &posVBO);
	if (inColor != -1) glDeleteBuffers(1, &colorVBO);
	if (inNormal != -1) glDeleteBuffers(1, &normalVBO);
	if (inTexCoord != -1) glDeleteBuffers(1, &texCoordVBO);
	glDeleteBuffers(1, &triangleIndexVBO);

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);

	//Borrado de texturas
	glBindTexture(GL_TEXTURE_2D, 0);
	glDeleteTextures(1, &colorTexId);
	glDeleteTextures(1, &emiTexId);
	glDeleteTextures(1, &vertexBuffTexId);
}

void initShaderFw(const char *vname, const char *fname)
{
	vshader = loadShader(vname, GL_VERTEX_SHADER);
	fshader = loadShader(fname, GL_FRAGMENT_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vshader);
	glAttachShader(program, fshader);

	glBindAttribLocation(program, 0, "inPos");
	glBindAttribLocation(program, 1, "inColor");
	glBindAttribLocation(program, 2, "inNormal");
	glBindAttribLocation(program, 3, "inTexCoord");


	glLinkProgram(program);

	int linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);

		char *logString = new char[logLen];
		glGetProgramInfoLog(program, logLen, NULL, logString);
		std::cout << "Error: " << logString << std::endl;
		delete logString;

		glDeleteProgram(program);
		program = 0;
		exit(-1);
	}


	//Identificadores variabes Uniform
	uNormalMat = glGetUniformLocation(program, "normal");
	uModelViewMat = glGetUniformLocation(program, "modelView");
	uModelViewProjMat = glGetUniformLocation(program, "modelViewProj");

	uColorTex = glGetUniformLocation(program, "colorTex");
	uEmiTex = glGetUniformLocation(program, "emiTex");

	inPos = glGetAttribLocation(program, "inPos");
	inColor = glGetAttribLocation(program, "inColor");
	inNormal = glGetAttribLocation(program, "inNormal");
	inTexCoord = glGetAttribLocation(program, "inTexCoord");
}

void initObj()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	if (inPos != -1)
	{
		glGenBuffers(1, &posVBO);
		glBindBuffer(GL_ARRAY_BUFFER, posVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex*sizeof(float) * 3,
			cubeVertexPos, GL_STATIC_DRAW);
		glVertexAttribPointer(inPos, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inPos);
	}

	if (inColor != -1)
	{
		glGenBuffers(1, &colorVBO);
		glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex*sizeof(float) * 3,
			cubeVertexColor, GL_STATIC_DRAW);
		glVertexAttribPointer(inColor, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inColor);
	}

	if (inNormal != -1)
	{
		glGenBuffers(1, &normalVBO);
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex*sizeof(float) * 3,
			cubeVertexNormal, GL_STATIC_DRAW);
		glVertexAttribPointer(inNormal, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inNormal);
	}


	if (inTexCoord != -1)
	{
		glGenBuffers(1, &texCoordVBO);
		glBindBuffer(GL_ARRAY_BUFFER, texCoordVBO);
		glBufferData(GL_ARRAY_BUFFER, cubeNVertex*sizeof(float) * 2,
			cubeVertexTexCoord, GL_STATIC_DRAW);
		glVertexAttribPointer(inTexCoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(inTexCoord);
	}

	glGenBuffers(1, &triangleIndexVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triangleIndexVBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		cubeNTriangleIndex*sizeof(unsigned int) * 3, cubeTriangleIndex,
		GL_STATIC_DRAW);

	model = glm::mat4(1.0f);

	colorTexId = loadTex("../img/color2.png");
	emiTexId = loadTex("../img/emissive.png");
}

GLuint loadShader(const char *fileName, GLenum type)
{
	unsigned int fileLen;
	char *source = loadStringFromFile(fileName, fileLen);

	//////////////////////////////////////////////
	//Creación y compilación del Shader
	GLuint shader;
	shader = glCreateShader(type);
	glShaderSource(shader, 1,
		(const GLchar **)&source, (const GLint *)&fileLen);
	glCompileShader(shader);
	delete source;

	//Comprobamos que se compilo bien
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

		char *logString = new char[logLen];
		glGetShaderInfoLog(shader, logLen, NULL, logString);
		std::cout << "Error: " << logString << std::endl;
		delete logString;

		glDeleteShader(shader);
		exit(-1);
	}

	return shader;
}

unsigned int loadTex(const char *fileName)
{
	unsigned char *map;
	unsigned int w, h;
	map = loadTexture(fileName, w, h);

	if (!map)
	{
		std::cout << "Error cargando el fichero: "
			<< fileName << std::endl;
		exit(-1);
	}

	unsigned int texId;
	glGenTextures(1, &texId);
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, (GLvoid*)map);
	delete[] map;
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	return texId;
}

void renderFunc()
{
	

	

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  //borra el buffer activo, no el defecto

	/**/
	glUseProgram(program);


	

	//Texturas
	if (uColorTex != -1)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorTexId);
		glUniform1i(uColorTex, 0);
	}

	if (uEmiTex != -1)
	{
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, emiTexId);
		glUniform1i(uEmiTex, 1);
	}


	model = glm::mat4(2.0f);
	model[3].w = 1.0f;
	model = glm::rotate(model, angle, glm::vec3(1.0f, 1.0f, 0.0f));
	renderCube();
	

	std::srand(RAND_SEED);
	for (unsigned int i = 0; i < 10; i++)
	{
		float size = float(std::rand() % 3 + 1);

		glm::vec3 axis(glm::vec3(float(std::rand() % 2),
			float(std::rand() % 2), float(std::rand() % 2)));
		if (glm::all(glm::equal(axis, glm::vec3(0.0f))))
			axis = glm::vec3(1.0f);

		float trans = float(std::rand() % 7 + 3) * 1.00f + 0.5f;
		glm::vec3 transVec = axis * trans;
		transVec.x *= (std::rand() % 2) ? 1.0f : -1.0f;
		transVec.y *= (std::rand() % 2) ? 1.0f : -1.0f;
		transVec.z *= (std::rand() % 2) ? 1.0f : -1.0f;

		model = glm::rotate(glm::mat4(1.0f), angle*2.0f*size, axis);
		model = glm::translate(model, transVec);
		model = glm::rotate(model, angle*2.0f*size, axis);
		model = glm::scale(model, glm::vec3(1.0f / (size*0.7f)));

		renderCube();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);  //Activamos el Frame buffer object

	//renderizado del plano
	glUseProgram(postProccesProgram);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);


	if (uColorTexPP != -1)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffTexId);
		glUniform1i(uColorTexPP, 0);
	}

	//Activamos las texturas si no da ningun error
	if (uVertexTexPP != -1)
	{
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, vertexBuffTexId);
		glUniform1i(uVertexTexPP, 1);
	}

	if (uDepthTexPP != -1)
	{
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, depthBuffTexId);
		glUniform1i(uDepthTexPP, 1);
	}
	

	//Subimos las variables uniform del shaderPP
	if (uFocalDistance != -1)
		glUniform1f(uFocalDistance, fD);
	if (uMaxDistanceFactor != -1)
		glUniform1f(uMaxDistanceFactor, mDF);
	if (uMask != -1)
		glUniformMatrix3fv(uMask, 1, GL_FALSE,
		&(mask[0][0]));

	if (uZNear != -1)
		glUniform1f(uZNear, zNear);
	if (uZFar != -1)
		glUniform1f(uZFar, zFar);


	glEnable(GL_BLEND);
	glBlendFunc(GL_CONSTANT_COLOR, GL_CONSTANT_ALPHA);
	glBlendColor(red, green, blue, alpha);
	glBlendEquation(GL_FUNC_ADD);

	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(NULL);

	glDisable(GL_BLEND);

	glutSwapBuffers();
}

void renderCube()
{
	glm::mat4 modelView = view * model;
	glm::mat4 modelViewProj = proj * view * model;
	glm::mat4 normal = glm::transpose(glm::inverse(modelView));

	if (uModelViewMat != -1)
		glUniformMatrix4fv(uModelViewMat, 1, GL_FALSE,
		&(modelView[0][0]));
	if (uModelViewProjMat != -1)
		glUniformMatrix4fv(uModelViewProjMat, 1, GL_FALSE,
		&(modelViewProj[0][0]));
	if (uNormalMat != -1)
		glUniformMatrix4fv(uNormalMat, 1, GL_FALSE,
		&(normal[0][0]));
	
	

	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, cubeNTriangleIndex * 3,
		GL_UNSIGNED_INT, (void*)0);
}



void resizeFunc(int width, int height)
{
	

	glViewport(0, 0, width, height);
	proj = glm::perspective(glm::radians(60.0f), float(width) / float(height), zNear, zFar);

	resizeFBO(width, height); //cambio de tamaño del FBO. Se cambia la matriz de proyección también.

	glutPostRedisplay();
}

void idleFunc()
{

	

	angle = (angle > 3.141592f * 2.0f) ? 0 : angle + 0.02f;

	glutPostRedisplay();
}


//Se establecen los controles poor teclado de todos los postprocesos
//EXPLICADOS EN EL GUIÓN
void keyboardFunc(unsigned char key, int x, int y)
{
	
	
	switch (key){
	case 'i':
		fD += 5.0f;
		std::cout << fD << std::endl;
		break;
	case 'o':
		fD -= 5.0f;
		std::cout << fD << std::endl;
		break;
	case 'k':
		mDF += 0.05f;
		std::cout << mDF << std::endl;
		break;
	case 'l':
		mDF -= 0.05f;
		std::cout << mDF << std::endl;
		break;
	case 'q':
		red += 0.05f;
		break;
	case 'a':
		red -= 0.05f;
		break;
	case 'w':
		green += 0.05f;
		break;
	case 's':
		green -= 0.05f;
		break;
	case 'e':
		blue += 0.05f;
		break;
	case 'd':
		blue -= 0.05f;
		break;
	case 'r':
		alpha += 0.05f;
		break;
	case 'f':
		alpha -= 0.05f;
		break;
	case 'z':
		mask = emboss*maskFactor;
		break;
	case 'x':
		mask = sharpen*maskFactor;
		break;
	case 'c':
		mask = edgeEnhance*maskFactor;
		break;
	case 'v':
		mask = edgeDetect*maskFactor;
		break;
	case 'n':
		mask /= maskFactor;
		maskFactor += 0.005;
		mask *= maskFactor;
		break;
	case 'm':
		mask /= maskFactor;
		maskFactor -= 0.005;
		mask *= maskFactor;
		break;
	}

	

}
void mouseFunc(int button, int state, int x, int y){}

void initShaderPP(const char *vname, const char *fname)
{
	postProccesVShader = loadShader(vname, GL_VERTEX_SHADER);
	postProccesFShader = loadShader(fname, GL_FRAGMENT_SHADER);
	postProccesProgram = glCreateProgram();

	glAttachShader(postProccesProgram, postProccesVShader);
	glAttachShader(postProccesProgram, postProccesFShader);
	glBindAttribLocation(postProccesProgram, 0, "inPos");
	glLinkProgram(postProccesProgram);

	int linked;
	glGetProgramiv(postProccesProgram, GL_LINK_STATUS, &linked);  //comprobar errores de linkado y qué long tiene la cadena de error y devolverlo
	if (!linked)
	{
		//Calculamos una cadena de error
		GLint logLen;
		glGetProgramiv(postProccesProgram, GL_INFO_LOG_LENGTH, &logLen);
		char *logString = new char[logLen];
		glGetProgramInfoLog(postProccesProgram, logLen, NULL, logString);
		std::cout << "Error en el enlazado del postproceso(pp): " << logString << std::endl;
		std::cout << "Error: " << logString << std::endl;
		delete logString;
		glDeleteProgram(postProccesProgram);
		postProccesProgram = 0;
		exit(-1);
	}

	//Configura la carga de shaders de PP 
	uVertexTexPP = glGetUniformLocation(postProccesProgram, "vertexTex");
	uDepthTexPP = glGetUniformLocation(postProccesProgram, "depthTex");

	uColorTexPP = glGetUniformLocation(postProccesProgram, "colorTex");
	inPosPP = glGetAttribLocation(postProccesProgram, "inPos");

	uMaxDistanceFactor = glGetUniformLocation(postProccesProgram, "maxDistanceFactor");
	uFocalDistance = glGetUniformLocation(postProccesProgram, "focalDistance");
	uMask = glGetUniformLocation(postProccesProgram, "mask");
	uZNear = glGetUniformLocation(postProccesProgram, "zNear");
	uZFar = glGetUniformLocation(postProccesProgram, "zFar");
}

void initPlane()
{
	glGenVertexArrays(1, &planeVAO); //1= Numero de ID's que quiero
	glBindVertexArray(planeVAO);

	//generar un ID de un VAO
	glGenBuffers(1, &planeVertexVBO);

	glBindBuffer(GL_ARRAY_BUFFER, planeVertexVBO);
	//Reservamos espacio
	glBufferData(GL_ARRAY_BUFFER, planeNVertex*sizeof(float) * 3,
		planeVertexPos, GL_STATIC_DRAW); //GL_STATIC_DRAW es una sugerencia para el driver por si no lo reescribimos, que optimice

	glVertexAttribPointer(inPosPP, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(inPosPP);
}


void initFBO()
{
	glGenFramebuffers(1, &fbo);
	glGenTextures(1, &colorBuffTexId);
	glGenTextures(1, &depthBuffTexId);
	//Coge el identificador de textura
	glGenTextures(1, &vertexBuffTexId);

}

void resizeFBO(unsigned int w, unsigned int h)
{
	glBindTexture(GL_TEXTURE_2D, colorBuffTexId);
	//RGBA DE 8 BITS. 32 BITS POR COLOR.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
	//Solo se escribe en un nivel de textura, no hay mitmaps.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //LINEAR para interpolar bilinearmente. No es nearest porque queremos coger valores algo más lejanos

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, depthBuffTexId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	//Inicialización de la textura
	glBindTexture(GL_TEXTURE_2D, vertexBuffTexId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, 0);  //3 canales de 32 bits, es muy grande pero si no no habría definicion suficiente.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	//el GL_COLOR_ATTACHMENT0 puede tener cualquier numero, por ejemplo 3
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffTexId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
		GL_TEXTURE_2D, vertexBuffTexId, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthBuffTexId, 0);

	//al GL_COLOR_ATTACHMENT0 le corresponde el 3(se cambia)
	//Si a la salida 0 no le corresponde ningun GL_COLOR_ATTACHMENT -> {GL_NONE,GL_COLOR_ATTACHMENT3}
	const GLenum buffs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(2, buffs);//1=cantidad de salidas que quieres enlazar(1 unico elemento)


	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER))
	{
		std::cerr << "Error configurando el FBO" << std::endl;
		exit(-1);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);  //se activa el framebuffer por defecto (el 0)
}