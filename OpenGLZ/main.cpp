#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stl.h"
#include "texture.h"
#include "OBJLoader.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <vector>
#include <iostream>
#include <random>
#include <sstream>
#include <fstream>
#include <string>
#include <cmath>

#define TINYPLY_IMPLEMENTATION
#include <tinyply.h>

#define M_PI 3.1415926535897932384626433832795f

GLFWwindow* window;
int width, height;

double cursorX, cursorY;

//----CAMERA----
float
	radius = 10.f,
	phi = M_PI / 2,
	theta = 0.f;
glm::vec3 camPos = glm::vec3(0.f, 0.f, radius);
glm::mat4 
	lookAt = glm::mat4(1.0),
	perspective = glm::perspective(M_PI / 2, 1.f, 1.f, 10000.f);

//----TRANSLATE----
float
	dx = 0.f,
	dy = 0.f,
	dz = 0.f;
glm::mat4 translateMatrix = glm::mat4(1.0);

//----SCALE----
float
	scaleX = 10.f,
	scaleY = 10.f,
	scaleZ = 10.f;
glm::mat4 scaleMatrix = glm::mat4(1.0);

//----ROTATE----
float
	rotX = 0.f,
	rotY = 0.f,
	rotZ = 0.f;
glm::mat4
	rotMatX = glm::mat4(1.0),
	rotMatY = glm::mat4(1.0),
	rotMatZ = glm::mat4(1.0),
	rotationMatrix = glm::mat4(1.0);

glm::mat4 transformMatrix = glm::mat4(1.0);

static void error_callback(int /*error*/, const char* description)
{
	std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	radius += yoffset;
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	theta += xpos * 0.0001f;
	phi += ypos * 0.0001f;
}

/* PARTICULES */
struct Particule {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 speed;
};

/* POINTS */
struct Vertex 
{
	glm::vec3 position;
	glm::vec2 uv;
};

std::vector<Particule> MakeParticules(const int n)
{
	std::default_random_engine generator;
	std::uniform_real_distribution<float> distribution01(0, 1);
	std::uniform_real_distribution<float> distributionWorld(-1, 1);

	std::vector<Particule> p;
	p.reserve(n);

	for(int i = 0; i < n; i++)
	{
		p.push_back(Particule{
				{
				distributionWorld(generator),
				distributionWorld(generator),
				distributionWorld(generator)
				},
				{
				distribution01(generator),
				distribution01(generator),
				distribution01(generator)
				},
				{0.f, 0.f, 0.f}
				});
	}

	return p;
}

GLuint MakeShader(GLuint t, std::string path)
{
	std::cout << path << std::endl;
	std::ifstream file(path.c_str(), std::ios::in);
	std::ostringstream contents;
	contents << file.rdbuf();
	file.close();

	const auto content = contents.str();
	std::cout << content << std::endl;

	const auto s = glCreateShader(t);

	GLint sizes[] = {(GLint) content.size()};
	const auto data = content.data();

	glShaderSource(s, 1, &data, sizes);
	glCompileShader(s);

	GLint success;
	glGetShaderiv(s, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetShaderInfoLog(s, 512, &l, infoLog);

		std::cout << infoLog << std::endl;
	}

	return s;
}

GLuint AttachAndLink(std::vector<GLuint> shaders)
{
	const auto prg = glCreateProgram();
	for(const auto s : shaders)
	{
		glAttachShader(prg, s);
	}

	glLinkProgram(prg);

	GLint success;
	glGetProgramiv(prg, GL_LINK_STATUS, &success);
	if(!success)
	{
		GLchar infoLog[512];
		GLsizei l;
		glGetProgramInfoLog(prg, 512, &l, infoLog);

		std::cout << infoLog << std::endl;
	}

	return prg;
}

void APIENTRY opengl_error_callback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar *message,
		const void *userParam)
{
	std::cout << message << std::endl;
}


