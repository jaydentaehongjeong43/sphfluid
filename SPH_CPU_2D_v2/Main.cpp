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
#include "stdafx.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SPHSystem.h"

GLFWwindow* window;

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

int main(int argc, char** argv)
{
	using glm::mat4;
	using glm::ortho;

	if (!initGLFW()) return EXIT_FAILURE;
	if (!initGLEW()) return EXIT_FAILURE;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "OpenGL version supported " << glGetString(GL_VERSION) << std::endl;

	initProgram();

	auto const shaderSphDensPres{MakeShader("./Shaders/sph-0-dens-pres.vert", GL_VERTEX_SHADER)};
	auto const shaderSphAcc{MakeShader("./Shaders/sph-1-acc.vert", GL_VERTEX_SHADER)};
	auto const shaderSphVelPos{MakeShader("./Shaders/sph-2-vel-pos.vert", GL_VERTEX_SHADER)};

	auto const programSphDensPres{MakeSphProgram(shaderSphDensPres)};
	auto const programSphAcc{MakeSphProgram(shaderSphAcc)};
	auto const programSphVelPos{MakeSphProgram(shaderSphVelPos)};

	MySphSystem sph;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		/* Render here */
		auto width{0}, height{0};
		glfwGetWindowSize(window, &width, &height);
		glViewport(0, 0, width, height);

		auto const projectionMat{ortho(0.f, sph.worldSize.x, 0.f, sph.worldSize.y)};
		auto const modelMat{mat4()}, viewMat{mat4()}, mvpMat{projectionMat * viewMat * modelMat};

		glClearColor(.1f, .1f, .1f, .1f);
		glClear(GL_COLOR_BUFFER_BIT);

		sph.pullParticlesFromBuffer(sph.getPrimaryParticleBuffer());
		sph.buildGrid();
		sph.pushCellsToBuffer();
		sph.pushParticlesToBuffer(sph.getPrimaryParticleBuffer());
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		{
			glUseProgram(programSphDensPres);
			{
				glUniform1f(glGetUniformLocation(programSphDensPres, "cellSize"), sph.cellSize);
				glUniform1i(glGetUniformLocation(programSphDensPres, "gridWidth"), sph.gridSize.x);
				glUniform1i(glGetUniformLocation(programSphDensPres, "gridHeight"), sph.gridSize.y);
				glUniform1f(glGetUniformLocation(programSphDensPres, "kernel"), sph.kernel);
				glUniform1f(glGetUniformLocation(programSphDensPres, "mass"), sph.mass);
				glUniform1f(glGetUniformLocation(programSphDensPres, "restDensity"), sph.restDensity);
				glUniform1f(glGetUniformLocation(programSphDensPres, "stiffness"), sph.stiffness);
				sph.runXfb();
				sph.swapParticleBuffers();
			}
			glUseProgram(programSphAcc);
			{
				glUniform1f(glGetUniformLocation(programSphAcc, "cellSize"), sph.cellSize);
				glUniform1i(glGetUniformLocation(programSphAcc, "gridWidth"), sph.gridSize.x);
				glUniform1i(glGetUniformLocation(programSphAcc, "gridHeight"), sph.gridSize.y);
				glUniform1f(glGetUniformLocation(programSphAcc, "kernel"), sph.kernel);
				glUniform1f(glGetUniformLocation(programSphAcc, "mass"), sph.mass);
				glUniform1f(glGetUniformLocation(programSphAcc, "viscosity"), sph.viscosity);
				glUniform2fv(glGetUniformLocation(programSphAcc, "gravity"), 1, value_ptr(sph.gravity));
				sph.runXfb();
				sph.swapParticleBuffers();
			}
			glUseProgram(programSphVelPos);
			{
				glUniform1f(glGetUniformLocation(programSphVelPos, "timeStep"), sph.timeStep);
				glUniform1f(glGetUniformLocation(programSphVelPos, "wallDamping"), sph.wallDamping);
				glUniform2fv(glGetUniformLocation(programSphVelPos, "worldSize"), 1, value_ptr(sph.worldSize));
				sph.runXfb();
				sph.swapParticleBuffers();
			}
		}
		glUseProgram(program);
		{
			glUniformMatrix4fv(glGetUniformLocation(program, "mvpMat"), 1, GL_FALSE, value_ptr(mvpMat));
			glEnable(GL_PROGRAM_POINT_SIZE);
			glPointSize(2);
			sph.drawVertexArray();
		}
		glfwSwapBuffers(window);
	}

	glDetachShader(programSphDensPres, shaderSphDensPres);
	glDeleteShader(shaderSphDensPres);
	glDeleteProgram(programSphDensPres);

	glDetachShader(programSphAcc, shaderSphAcc);
	glDeleteShader(shaderSphAcc);
	glDeleteProgram(programSphAcc);

	glDetachShader(programSphVelPos, shaderSphVelPos);
	glDeleteShader(shaderSphVelPos);
	glDeleteProgram(programSphVelPos);

	glfwTerminate();

	return 0;
}
