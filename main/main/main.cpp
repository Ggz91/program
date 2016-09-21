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




int ciWidth= 1024;
int ciHeight = 768;

const char* ccpFileName = "bunny_new2.obj";
//const char* ccpFileName = "suzanne.obj";


void main(int argc, char** argv)
{
	
	//创建Opengl显示窗口
	if( !glfwInit())
	{
		checkErr(PRINT_INFO, "fail to init glfw!");
		exit(0);
	}

	//glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __REALTIME__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	
	

	window = glfwCreateWindow(ciWidth, ciHeight, " OpenCL-OpenGL-Program", NULL, NULL);
	if ( window == NULL)
	{
		checkErr(PRINT_INFO, "fail to open glfw window!");
		glfwTerminate();
		exit(0);
	}
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if ( glewInit() != GLEW_OK ) {
		checkErr(PRINT_INFO, "fail to init glew");
		glfwTerminate();
		exit(0);
	}

	


	


	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwPollEvents();
	glfwSetCursorPos(window, ciWidth/2, ciHeight/2);

	//读取obj文件的数据
	std::vector<glm::vec3> vv3Verts;
	std::vector<glm::vec2> vv2UVs;
	std::vector<glm::vec3> vv3Nor;

	bool bRes = loadOBJ( ccpFileName, vv3Verts, vv2UVs, vv3Nor);

	DrawableInfo* pRes = new DrawableInfo;
	pRes = getTriangles(&vv3Verts[0], &vv3Nor[0], vv3Verts.size());

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
#ifdef __REALTIME__
		CL_GL_CONTEXT_KHR,
		(cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR,
		(cl_context_properties)wglGetCurrentDC(),
#endif
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
	const char ccOptions[] ="-cl-std=CL1.1 ";//-DT0  -DNMAX -DDEC_SPEED -DMAXITER -w ";
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

	


	DWORD dwBuildBeg = GetTickCount();
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

	cl_mem maxSizeMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &maxSplitNodeArrayLength, &uiStatus);
	clSetKernelArg(kernelSAHSplit, 6, sizeof(cl_mem), &maxSizeMem);

#ifdef __OPT__
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

	clFinish(clQueue);

	DWORD dwBuildEnd = GetTickCount();
	std::cout<<"the time cost of building Opt-KD-tree is "<<dwBuildEnd - dwBuildBeg<<std::endl;
#endif

	//普通的kd-tree建树方法
#ifdef __COMM__
	cl_kernel ckCommSplitKernel = clCreateKernel(clpProgram, "CommSAHSplit", &uiStatus);
	checkErr(uiStatus, "fail to create CommSplitKernel");
	
	cl_mem commSplitNodeArrayMem = clCreateBuffer(clContext, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE, sizeof(SplitNode)*maxSplitNodeArrayLength, &splitNodeArray[0], &uiStatus);

	clSetKernelArg(ckCommSplitKernel, 0, sizeof(cl_mem), &inputMem);
	clSetKernelArg(ckCommSplitKernel, 1, sizeof(cl_mem), &commSplitNodeArrayMem);
	clSetKernelArg(ckCommSplitKernel, 4, sizeof(cl_mem), &maxSizeMem);

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

		clSetKernelArg(ckCommSplitKernel, 2, sizeof(cl_mem), &splitNodeArrayBegMem);
		clSetKernelArg(ckCommSplitKernel, 3, sizeof(cl_mem), &splitNodeArrayEndMem);

		uiStatus = clEnqueueNDRangeKernel(clQueue, ckCommSplitKernel, 1, 0, &globalSize, &localSize, 0, NULL, NULL);

		clReleaseMemObject(splitNodeArrayBegMem);
		clReleaseMemObject(splitNodeArrayEndMem);
		depth++;
	}

	clFinish(clQueue);

	DWORD dwCommBuildEnd = GetTickCount();
	std::cout<<"the time cost of building Comm-KD-tree is "<<dwCommBuildEnd - dwBuildBeg<<std::endl;