int main(void)
{
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);
	// NOTE: OpenGL error checks have been omitted for brevity

	if(!gladLoadGL()) {
		std::cerr << "Something went wrong!" << std::endl;
		exit(-1);
	}

	// Callbacks
	glDebugMessageCallback(opengl_error_callback, nullptr);

	// Shader
	const auto vertex = MakeShader(GL_VERTEX_SHADER, "shader.vert");
	const auto fragment = MakeShader(GL_FRAGMENT_SHADER, "shader.frag");

	const auto program = AttachAndLink({vertex, fragment});

	glUseProgram(program);

	// Objects
	std::vector<Triangle> triangles = ReadStl("logo.stl");

	std::vector<Vertex> points
	{
		{
			{0, 0, 0},
			{0, 0}
		},
		{
			{0, 1, 0},
			{0, 1}
		},
		{
			{1, 0, 0},
			{1, 0}
		},
		{
			{1, 0, 0},
			{1, 0}
		},
		{
			{1, 1, 0},
			{1, 1}
		},
		{
			{0, 1, 0},
			{0, 1}
		}
	};

	std::vector<glm::vec3> verticesOBJ;
	std::vector<glm::vec2> uvsOBJ;
	std::vector<glm::vec3> normalsOBJ;
	const char* filename = "cube.obj";
	loadOBJ(filename, verticesOBJ, uvsOBJ, normalsOBJ);
	
	std::vector<Vertex> cubeVert;

	for(unsigned int i = 0; i < verticesOBJ.size(); i++)
	{
		cubeVert.push_back({
			verticesOBJ[i],
			uvsOBJ[i]
			});
	}

	// Textures
	Image bmp = LoadImage("wood.bmp");
	GLuint woodTextureID;
	glCreateTextures(GL_TEXTURE_2D, 1, &woodTextureID);
	glTextureStorage2D(woodTextureID, 1, GL_RGB8, bmp.width, bmp.height);
	glTextureSubImage2D(woodTextureID, 0, 0, 0, bmp.width, bmp.height, GL_RGB, GL_UNSIGNED_BYTE, bmp.data.data());

	GLuint frameColorTextureID;
	glCreateTextures(GL_TEXTURE_2D, 1, &frameColorTextureID);
	glTextureStorage2D(frameColorTextureID, 1, GL_RGB8, 100, 100);

	GLuint frameDepthTextureID;
	glCreateTextures(GL_TEXTURE_2D, 1, &frameDepthTextureID);
	glTextureStorage2D(frameDepthTextureID, 1, GL_DEPTH_COMPONENT24, 100, 100);

	// Buffers
	GLuint vbo, vao;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, cubeVert.size() * sizeof(Vertex), cubeVert.data(), GL_STATIC_DRAW);

	// Bindings
	const auto indexPos = glGetAttribLocation(program, "position");
	glVertexAttribPointer(indexPos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(indexPos);


	const auto indexUV = glGetAttribLocation(program, "uv");
	glVertexAttribPointer(indexUV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
	glEnableVertexAttribArray(indexUV);

	// Uniforms
	int uniformLookAt = glGetUniformLocation(program, "lookAt");
	int uniformPers = glGetUniformLocation(program, "perspective");
	int uniformTransform = glGetUniformLocation(program, "transformMatrix");
	int uniformTexture = glGetUniformLocation(program, "text");
	glProgramUniform1i(program, uniformTexture, 0);

	// Frame buffers
	GLenum gl_color_attachment0[] = { (GLenum)GL_COLOR_ATTACHMENT0 };

	GLuint frameBufferID;
	glCreateFramebuffers(1, &frameBufferID);	
	glNamedFramebufferTexture(frameBufferID, GL_COLOR_ATTACHMENT0, frameColorTextureID, 0);
	glNamedFramebufferTexture(frameBufferID, GL_DEPTH_ATTACHMENT, frameDepthTextureID, 0);
	glNamedFramebufferDrawBuffers(frameBufferID, 1, gl_color_attachment0);

	GLenum fbStatus = glCheckNamedFramebufferStatus(frameBufferID, GL_FRAMEBUFFER);

	std::cout << fbStatus << std::endl;

	switch (fbStatus)
	{
	case GL_FRAMEBUFFER_UNDEFINED:
		std::cout << 1 << std::endl;
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		std::cout << 2 << std::endl;
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
		std::cout << 3 << std::endl;
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
		std::cout << 4 << std::endl;
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER :
		std::cout << 5 << std::endl;
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED :
		std::cout << 6 << std::endl;
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE :
		std::cout << 7 << std::endl;
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		std::cout << 8 << std::endl;
		break;
	default:
		std::cout << 9 << std::endl;
		break;
	}

	glfwGetCursorPos(window, &cursorX, &cursorY);//update cursor pos
	glPointSize(2.f);
	glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(window))
	{
		//--Camera distance--
		glfwSetScrollCallback(window, scroll_callback);
		//-------------------

		//--Camera rotation--
		double 
			oldCursorX = cursorX, 
			oldCursorY = cursorY;
		glfwGetCursorPos(window, &cursorX, &cursorY);//update cursor pos

		theta += (oldCursorX - cursorX) * 0.001f;
		phi += (oldCursorY - cursorY) * 0.001f;

		camPos = glm::vec3(radius * sin(phi) * cos(theta) , radius * cos(phi), radius * sin(phi) * sin(theta));
		lookAt = glm::lookAt(camPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		//-------------------

		//--Object transformations--
		translateMatrix = glm::translate(glm::vec3(dx, dy, dz));

		scaleMatrix = glm::scale(glm::vec3(scaleX, scaleY, scaleZ));

		rotMatX = glm::rotate(rotX, glm::vec3(1, 0, 0));
		rotMatY = glm::rotate(rotY, glm::vec3(0, 1, 0));
		rotMatZ = glm::rotate(rotZ, glm::vec3(0, 0, 1));
		rotationMatrix = rotMatX * rotMatY * rotMatZ;

		transformMatrix = rotationMatrix * translateMatrix * scaleMatrix;
		//--------------------------

		glProgramUniformMatrix4fv(program, uniformLookAt, 1, GL_FALSE, &lookAt[0][0]);
		glProgramUniformMatrix4fv(program, uniformPers, 1, GL_FALSE, &perspective[0][0]);
		glProgramUniformMatrix4fv(program, uniformTransform, 1, GL_FALSE, &transformMatrix[0][0]);

		//--Dessine sur le framebuffer cache--
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferID);

		glViewport(0, 0, 100, 100);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindTextureUnit(0, woodTextureID);
		
		glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);

		//----Dessine a l'ecran----
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		glBindTextureUnit(0, frameColorTextureID);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
