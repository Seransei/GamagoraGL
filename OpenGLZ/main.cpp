#include <glad/glad.h>
#include "stl.h"
#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
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
	scaleX = 0.1f,
	scaleY = 0.1f,
	scaleZ = 0.1f;
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
	
	auto trianglesLogo = ReadStl("logo.stl");

	// Shader
	const auto vertex = MakeShader(GL_VERTEX_SHADER, "shader.vert");
	const auto fragment = MakeShader(GL_FRAGMENT_SHADER, "shader.frag");

	const auto program = AttachAndLink({vertex, fragment});

	glUseProgram(program);

	// Buffers
	
	GLuint vbo, vao;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, trianglesLogo.size() * sizeof(glm::vec3) * 3, trianglesLogo.data(), GL_STATIC_DRAW);

	// Bindings

	const auto index = glGetAttribLocation(program, "position");

	glVertexAttribPointer(index, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
	glEnableVertexAttribArray(index);

	glPointSize(2.f);

	while (!glfwWindowShouldClose(window))
	{
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);

		//--Camera distance--
		glfwSetScrollCallback(window, scroll_callback);
		//-------------------

		//--Camera rotation--
		double 
			oldCursorX = cursorX, 
			oldCursorY = cursorY;
		glfwGetCursorPos(window, &cursorX, &cursorY);//update cursor pos

		theta += (cursorX - oldCursorX) * 0.001f;
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

		int uniformLookAt = glGetUniformLocation(program, "lookAt");
		glProgramUniformMatrix4fv(program, uniformLookAt, 1, GL_FALSE, &lookAt[0][0]);

		int uniformPers = glGetUniformLocation(program, "perspective");
		glProgramUniformMatrix4fv(program, uniformPers, 1, GL_FALSE, &perspective[0][0]);

		int uniformTransform = glGetUniformLocation(program, "transformMatrix");
		glProgramUniformMatrix4fv(program, uniformTransform, 1, GL_FALSE, &transformMatrix[0][0]);


		glClear(GL_COLOR_BUFFER_BIT);
		// glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

		glDrawArrays(GL_TRIANGLES, 0, trianglesLogo.size() * 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
