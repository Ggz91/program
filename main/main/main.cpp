#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <fstream>

#include <GL/glew.h>

#include <GLFW/glfw3.h>
GLFWwindow* window;

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

#include <CL/cl.h>
#include <CL/cl_gl.h>

#include "custom.h"

int iWinWidth = 1024;
int iWinHeight = 768;

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

	window = glfwCreateWindow(iWinWidth, iWinHeight, " OpenCL-OpenGL-Program", NULL, NULL);
	if ( window == NULL)
	{
		checkErr(PRINT_INFO, "fail to open glfw window!");
		glfwTerminate();
		exit(0);
	}
	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if ( glewInit() != GLEW_OK ) {
		checkErr(PRINT_INFO, "fail to init glew");
		glfwTerminate();
		exit(0);
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwPollEvents();
	glfwSetCursorPos(window, iWinWidth/2, iWinHeight/2);

	//读取obj文件的数据
	std::vector<glm::vec3> vv3Verts;
	std::vector<glm::vec2> vv2UVs;
	std::vector<glm::vec3> vv3Nor;

	bool bRes = loadOBJ("suzanne.obj", vv3Verts, vv2UVs, vv3Nor);

	DrawableInfo* pRes = new DrawableInfo;
	pRes = getTriangles(&vv3Verts[0], vv3Verts.size());

	//设置OpenCL环境
	cl_int uiStatus;
	cl_uint	iPlatformNum;
	uiStatus = clGetPlatformIDs(0, NULL, &iPlatformNum);
	checkErr(uiStatus, "fail to get the num of platforms");

	std::vector<cl_platform_id> svPlatformIDs(iPlatformNum);
	uiStatus = clGetPlatformIDs(iPlatformNum, &svPlatformIDs[0], NULL);
	checkErr(uiStatus, "fail to get the ids of platforms");

	cl_uint iDeviceNum;
	uiStatus = clGetDeviceIDs(svPlatformIDs[0], CL_DEVICE_TYPE_GPU, 0, NULL, &iDeviceNum);
	checkErr(uiStatus, "fail to get the num of device");
	
	std::vector<cl_device_id> svDeviceIDs(iDeviceNum);
	uiStatus = clGetDeviceIDs(svPlatformIDs[0], CL_DEVICE_TYPE_GPU, iDeviceNum, &svDeviceIDs[0], NULL);
	checkErr(uiStatus, "fail to get the ids of devices");

	//创建OpenCL-OpenGL环境
	cl_context_properties clProp[]={
		/*CL_GL_CONTEXT_KHR,
		(cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR,
		(cl_context_properties)wglGetCurrentDC(),*/
		CL_CONTEXT_PLATFORM,
		(cl_context_properties)svPlatformIDs[0],
		0
	};
	cl_context clContext = clCreateContext(clProp, 1, &svDeviceIDs[0], NULL, NULL, &uiStatus);
	checkErr(uiStatus, "fail to creat the GL_CL clContext");

	std::ifstream ifFile("kernel.cl", std::ios_base::binary);

	if ( !ifFile.is_open())
	{
		checkErr(PRINT_INFO, "fail to open kernel file");
		exit(0);
	}
	size_t stFileLen;
	ifFile.seekg(0, std::ios_base::end);
	stFileLen = ifFile.tellg();
	ifFile.seekg(0, std::ios_base::beg);
	std::vector<char> svcData(stFileLen + 1);
	ifFile.read(&svcData[0], stFileLen);
	svcData[stFileLen] = 0;
	const char *ccSource = &svcData[0];

	cl_program clpProgram = clCreateProgramWithSource(clContext, 1,	&ccSource, &stFileLen, &uiStatus);
	checkErr(uiStatus, "fail to create program");
	const char ccOptions[] ="-cl-std=CL1.1 -D T0=64 -D NMAX=8";
	uiStatus = clBuildProgram(clpProgram, 1, &svDeviceIDs[0], ccOptions, 0, 0);
	if ( CL_SUCCESS != uiStatus )
	{
		char cInfo[0x10000];
		checkErr(PRINT_INFO, "fail to build program");
		clGetProgramBuildInfo(clpProgram, svDeviceIDs[0], CL_PROGRAM_BUILD_LOG, 0x10000, cInfo, NULL);
		std::cerr<<cInfo<<std::endl;
		system("pause");
		exit(0);
	}

	cl_kernel clKernel = clCreateKernel(clpProgram, "BitonicSort", &uiStatus);
	checkErr(uiStatus, "fail to create kernel");

	cl_command_queue clQueue = clCreateCommandQueue(clContext, svDeviceIDs[0], NULL, &uiStatus);
	checkErr(uiStatus, "fail to create queue");

	checkErr(PRINT_INFO, "complete the GL_CL clContext");

	


	DWORD sortBeg = GetTickCount();
	//使用OpenCL的方法给三角面片进行排序
	int len = pRes->triangleCandidateSplitPlaneArray.size();
	std::vector<TriangleCandidateSplitPlane> input = pRes->triangleCandidateSplitPlaneArray;
	fillTo2PowerScale(input);

	cl_mem inputMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(TriangleCandidateSplitPlane)*input.size(), &input[0], &uiStatus);
	checkErr(uiStatus, "clCreateBuffer of inputMem error");
	int dir = 1;
	cl_mem dirMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &dir, &uiStatus);
	checkErr(uiStatus, "clCreateBuffer of dirMem error");

	clSetKernelArg(clKernel, 0, sizeof(cl_mem), &inputMem);
	clSetKernelArg(clKernel, 3, sizeof(cl_mem), &dirMem);

	int ceil = input.size();
	for(int i = 2; i <= ceil; i<<=1)
	{
		//std::cout<<"i:"<<i<<std::endl;
		//std::cout<<"j:"<<"\t";
		for (int j = i; j > 1; j>>=1)
		{
			int groupSize = ceil / j;
			int flip = (j==i?1:-1);

			const size_t global = 128;
			const size_t local = 64;


			cl_mem groupSizeMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &groupSize, &uiStatus);
			cl_mem  lengthMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &j, &uiStatus);
			cl_mem flipMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &flip, &uiStatus);
			clSetKernelArg(clKernel, 1, sizeof(cl_mem), &groupSizeMem);
			clSetKernelArg(clKernel, 2, sizeof(cl_mem), &lengthMem);
			clSetKernelArg(clKernel, 4, sizeof(cl_mem), &flipMem);

			uiStatus = clEnqueueNDRangeKernel(clQueue, clKernel, 1, 0, &global, &local, 0, 0, 0);
			checkErr(uiStatus, "clEnqueueNDRangeKernel error");

			clReleaseMemObject(groupSizeMem);
			clReleaseMemObject(flipMem);
			clReleaseMemObject(lengthMem);

			//std::cout<< j << "\t";
		}
		//std::cout<<std::endl;
	}
	DWORD sortEnd = GetTickCount();
	std::cout<<"the sort diff is "<< sortEnd - sortBeg <<std::endl;

	DWORD splitBeg = GetTickCount();

	clFinish(clQueue);
	//按照splitNode的结构来分割inputMem，并生成一个splitNode的数组
	cl_kernel kernelSAHSplit = clCreateKernel(clpProgram, "SAHSplit", 0);
	checkErr(uiStatus, "clCreateKernel of kernelSAHSplit error");

	clSetKernelArg(kernelSAHSplit, 0, sizeof(cl_mem), &inputMem);

	int maxSplitNodeArrayLength = GetNodeArrayMaxLength(input.size());
	SplitNode originSplitNode;
	InitialSplitNode(&originSplitNode);
	std::vector<SplitNode> splitNodeArray(maxSplitNodeArrayLength, originSplitNode);
	SplitNode firstSplitNode;
	firstSplitNode.beg = 0;
	firstSplitNode.end = pRes->triangleInfoArray.size() - 1;
	firstSplitNode.leftChild = -1;
	firstSplitNode.rightChild = -1;
	firstSplitNode.xMax = input[0].xMax;
	firstSplitNode.xMin = input[0].xMin;
	firstSplitNode.yMax = input[0].yMax;
	firstSplitNode.yMin = input[0].yMin;
	firstSplitNode.zMax = input[0].zMax;
	firstSplitNode.zMin = input[0].zMin;
	splitNodeArray[0] = firstSplitNode;

	cl_mem splitNodeArrayMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(SplitNode)*maxSplitNodeArrayLength, &splitNodeArray[0], &uiStatus);
	checkErr(uiStatus, "clCreateBuffer of splitNodeArrayMem error");

	clSetKernelArg(kernelSAHSplit, 1, sizeof(cl_mem), &splitNodeArrayMem);


	int maxLayerLenght = getMin2Power(input.size());
	std::vector<int> randArray(maxLayerLenght);
	for (int i = 0; i< maxLayerLenght; i++)
	{
		randArray[i] = rand() % maxLayerLenght;
	}
	cl_mem randArrayMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int)*maxLayerLenght, &randArray[0], &uiStatus);
	clSetKernelArg(kernelSAHSplit, 4, sizeof(cl_mem), &randArrayMem);

	std::vector<float> randPro(T0*NMAX);
	for (int i = 0; i < randPro.size(); i++)
	{
		randPro[i] = (rand()%10)/10.0;
		//std::cout<<randPro[i]<<std::endl;

	}
	cl_mem randProMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(float)*randPro.size(), &randPro[0], &uiStatus);
	clSetKernelArg(kernelSAHSplit, 5, sizeof(cl_mem), &randProMem);

	cl_mem maxSizeMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &maxSplitNodeArrayLength, &uiStatus);
	clSetKernelArg(kernelSAHSplit, 6, sizeof(cl_mem), &maxSizeMem);

	int depth = 0;
	for(int i = 1; (i < log((float)maxSplitNodeArrayLength) / log(2.0) + 1) && (depth < MAXDEPTH); i++)
	{

		int splitNodeArrayBeg = pow(2.0, i - 1) - 1;
		int splitNodeArrayEnd = pow(2.0, i) - 2;

		size_t layerLength = splitNodeArrayEnd - splitNodeArrayBeg + 1;
		const size_t globalSize = layerLength;
		const size_t localSize = 64;

		cl_mem splitNodeArrayBegMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &splitNodeArrayBeg, &uiStatus);
		checkErr(uiStatus, "clCreateBuffer of splitNodeArrayBegMem error");
		cl_mem splitNodeArrayEndMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &splitNodeArrayEnd, &uiStatus);

		clSetKernelArg(kernelSAHSplit, 2, sizeof(cl_mem), &splitNodeArrayBegMem);
		clSetKernelArg(kernelSAHSplit, 3, sizeof(cl_mem), &splitNodeArrayEndMem);

		uiStatus = clEnqueueNDRangeKernel(clQueue, kernelSAHSplit, 1, 0, &globalSize, &localSize, 0, NULL, NULL);

		clReleaseMemObject(splitNodeArrayBegMem);
		clReleaseMemObject(splitNodeArrayEndMem);
		depth++;
	}

	DWORD splitEnd = GetTickCount();
	std::cout<<"the split time diff is "<<splitEnd - splitBeg<<std::endl;

	/*std::vector<SplitNode> nodeStructureArray(maxSplitNodeArrayLength);
	clEnqueueReadBuffer(clQueue, splitNodeArrayMem, CL_TRUE, 0, sizeof(SplitNode)*nodeStructureArray.size(), &nodeStructureArray[0],0, 0, 0);*/


	clFinish(clQueue);

	//释放资源
	clReleaseKernel(clKernel);
	clReleaseKernel(kernelSAHSplit);
	clReleaseMemObject(randProMem);
	clReleaseMemObject(randArrayMem);
	clReleaseMemObject(splitNodeArrayMem);
	clReleaseMemObject(dirMem);

	//开始绘制图像
	/*glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 


	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);


	

	GLuint uiVertexBuffer;
	glGenBuffers(1, &uiVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uiVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vv3Verts.size()*sizeof(glm::vec3), &vv3Verts[0], GL_STATIC_DRAW);

	GLuint uiUVBuffer;
	glGenBuffers(1, &uiUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uiUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, vv2UVs.size()*sizeof(glm::vec2), &vv2UVs[0], GL_STATIC_DRAW);

	GLuint uiNormalBuffer;
	glGenBuffers(1, &uiNormalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uiNormalBuffer);
	glBufferData(GL_ARRAY_BUFFER, vv3Nor.size()*sizeof(glm::vec3), &vv3Nor[0], GL_STATIC_DRAW);

	GLuint uiProgram = LoadShaders("Shader.vs", "Shader.fs");

	

	GLuint uiMatrixID = glGetUniformLocation(uiProgram, "MVP");
	GLuint uiViewMatrixID = glGetUniformLocation(uiProgram, "V");
	GLuint uiModelMatrixID = glGetUniformLocation(uiProgram, "M");

	GLuint uiText = loadDDS("uvmap.DDS");
	GLuint uiTextID = glGetUniformLocation(uiProgram, "textSampler");
	
	GLuint uiLightID = glGetUniformLocation(uiProgram, "LightPos");


	do 
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(uiProgram);

		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		
		glUniformMatrix4fv(uiMatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(uiModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(uiViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, uiText);
		glUniform1i(uiTextID, 0);

		glm::vec3 v3Light = glm::vec3(4, 4, 0);
		glUniform3f(uiLightID, v3Light.x, v3Light.y, v3Light.z);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, uiVertexBuffer);
		glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			0,
			(void*)0
		);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uiUVBuffer);
		glVertexAttribPointer(
			1,
			2,
			GL_FLOAT,
			GL_FALSE,
			0,
			(void*)0
		);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, uiNormalBuffer);
		glVertexAttribPointer(
			2,
			3,
			GL_FLOAT,
			GL_FALSE,
			0,
			(void*)0
		);

		glDrawArrays(GL_TRIANGLES, 0, vv3Verts.size());

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
*/
	
	//使用OpenCL的方法直接绘制PBO
	cl_kernel ckRayTraceKernel = clCreateKernel(clpProgram, "RayTrace", &uiStatus);
	checkErr(uiStatus, "fail to create kernel");

	cl_mem cmWinWidthMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &iWinWidth, &uiStatus);
	checkErr(uiStatus, "fail to create width buffer");
	cl_mem cmWinHeightMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &iWinHeight, &uiStatus);
	checkErr(uiStatus, "fail to create height buffer");
	
	clSetKernelArg(ckRayTraceKernel, 0, sizeof(cl_mem), &splitNodeArrayMem);
	clSetKernelArg(ckRayTraceKernel, 1, sizeof(cl_mem), &cmWinWidthMem);
	clSetKernelArg(ckRayTraceKernel, 2, sizeof(cl_mem), &cmWinHeightMem);
	
	//设置OpenCL_OpenGL共享内存
	/*GLuint gluPixelBuffer;
	glGenBuffers(1, &gluPixelBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, gluPixelBuffer);
	glBufferData(GL_ARRAY_BUFFER, iWinHeight*iWinWidth*sizeof(glm::vec3), NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	cl_mem	clmPBOMem = clCreateFromGLBuffer(clContext, CL_MEM_READ_WRITE, gluPixelBuffer, &uiStatus);
	checkErr(uiStatus, "fail to create CL-GL shared buffer");
	clSetKernelArg(ckRayTraceKernel, 3, sizeof(cl_mem), &gluPixelBuffer);



	size_t stGlobalSize = {iWinWidth};
	size_t stLocalSize = {256};

	glFinish();
	clEnqueueAcquireGLObjects(clQueue, 1, &clmPBOMem, 0, NULL, NULL);
	uiStatus = clEnqueueNDRangeKernel(clQueue, ckRayTraceKernel, 1, NULL, &stGlobalSize, &stLocalSize, 0, 0, 0);
	clFinish(clQueue);
	clEnqueueReleaseGLObjects(clQueue, 1, &clmPBOMem, 0, NULL, NULL);
	checkErr(uiStatus, "fail to excute kernel");
	clFinish(clQueue);*/
	std::vector<char> pcPixelBufferIn(iWinWidth*iWinHeight*sizeof(glm::vec3));
	cl_mem clmPBOMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE  | CL_MEM_COPY_HOST_PTR, iWinWidth*iWinHeight*sizeof(glm::vec3), &pcPixelBufferIn[0], &uiStatus);
	checkErr(uiStatus, "fail to create PBO buffer");
	clSetKernelArg(ckRayTraceKernel, 3, sizeof(cl_mem), &clmPBOMem);

	std::vector<TriangleInfo> svTriangleInfo = pRes->triangleInfoArray;
	cl_mem clmTriangleInfoMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, sizeof(TriangleInfo)*pRes->triangleInfoArray.size(), &svTriangleInfo[0], &uiStatus);
	clSetKernelArg(ckRayTraceKernel, 4, sizeof(cl_mem), &clmTriangleInfoMem);

	clSetKernelArg(ckRayTraceKernel, 5, sizeof(cl_mem), &inputMem);

	cl_int cliLength = maxSplitNodeArrayLength;
	cl_mem lengthMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_int), &cliLength, &uiStatus);
	clSetKernelArg(ckRayTraceKernel, 6, sizeof(cl_mem), &lengthMem);

	size_t stGlobalSize = {iWinWidth};
	size_t stLocalSize = {256};
	uiStatus = clEnqueueNDRangeKernel(clQueue, ckRayTraceKernel, 1, NULL, &stGlobalSize, &stLocalSize, 0, 0, 0);
	clFinish(clQueue);

	std::vector<char> pcPixelBufferOut(iWinWidth*iWinHeight);
	clEnqueueReadBuffer(clQueue, clmPBOMem, CL_FALSE, 0,iWinWidth*iWinHeight*sizeof(glm::vec3), &pcPixelBufferOut[0], NULL, NULL, NULL);
	clFinish(clQueue);

	do 
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glRasterPos2f(-1, -1);
		glDrawPixels(iWinWidth, iWinHeight, GL_RGB, GL_UNSIGNED_BYTE, &pcPixelBufferOut[0]);
	} while (1);


	//释放资源
	clReleaseProgram(clpProgram);
	clReleaseMemObject(inputMem);
	clReleaseMemObject(splitNodeArrayMem);
	clReleaseCommandQueue(clQueue);



	
}