#endif
	//释放资源
	clReleaseKernel(clKernel);
	clReleaseKernel(kernelSAHSplit);
#ifdef __OPT__
	clReleaseMemObject(randProMem);
	clReleaseMemObject(randArrayMem);
#endif
	//clReleaseMemObject(splitNodeArrayMem);
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

	cl_mem cmWinWidthMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &ciWidth, &uiStatus);
	checkErr(uiStatus, "fail to create width buffer");
	cl_mem cmWinHeightMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &ciHeight, &uiStatus);
	checkErr(uiStatus, "fail to create height buffer");
	
#ifdef __OPT__
	clSetKernelArg(ckRayTraceKernel, 0, sizeof(cl_mem), &splitNodeArrayMem);
#endif
#ifdef __COMM__
	clSetKernelArg(ckRayTraceKernel, 0, sizeof(cl_mem), &commSplitNodeArrayMem);
#endif
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
#ifdef __REALTIME__
	//渲染到纹理，使用OpenCL与OpenGL共享的texture
	GLuint uiFbo;
	glGenFramebuffers(1, &uiFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, uiFbo);

	GLuint uiRenderedTexture;
	glGenTextures(1, &uiRenderedTexture);
	glBindTexture(GL_TEXTURE_2D, uiRenderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ciWidth, ciHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	GLuint uiPBO;
	glGenBuffers(1, &uiPBO);
	glBindBuffer(GL_ARRAY_BUFFER, uiPBO);
	glBufferData(GL_ARRAY_BUFFER, ciWidth*ciHeight*4*sizeof(unsigned char), 0, GL_STREAM_DRAW);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, uiRenderedTexture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);



	cl_mem clmPBOMem = clCreateFromGLBuffer(clContext, CL_MEM_WRITE_ONLY, uiPBO, &uiStatus);
	checkErr(uiStatus, "fail to create clmPBOMem");
	clSetKernelArg(ckRayTraceKernel, 3, sizeof(cl_mem), &clmPBOMem);

	

#endif
	
#ifdef __NREALTIME__
	std::vector<cl_uchar> pcPixelBufferIn(ciWidth*ciHeight*3);
	cl_mem clmPBOMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE  | CL_MEM_COPY_HOST_PTR, 3*ciWidth*ciHeight*sizeof(cl_uchar), &pcPixelBufferIn[0], &uiStatus);
	checkErr(uiStatus, "fail to create PBO buffer");
	clSetKernelArg(ckRayTraceKernel, 3, sizeof(cl_mem), &clmPBOMem);
#endif
	

	std::vector<TriangleInfo> svTriangleInfo = pRes->triangleInfoArray;
	cl_mem clmTriangleInfoMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY  | CL_MEM_COPY_HOST_PTR, sizeof(TriangleInfo)*pRes->triangleInfoArray.size(), &svTriangleInfo[0], &uiStatus);
	clSetKernelArg(ckRayTraceKernel, 4, sizeof(cl_mem), &clmTriangleInfoMem);

	clSetKernelArg(ckRayTraceKernel, 5, sizeof(cl_mem), &inputMem);

	cl_int cliLength = maxSplitNodeArrayLength;
	cl_mem lengthMem = clCreateBuffer(clContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(cl_int), &cliLength, &uiStatus);
	clSetKernelArg(ckRayTraceKernel, 6, sizeof(cl_mem), &lengthMem);

	/*double dXpos, dYpos;
	glfwGetCursorPos(window, &dXpos, &dYpos);

	cl_mem xposMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(double), &dXpos, &uiStatus);
	clSetKernelArg(ckRayTraceKernel, 7, sizeof(cl_mem), &xposMem);
	cl_mem yposMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(double), &dYpos, &uiStatus);
	clSetKernelArg(ckRayTraceKernel, 8, sizeof(cl_mem), &yposMem);*/

