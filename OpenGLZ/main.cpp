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
int frameWidth = 500, frameHeight = 500;
int nParticules = 10;

double 
	oldCursorX, oldCursorY,
	cursorX, cursorY;

//----CAMERA----
float
	radius = 200.f,
	phi = M_PI / 2,
	theta = 0.f;
glm::vec3 camPos = glm::vec3(0.f, 0.f, radius);
glm::mat4 
	lookAt = glm::mat4(1.0),
	perspective = glm::perspective(M_PI / 2, 1.f, 0.1f, 1000.f);

//----TRANSLATE----
float
	dx = 0.f,
	dy = 0.f,
	dz = 0.f;
glm::mat4 translateMatrix = glm::mat4(1.0);

//----SCALE----
float
	scaleX = 40.f,
	scaleY = 40.f,
	scaleZ = 40.f;
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
	glm::vec4 position;
	glm::vec4 color;
	glm::vec4 speed;
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
	std::uniform_real_distribution<float> distributionMass(10, 100);

	std::vector<Particule> p;
	p.reserve(n);

	for(int i = 0; i < n; i++)
	{
		float col = distribution01(generator);
		p.push_back(Particule{
				{ // pos
					distributionWorld(generator),
					distributionWorld(generator),
					distributionWorld(generator),
					distribution01(generator) * 100 // mass
				}, 
				{ // color
					col, col, col,
					1.f
				},
				{ // speed
					0.f, 
					0.f,
					0.f,
					1.f
				},
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

glm::vec3 wrapAround(glm::vec3 pos)
{
	if (pos.x > 1.f) pos.x = -1.f;
	if (pos.y > 1.f) pos.y = -1.f;
	if (pos.z > 1.f) pos.z = -1.f;

	if (pos.x < -1.f) pos.x = 1.f;
	if (pos.y < -1.f) pos.y = 1.f;
	if (pos.z < -1.f) pos.z = 1.f;

	return pos;
};

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
	const auto compute = MakeShader(GL_COMPUTE_SHADER, "shader.comp");

	const auto programDisplay = AttachAndLink({vertex, fragment});
	const auto programCompute = AttachAndLink({ compute });

	glUseProgram(programDisplay);

	// Objects
	std::vector<Triangle> triangles = ReadStl("logo.stl");

	// - Cube
	std::vector<Vertex> cubePoints
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
	std::cout << verticesOBJ.size() << " " << uvsOBJ.size() << std::endl;
	for(unsigned int i = 0; i < verticesOBJ.size(); i++)
	{
		cubeVert.push_back({
			verticesOBJ[i],
			uvsOBJ[i]
			});
	}
	// - End Cube

	// - Particules
	std::vector<Particule> particules = MakeParticules(nParticules);
	// - End Particules

	// Textures
	Image bmp = LoadImage("wood.bmp");
	GLuint woodTextureID;
	glCreateTextures(GL_TEXTURE_2D, 1, &woodTextureID);
	glTextureStorage2D(woodTextureID, 1, GL_RGB8, bmp.width, bmp.height);
	glTextureSubImage2D(woodTextureID, 0, 0, 0, bmp.width, bmp.height, GL_RGB, GL_UNSIGNED_BYTE, bmp.data.data());

	GLuint frameColorTextureID;
	glCreateTextures(GL_TEXTURE_2D, 1, &frameColorTextureID);
	glTextureStorage2D(frameColorTextureID, 1, GL_RGB8, frameWidth, frameHeight);

	GLuint frameDepthTextureID;
	glCreateTextures(GL_TEXTURE_2D, 1, &frameDepthTextureID);
	glTextureStorage2D(frameDepthTextureID, 1, GL_DEPTH_COMPONENT16, frameWidth, frameHeight);

	// Buffers
	GLuint vbo, vao;
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, particules.size() * sizeof(Particule), particules.data(), GL_STATIC_DRAW);

	// Bindings
	const auto indexPos = glGetAttribLocation(programDisplay, "position");
	glVertexAttribPointer(indexPos, 3, GL_FLOAT, GL_FALSE, sizeof(Particule), nullptr);
	glEnableVertexAttribArray(indexPos);

	const auto indexCol = glGetAttribLocation(programDisplay, "color");
	glVertexAttribPointer(indexCol, 4, GL_FLOAT, GL_FALSE, sizeof(Particule), (void*)16);
	glEnableVertexAttribArray(indexCol);

	// Uniforms
	int uniformLookAt = glGetUniformLocation(programDisplay, "lookAt");
	int uniformPers = glGetUniformLocation(programDisplay, "perspective");
	int uniformTransform = glGetUniformLocation(programDisplay, "transformMatrix");
	int uniformDt = glGetUniformLocation(programCompute, "dt");

	// Frame buffers
	GLenum gl_color_attachment0[] = { GL_COLOR_ATTACHMENT0 };

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

	// Light point
	glm::vec3 lightPos = {100, 0, 0};

	glfwGetCursorPos(window, &cursorX, &cursorY);//update cursor pos
	glPointSize(2.f);
	glEnable(GL_DEPTH_TEST);

	float time = glfwGetTime();
	int frame = 0;
	float timeSum = 0;
	while (!glfwWindowShouldClose(window))
	{
		auto frameTime = glfwGetTime();
		auto dt = frameTime - time;
		timeSum += dt;
		if (frame == 1000) 
		{
			std::cout << 1 / (timeSum / 1000) << std::endl;
			frame = 0;
			timeSum = 0;
		}

		// GPU compute shaders
		/*glUseProgram(programCompute);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo);
		glDispatchCompute(nParticules / 32, 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);*/

		glBindVertexArray(vao);
		glUseProgram(programDisplay);

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Camera
		{
			//--Camera distance--
			glfwSetScrollCallback(window, scroll_callback);
			//-------------------

			//--Camera rotation--
			oldCursorX = cursorX;
			oldCursorY = cursorY;
			glfwGetCursorPos(window, &cursorX, &cursorY);//update cursor pos

			theta += (oldCursorX - cursorX) * 0.001f;
			phi += (oldCursorY - cursorY) * 0.001f;

			camPos = glm::vec3(radius * sin(phi) * cos(theta), radius * cos(phi), radius * sin(phi) * sin(theta));
			lookAt = glm::lookAt(camPos, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
		}

		// Object 1
		{
			//--Object transformations--
			translateMatrix = glm::translate(glm::vec3(dx, dy, dz));

			scaleMatrix = glm::scale(glm::vec3(scaleX, scaleY, scaleZ));

			rotMatX = glm::rotate(rotX, glm::vec3(1, 0, 0));
			rotMatY = glm::rotate(rotY, glm::vec3(0, 1, 0));
			rotMatZ = glm::rotate(rotZ, glm::vec3(0, 0, 1));
			rotationMatrix = rotMatX * rotMatY * rotMatZ;

			transformMatrix = rotationMatrix * translateMatrix * scaleMatrix;
			//--------------------------

			// Compute with CPU
			float g = -9.81f;
			for (int i = 0; i < nParticules; i++) 
			{
				glm::vec3 accel = particules[i].position.w * g * glm::vec3(0.f, 0.1f, 0.f);
				particules[i].speed += glm::vec4(accel * float(dt), particules[i].speed.w);
				particules[i].position += glm::vec4(particules[i].speed.x * dt, particules[i].speed.y * dt, particules[i].speed.z * dt, 0.f);
				particules[i].position = glm::vec4(wrapAround(particules[i].position), particules[i].position.w);
			}
			glBufferSubData(GL_ARRAY_BUFFER, 0, nParticules * sizeof(Particule), particules.data());

		}

		glProgramUniformMatrix4fv(programDisplay, uniformLookAt, 1, GL_FALSE, &lookAt[0][0]);
		glProgramUniformMatrix4fv(programDisplay, uniformPers, 1, GL_FALSE, &perspective[0][0]);
		glProgramUniformMatrix4fv(programDisplay, uniformTransform, 1, GL_FALSE, &transformMatrix[0][0]);
		glProgramUniform1f(programCompute, uniformDt, dt);

		time = frameTime;
		frame++;


		glDrawArrays(GL_POINTS, 0, particules.size());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
