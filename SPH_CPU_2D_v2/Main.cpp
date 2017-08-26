/** File:		Main.cpp
 ** Author:		Dongli Zhang
 ** Contact:	dongli.zhang0129@gmail.com
 **
 ** Copyright (C) Dongli Zhang 2013
 **
 ** This program is free software;  you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY;  without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 ** the GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program;  if not, write to the Free Software
 ** Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SPHSystem.h"

SPHSystem* sph;

GLFWwindow* window;
int windowWidth = 540;
int windowHeight = 960;
char*const windowTitle = "SPH CPU 2D v2";

unsigned buffer, vertexArray;

bool initGLEW()
{
	glewExperimental = GL_TRUE;
	auto err = glewInit();
	if (GLEW_OK != err)
	{
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		return false;
	}
	std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
	return true;
}

bool initGLFW()
{
	if (!glfwInit())
	{
		std::cerr << "Error: could not start GLFW3." << std::endl;
		return false;
	}
	window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Error: could not create window." << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	return true;
}

void initBuffer()
{
	auto const sizeOfBuffer{sph->getNumParticle() * sizeof(Particle)};
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, sph->getParticles(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initVertexArray()
{
	glGenVertexArrays(1, &vertexArray);
	glBindVertexArray(vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 2));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 4));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 6));
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 8));
	glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 9));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(0); // Vec2f pos;
	glEnableVertexAttribArray(1); // Vec2f vel;
	glEnableVertexAttribArray(2); // Vec2f acc;
	glEnableVertexAttribArray(3); // Vec2f ev;
	glEnableVertexAttribArray(4); // float dens;
	glEnableVertexAttribArray(5); // float pres;
	glBindVertexArray(0);
}

void updateBuffer()
{
	auto const sizeOfBuffer{sph->getNumParticle() * sizeof(Particle)};
	auto bufferData{sph->getParticles()};
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeOfBuffer, bufferData);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawVertexArray()
{
	glBindVertexArray(vertexArray);
	glDrawArrays(GL_POINTS, 0, sph->getNumParticle());
	glBindVertexArray(0);
}

void PrintShaderInfoLog(GLuint const& shader)
{
	GLint shaderCompileStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompileStatus);
	fprintf(stdout, " - GL_COMPILE_STATUS = %s\n", GL_TRUE == shaderCompileStatus ? "Success" : "Failure");
	if (GL_TRUE == shaderCompileStatus) return;
	GLchar infoLog[8192];
	GLsizei infoLogLength;
	glGetShaderInfoLog(shader, 8192, &infoLogLength, infoLog);
	if (infoLogLength <= 0) return;
	std::cout << "-> Shader Info Log: " << infoLog << std::endl;
}

unsigned program;

void initProgram()
{
	auto vertSrc =
		"\n #version 330 core"
		"\n layout(location=0) in vec2 pos;"
		"\n layout(location=1) in vec2 vel;"
		"\n layout(location=2) in vec2 acc;"
		"\n layout(location=3) in vec2 ev;"
		"\n layout(location=4) in float dens;"
		"\n layout(location=5) in float pres;"
		"\n out vec2 fragVel;"
		"\n out float fragDens;"
		"\n out float fragPres;"
		"\n uniform mat4 mvpMat;"
		"\n void main() {"
		"\n   fragVel = vel;"
		"\n   fragDens = dens;"
		"\n   fragPres = pres;"
		"\n   gl_Position = mvpMat * vec4(pos, 0, 1);"
		"\n }";

	auto fragSrc =
		"\n #version 330 core"
		"\n layout(location=0) out vec4 color;"
		"\n in vec2 fragVel;"
		"\n out float fragDens;"
		"\n out float fragPres;"
		"\n void main() {"
		"\n   color = vec4(0.5 * (normalize(fragVel) + 1.0), 1.0, 1.0);"
		"\n }";

	auto vert{glCreateShader(GL_VERTEX_SHADER)}, frag{glCreateShader(GL_FRAGMENT_SHADER)};
	glShaderSource(vert, 1, &vertSrc, nullptr);
	glShaderSource(frag, 1, &fragSrc, nullptr);
	glCompileShader(vert);
	PrintShaderInfoLog(vert);
	glCompileShader(frag);
	PrintShaderInfoLog(frag);
	program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
}

int main(int argc, char** argv)
{
	using glm::mat4;
	using glm::ortho;

	sph = new SPHSystem();
	sph->initFluid();

	if (!initGLFW()) return EXIT_FAILURE;
	if (!initGLEW()) return EXIT_FAILURE;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL version supported " << glGetString(GL_VERSION) << std::endl;

	initProgram();
	initBuffer();
	initVertexArray();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	auto mvpMatLocation{glGetUniformLocation(program, "mvpMat")};

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		/* Render here */
		auto width{0}, height{0};
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);

		auto const worldWidth{float(sph->getWorldSize().x)}, worldHeight{float(sph->getWorldSize().y)};
		auto const projectionMat{ortho(0.f, worldWidth, 0.f, worldHeight)};
		auto const modelMat{mat4()}, viewMat{mat4()}, mvpMat{projectionMat * viewMat * modelMat};

		glClearColor(.0f, .0f, .0f, .0f);
		glClear(GL_COLOR_BUFFER_BIT);

		sph->animation();

		updateBuffer();

		glUseProgram(program);
		glUniformMatrix4fv(mvpMatLocation, 1, GL_FALSE, value_ptr(mvpMat));
		glEnable(GL_PROGRAM_POINT_SIZE);
		glPointSize(2);
		drawVertexArray();
		glUseProgram(0);

		glfwSwapBuffers(window);
	}
	glfwTerminate();

	free(sph);
	return 0;
}
