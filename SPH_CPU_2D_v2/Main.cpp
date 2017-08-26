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
#include <fstream>
#include <sstream>
#include "utf8.h"
#include "XfbClient.h"

GLFWwindow* window;
int windowWidth = 540;
int windowHeight = 960;
char*const windowTitle = "SPH CPU 2D v2";


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
		"\n layout(location = 0) in vec2  pos;"
		"\n layout(location = 1) in vec2  vel;"
		"\n layout(location = 2) in vec2  acc;"
		"\n layout(location = 3) in vec2  ev;"
		"\n layout(location = 4) in float dens;"
		"\n layout(location = 5) in float pres;"
		"\n out vec2 fragVel;"
		"\n uniform mat4 mvpMat;"
		"\n void main() {"
		"\n   fragVel = vel;"
		"\n   gl_Position = mvpMat * vec4(pos, 0, 1);"
		"\n }";

	auto fragSrc =
		"\n #version 440 core"
		"\n layout(location=0) out vec4 color;"
		"\n in vec2 fragVel;"
		"\n void main() {"
		"\n   color = vec4(0.5 * (normalize(fragVel) + 1.0), 1, 1.0);"
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

unsigned shaderXfbVert{0}, programXfb;

bool ReadShaderSourceFromFile(std::string const filename, std::string& str)
{
	std::ifstream ifs(filename);
	if (!ifs.good())
	{
		std::cerr << "Failed to open \"" << filename << "\". Press enter to exit.";
		return false;
	}
	std::stringstream ss;
	ss << ifs.rdbuf();
	str = {ss.str()};
	std::string temp;
	utf8::replace_invalid(str.begin(), str.end(), back_inserter(temp));
	str = temp;
	return true;
}

void initXfbProgram()
{
	std::string str;
	if (!ReadShaderSourceFromFile("./sph.vert", str))
		return;

	auto vertSrc{str.c_str()};
	shaderXfbVert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shaderXfbVert, 1, &vertSrc, nullptr);
	glCompileShader(shaderXfbVert);
	PrintShaderInfoLog(shaderXfbVert);

	programXfb = glCreateProgram();
	glAttachShader(programXfb, shaderXfbVert);
	auto const xfbVaryings{"vertOut"};
	glTransformFeedbackVaryings(programXfb, 1, &xfbVaryings, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(programXfb);
}

unsigned texCells;

void initTexCells(SPHSystem const& sphSystem)
{
	glGenTextures(1, &texCells);
	glBindTexture(GL_TEXTURE_2D, texCells);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, sphSystem.getGridWidth(), sphSystem.getGridHeight(), 0, GL_RED_INTEGER, GL_INT, sphSystem.getCells());
	glBindTexture(GL_TEXTURE_2D, 0);
}

struct MySphSystem: SPHSystem
{
	unsigned getCellBufferSize() const
	{
		return sizeof(Cell) * totCell;
	}

	void initCellBuffer() const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, cellBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, getCellBufferSize(), getCells(), GL_DYNAMIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	MySphSystem() : SPHSystem()
	{
		initFluid();
		glGenBuffers(1, &initialParticleBuffer);
		initInitialParticleBuffer();
		glGenBuffers(1, &cellBuffer);
		initCellBuffer();
		glGenVertexArrays(1, &vertexArray);
		initVertexArray(getInitialParticleBuffer());
	}

	unsigned getParticleBufferSize() const
	{
		return getNumParticles() * sizeof(Particle);
	}

//	void updateBuffer() const
//	{
//		glBindBuffer(GL_SHADER_STORAGE_BUFFER, getInitialParticleBuffer());
//		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, getBufferSize(), getParticles());
//		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
//	}

	void drawVertexArray() const
	{
		glBindVertexArray(getVertexArray());
		glDrawArrays(GL_POINTS, 0, getNumParticles());
		glBindVertexArray(0);
	}

	unsigned getInitialParticleBuffer() const
	{
		return initialParticleBuffer;
	}

	unsigned getVertexArray() const
	{
		return vertexArray;
	}

	void initVertexArray(unsigned particleBuffer) const
	{
		glBindVertexArray(getVertexArray());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particleBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cellBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, particleBuffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 0));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 2));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 4));
		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 6));
		glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 8));
		glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void* const>(sizeof(float) * 9));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0); // vec2 pos;
		glEnableVertexAttribArray(1); // vec2 vel;
		glEnableVertexAttribArray(2); // vec2 acc;
		glEnableVertexAttribArray(3); // vec2 ev;
		glEnableVertexAttribArray(4); // float dens;
		glEnableVertexAttribArray(5); // float pres;
		glBindVertexArray(0);
	}

private:

	void initInitialParticleBuffer() const
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, getInitialParticleBuffer());
		glBufferData(GL_SHADER_STORAGE_BUFFER, getParticleBufferSize(), getParticles(), GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	}

	unsigned initialParticleBuffer{ 0 };
	unsigned cellBuffer{0};
	unsigned vertexArray{0};
};

struct MyFluidSystem
{
	struct XfbClientImpl: XfbClient
	{
		explicit XfbClientImpl(MySphSystem const& _sphSystem) : XfbClient(), sphSystem_(_sphSystem)
		{
			lastBufferSize_ = XfbClient::initBuffer();
			XfbClient::initXfb();
		}

		unsigned getPrims() const override
		{
			return sphSystem_.getNumParticles();
		}

		unsigned getVerts() const override
		{
			return getPrims() * 1; // point-to-point
		}

		unsigned getBufferSize() const override
		{
			return getVerts() * sizeof(Particle);
		}