#ifdef __ONEDIMCAL__
	size_t stGlobalSize = {ciWidth};
	size_t stLocalSize = {256};
	uiStatus = clEnqueueNDRangeKernel(clQueue, ckRayTraceKernel, 1, NULL, &stGlobalSize, &stLocalSize, 0, 0, 0);
#endif

#ifdef __TWODIMCAL__
	size_t stGlobalSize[2] = {ciWidth, ciHeight};
	size_t stLocalSize[2] = {16,16};
#ifdef __NREALTIME__
	uiStatus = clEnqueueNDRangeKernel(clQueue, ckRayTraceKernel, 2, NULL, &stGlobalSize[0], &stLocalSize[0], 0, 0, 0);
	clFinish(clQueue);
#endif
	
//#ifdef __REALTIME__
//	glFinish();
//	clEnqueueAcquireGLObjects(clQueue, 1, &clmPBOMem, 0, NULL, NULL);
//	uiStatus = clEnqueueNDRangeKernel(clQueue, ckRayTraceKernel, 2, NULL, &stGlobalSize[0], &stLocalSize[0], 0, 0, 0);
//	clFinish(clQueue);
//	clEnqueueReleaseGLObjects(clQueue, 1, &clmPBOMem, 0, NULL, NULL);
//	checkErr(uiStatus, "fail to excute kernel");
//	clFinish(clQueue);
//#endif
#endif
//	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uiPBO);
//	glBindTexture(GL_TEXTURE_2D, uiRenderedTexture);
//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ciWidth, ciHeight, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
//
//	DWORD dwRenderEnd = GetTickCount();
//	std::cout<<" the PBO is completed"<<std::endl;
#ifdef __COMM__
	std::cout<<" the comm-render time is "<< dwRenderEnd - dwRenderBeg <<std::endl;
#endif
#ifdef __OPT__
	//std::cout<<" the opt-render time is "<< dwRenderEnd - dwRenderBeg <<std::endl;
#endif

#ifdef __NREALTIME__
	std::vector<cl_uchar> pcPixelBufferOut(ciWidth*ciHeight*3, 100);
	clEnqueueReadBuffer(clQueue, clmPBOMem, CL_TRUE, 0, 3*ciWidth*ciHeight*sizeof(cl_uchar), &pcPixelBufferOut[0], NULL, NULL, NULL);
	clFinish(clQueue);


	std::freopen("PBO.txt", "w", stdout);

	for(int i=0; i<ciWidth*ciHeight*3; i++)
	{
		std::cout<<pcPixelBufferOut[i]<<" ";
		if ( i%3 == 2)
		{
			std::cout<<" "<<std::endl;
		}
	}

	
	

	

	do 
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glRasterPos2f(-1, -1);
		glDrawPixels(ciWidth, ciHeight, GL_RGB, GL_UNSIGNED_BYTE, &pcPixelBufferOut[0]);
		glfwSwapBuffers(window);
	} while (1);

#endif
	

