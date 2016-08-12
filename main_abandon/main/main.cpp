// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#include <Windows.h>
#include <iostream>


#define T0 64
#define NMAX 8
#define MAXDEPTH 20

typedef enum 
{
	PRINT_INFO = 0xf00000,
};

bool checkErr(int status, char* string)
{
	if ( PRINT_INFO == status)
	{
		std::cout<<string<<std::endl;
		system("echo =================================");
		system("pause");
	}
	/*if (CL_SUCCESS != status)
	{
	std::cout<<string<<std::endl;
	system("echo =================================");
	system("pause");
	exit(0);
	}*/
	return true;
}

void main(int argc, char** argv)
{
	//创建Opengl显示窗口
	if( !glfwInit())
	{
		checkErr(PRINT_INFO, "fail to init glfw!");
		exit(0);
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(1400, 1080, " OpenCL加速kd-tree以及渲染过程程序", NULL, NULL);
	if ( window == NULL)
	{
		checkErr(PRINT_INFO, "fail to open glfw window!");
		glfwTerminate();
		exit(0);
	}
	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if (glewInit() != GLEW_OK) {
		checkErr(PRINT_INFO, "fail to init glew");
		glfwTerminate();
		exit(0);
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwPollEvents();
	glfwSetCursorPos(window, 1024/2, 768/2);

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	std::vector<glm::vec3> vv3Verts;
	std::vector<glm::vec2> vv2UVs;
	std::vector<glm::vec3> vv3Nor;

	bool bRes = loadOBJ("bunny.obj", vv3Verts, vv2UVs, vv3Nor);

	while (1)
	{
	};
}