		void drawXfb() const
		{
			sphSystem_.initVertexArray(getBuffer());
			sphSystem_.drawVertexArray();
		}

		void operator()()
		{
			if (needInitBuffer()) lastBufferSize_ = initBuffer();
			BeginTransformFeedback(getXfb(), GL_POINTS);
			sphSystem_.drawVertexArray();
			EndTransformFeedback();
		}

		void operator()(XfbClientImpl& other)
		{
			if (needInitBuffer()) lastBufferSize_ = initBuffer();
			BeginTransformFeedback(getXfb(), GL_POINTS);
			other.drawXfb();
			EndTransformFeedback();
		}

		MySphSystem const& sphSystem_;
	};

	explicit MyFluidSystem(MySphSystem const& _sphSystem) :
		sphSystem_(_sphSystem), ping_(sphSystem_), pong_(sphSystem_)
	{
	}

	void simulate()
	{
		if (toggle == -1)
		{
			ping_();
			toggle = 0;
		}
		else
		{
			auto& pongOrPing{toggle == 0 ? pong_ : ping_};
			auto& pingOrPong{toggle == 1 ? pong_ : ping_};
			pongOrPing(pingOrPong);
			toggle = 1 - toggle;
		}
	}

	void draw()
	{
		auto& pongOrPing{toggle == 1 ? pong_ : ping_};
		pongOrPing.drawXfb();
	}

	MySphSystem const& sphSystem_;
	XfbClientImpl ping_;
	XfbClientImpl pong_;

	int toggle{-1};
};

int main(int argc, char** argv)
{
	using glm::mat4;
	using glm::ortho;


	if (!initGLFW()) return EXIT_FAILURE;
	if (!initGLEW()) return EXIT_FAILURE;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL version supported " << glGetString(GL_VERSION) << std::endl;

	MySphSystem sph;
	MyFluidSystem fluid(sph);

//	initTexCells(sph);
	initProgram();
	initXfbProgram();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	auto mvpMatLocation{glGetUniformLocation(program, "mvpMat")};

//	auto cellsLocation{glGetUniformLocation(programXfb, "cells")};
	auto cellSizeLocation{glGetUniformLocation(programXfb, "cellSize")};
	auto gridWidthLocation{glGetUniformLocation(programXfb, "gridWidth")};
	auto gridHeightLocation{ glGetUniformLocation(programXfb, "gridHeight") };
	auto massLocation{ glGetUniformLocation(programXfb, "mass") };
	auto restDensityLocation{ glGetUniformLocation(programXfb, "restDensity") };
	auto stiffnessLocation{ glGetUniformLocation(programXfb, "stiffness") };
	auto kernelLocation{glGetUniformLocation(programXfb, "kernel")};
	auto gravityLocation{ glGetUniformLocation(programXfb, "gravity") };
	auto viscosityLocation{ glGetUniformLocation(programXfb, "viscosity") };
	auto wallDampingLocation{ glGetUniformLocation(programXfb, "wallDamping") };
	auto timeStepLocation{ glGetUniformLocation(programXfb, "timeStep") };
	auto worldSizeLocation{ glGetUniformLocation(programXfb, "worldSize") };
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		/* Render here */
		auto width{0}, height{0};
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);

		auto const worldWidth{float(sph.getWorldSize().x)}, worldHeight{float(sph.getWorldSize().y)};
		auto const projectionMat{ortho(0.f, worldWidth, 0.f, worldHeight)};
		auto const modelMat{mat4()}, viewMat{mat4()}, mvpMat{projectionMat * viewMat * modelMat};

		glClearColor(.0f, .0f, .0f, .0f);
		glClear(GL_COLOR_BUFFER_BIT);

//		sph.animation();
//		sph.updateBuffer();
//		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
//		{
			sph.clearGrid();
			sph.buildGrid();
			sph.initCellBuffer();
//			glBindTexture(GL_TEXTURE_2D, texCells);
//			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sph.getGridWidth(), sph.getGridHeight(), GL_RED_INTEGER, GL_INT, sph.getCells());
//			glBindTexture(GL_TEXTURE_2D, 0);

			glUseProgram(programXfb);
//			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, texCells);
//			glUniform1i(cellsLocation, 0);
			glUniform1f(cellSizeLocation, sph.getCellSize());
			glUniform1i(gridWidthLocation, sph.getGridWidth());
			glUniform1i(gridHeightLocation, sph.getGridHeight());
			glUniform1f(massLocation, sph.getMass());
			glUniform1f(restDensityLocation, sph.getRestDensity());
			glUniform1f(stiffnessLocation, sph.getStiffness());
			glUniform1f(kernelLocation, sph.getKernel());
			glUniform1f(viscosityLocation, sph.getViscosity());
			glUniform1f(wallDampingLocation, sph.getWallDamping());
			glUniform1f(timeStepLocation, sph.getTimeStep());
			glUniform2fv(gravityLocation, 1, glm::value_ptr(sph.getGravity()));
			glUniform2fv(worldSizeLocation, 1, glm::value_ptr(sph.getWorldSize()));
			fluid.simulate();
			glUseProgram(0);
//		}

		glUseProgram(program);
		glUniformMatrix4fv(mvpMatLocation, 1, GL_FALSE, value_ptr(mvpMat));
		glEnable(GL_PROGRAM_POINT_SIZE);
		glPointSize(2);
		fluid.draw();
		glUseProgram(0);

		

		glfwSwapBuffers(window);
	}
	glfwTerminate();

	return 0;
}
