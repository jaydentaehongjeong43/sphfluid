#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "utf8.h"

extern GLFWwindow* window;
int const windowWidth = 512;
int const windowHeight = 512;
char* const windowTitle = "SPH CPU 2D v2";

inline bool initGLEW()
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

inline bool initGLFW()
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

inline bool ReadShaderSourceFromFile(std::string const filename, std::string& str)
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

inline void PrintShaderInfoLog(unsigned const& shader)
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

inline unsigned MakeShader(std::string const filename, int const shaderType)
{
	std::string str;
	if (!ReadShaderSourceFromFile(filename, str)) return 0;
	auto const src{str.c_str()};
	auto const shader{glCreateShader(shaderType)};
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	std::cout << "MakeShader(filename = " << filename << ", shaderType = " << shaderType << ")" << std::endl;
	PrintShaderInfoLog(shader);
	return shader;
}

inline unsigned MakeSphProgram(unsigned const shader)
{
	auto const program{glCreateProgram()};
	glAttachShader(program, shader);
	auto const xfbVaryings{"vertOut"};
	glTransformFeedbackVaryings(program, 1, &xfbVaryings, GL_INTERLEAVED_ATTRIBS);
	glLinkProgram(program);
	return program;
}