#ifdef __REALTIME__ 
	static const GLfloat cfQuadVertBuffer[] = { 
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};
	
	static const GLfloat UV[] = { 
		0.0f, 0.0f, 
		1.0f, 0.0f, 
		0.0f,  1.0f, 
		0.0f,  1.0f, 
		1.0f, 0.0f, 
		1.0f,  1.0f, 
	};

	GLuint guVertArrayID;
	glGenVertexArrays(1, &guVertArrayID);
	glBindVertexArray(guVertArrayID);

	GLuint guQuadVert;
	glGenBuffers(1, &guQuadVert);
	glBindBuffer(GL_ARRAY_BUFFER, guQuadVert);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cfQuadVertBuffer), cfQuadVertBuffer, GL_STATIC_DRAW);

	GLuint guUV;
	glGenBuffers(1, &guUV);
	glBindBuffer(GL_ARRAY_BUFFER, guUV);
	glBufferData(GL_ARRAY_BUFFER, sizeof(UV), UV, GL_STATIC_DRAW);
	GLuint secondProgram = LoadShaders("secondVert.vs", "secondFrag.fs");
	GLuint texID = glGetUniformLocation(secondProgram, "renderedTexture");
	glClearColor(153.0f, 51.0f, 250.0f, 0.0f);


	cl_mem xposMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(double), NULL, &uiStatus);
	checkErr(uiStatus, "fail to create buffer!");
	clSetKernelArg(ckRayTraceKernel, 7, sizeof(cl_mem), &xposMem);
	cl_mem yposMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, sizeof(double), NULL, &uiStatus);
	checkErr(uiStatus, "fail to create buffer!");
	clSetKernelArg(ckRayTraceKernel, 8, sizeof(cl_mem), &yposMem);

	double  *dXpos = (double* ) clEnqueueMapBuffer(clQueue, xposMem, CL_FALSE, CL_MAP_WRITE, 0, sizeof(double), 0, NULL, NULL, &uiStatus);
	checkErr(uiStatus, "fail to map buffer!");
	double  *dYpos = (double* ) clEnqueueMapBuffer(clQueue, yposMem, CL_FALSE, CL_MAP_WRITE, 0, sizeof(double), 0, NULL, NULL, &uiStatus);
	glfwGetCursorPos(window, dYpos, dXpos);
	

	do 
	{
		DWORD dwRenderBeg = GetTickCount();
		//double dXpos, dYpos;
		glfwGetCursorPos(window, dYpos, dXpos);
		//glfwGetCursorPos(window, &dYpos, &dXpos);
		
		
		*dYpos = -1*(*dYpos);

		//dYpos = -1*dYpos;

		//cl_mem xposMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(double), &dXpos, &uiStatus);
		//checkErr(uiStatus, "fail to create buffer!");
		//clSetKernelArg(ckRayTraceKernel, 7, sizeof(cl_mem), &xposMem);
		//cl_mem yposMem = clCreateBuffer(clContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(double), &dYpos, &uiStatus);
		//checkErr(uiStatus, "fail to create buffer!");
		//clSetKernelArg(ckRayTraceKernel, 8, sizeof(cl_mem), &yposMem);
		

#ifdef __REALTIME__
		glFinish();
		clEnqueueAcquireGLObjects(clQueue, 1, &clmPBOMem, 0, NULL, NULL);
		uiStatus = clEnqueueNDRangeKernel(clQueue, ckRayTraceKernel, 2, NULL, &stGlobalSize[0], &stLocalSize[0], 0, 0, 0);
		clFinish(clQueue);
		clEnqueueReleaseGLObjects(clQueue, 1, &clmPBOMem, 0, NULL, NULL);
		checkErr(uiStatus, "fail to excute kernel");
		clFinish(clQueue);
#endif
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, uiPBO);
		glBindTexture(GL_TEXTURE_2D, uiRenderedTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ciWidth, ciHeight, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, ciWidth, ciHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(secondProgram);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, uiRenderedTexture);
		glUniform1i(texID, 0);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, guQuadVert);
		glVertexAttribPointer(
			0,
			3,
			GL_FLOAT,
			GL_FALSE,
			0,
			(void*)0
			);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, guUV);
		glVertexAttribPointer(
			1,
			2,
			GL_FLOAT,
			GL_FALSE,
			0,
			(void*)0
			);

		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		
		glfwSwapBuffers(window);
		glfwPollEvents();

		DWORD dwRenderEnd = GetTickCount();
		std::cout<<"the render time is "<< dwRenderEnd - dwRenderBeg<<std::endl;
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS);
	
#endif	
	

	//释放资源
	clReleaseMemObject(xposMem);
	clReleaseMemObject(yposMem);
	clReleaseProgram(clpProgram);
	clReleaseMemObject(inputMem);
#ifdef __OPT__
	clReleaseMemObject(splitNodeArrayMem);
#endif
#ifdef __COMM__
	clReleaseMemObject(commSplitNodeArrayMem);
#endif
	clReleaseMemObject(cmWinHeightMem);
	clReleaseCommandQueue(clQueue);

	
}


