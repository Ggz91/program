#include <stdlib.h>
#include <iostream>

#include <CL/cl.h>
#include <vector>
#include <fstream>
#include <iomanip>
#include <Windows.h>
#include "cusLib.h"

#define T0 64
#define NMAX 8
#define MAXDEPTH 20

void main(int argc,char** argv[])
{
	//设置OpenCL环境
	cl_int status;
	cl_uint platformNum;
	char tbuf[0x10000];

	status = clGetPlatformIDs(0,NULL,&platformNum);
	if (CL_SUCCESS != status)
	{
		std::cout<<"clGetPlatformsIDs error!"<<std::endl;
		//system("pause");
		exit(0);
	}

	std::cout<<"the number of valid platforms is :"<<platformNum<<std::endl;


	std::vector<cl_platform_id>  platformIds(platformNum);
	status = clGetPlatformIDs(platformNum,&platformIds[0],NULL);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clGetPlatformsIDs error!"<<std::endl;
		//system("pause");
		exit(0);
	}

	cl_uint deviceNum;
	status = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 0, 0, &deviceNum);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clGetDeviceIDs error!"<<std::endl;
		//system("pause");
		exit(0);
	}

	std::cout<<"the num of this platform's devices is :"<<deviceNum<<std::endl;

	std::vector<cl_device_id> deviceIds(deviceNum);
	status = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 1, &deviceIds[0], NULL);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clGetDeviceIDs error!"<<std::endl;
		//system("pause");
		exit(0);
	}


	cl_context_properties prop[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platformIds[0], 0};
	cl_context context = clCreateContextFromType(prop, CL_DEVICE_TYPE_GPU, NULL, NULL, &status);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clCreateContextFromType error!"<<std::endl;
		cl_context_info contextInfo;
		clGetContextInfo(context,contextInfo,0x10000,tbuf,NULL);
		std::cout<<tbuf<<std::endl;
		//system("pause");
		exit(0);
	}

	//system("echo ====================================================");	
	//std::cout<<"The context of OpenCL is build successfully!"<<std::endl;
	//system("echo ====================================================");
	//system("pause");

	//载入kernel项目
	std::ifstream file("kernel.cl",std::ios_base::binary);

	if (!file.is_open())
	{
		std::cout<<"Error in the open of kernel file!"<<std::endl;
		//system("pause");
		exit(0);
	}

	file.seekg(0,std::ios_base::end);
	size_t length = file.tellg();
	file.seekg(0,std::ios_base::beg);
	std::vector<char> data(length+1);
	file.read(&data[0],length);
	data[length] = 0;
	const char* source = &data[0];

	

	cl_program program = clCreateProgramWithSource(context,1,&source,&length,&status);
	//const char options[] = "-I F:\\learning\\program\\main\\main\\cusLib.h";
	const char options[] ="-cl-std=CL1.1 -D T0=64 -D NMAX=8";
	status = clBuildProgram(program, 1, &deviceIds[0], options, 0, 0);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clBuildProgram error"<<std::endl;
		clGetProgramBuildInfo(program,deviceIds[0],CL_PROGRAM_BUILD_LOG,0x10000,tbuf,NULL);
		std::cout<<tbuf<<std::endl;
		//system("pause");
		exit(0);
	}

	cl_kernel kernel = clCreateKernel(program, "BitonicSort", &status);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clCreateKernel error"<<std::endl;
		//system("pause");
		exit(0);
	}

	cl_command_queue queue = clCreateCommandQueue(context, deviceIds[0], NULL, &status);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clCreateCommandQueue error!"<<std::endl;
		//system("pause");
		exit(0);
	}

	//system("echo ====================================================");	
	//system("echo the programs, kernels and command queues are built successfully!");
	//system("echo ====================================================");	
	//system("pause");

	//载入两个实验结点
	osg::ref_ptr<osg::Node> node1 = new osg::Node;
	osg::ref_ptr<osg::Node> node2 = new osg::Node;

	node1 = osgDB::readNodeFile("cow.osg");
	node2 = osgDB::readNodeFile("cessna.osg");

	osg::ref_ptr<osg::MatrixTransform> axes1 = new osg::MatrixTransform;
	osg::ref_ptr<osg::MatrixTransform> axes2 = new osg::MatrixTransform;

	axes1->setMatrix(osg::Matrix::translate(5,20,20));
	//axes1->setMatrix(osg::Matrix::translate(5,20,20)*osg::Matrix::rotate(osg::DegreesToRadians(30.0), 0, 0, 1));
	axes2->setMatrix(osg::Matrix::translate(5,0,0));

	axes1->addChild(node1);
	axes2->addChild(node2);

	osg::ref_ptr<osg::Group> group = new osg::Group;

	group->addChild(axes1);
	group->addChild(axes2);

	osg::ref_ptr<osg::Group> root =  new osg::Group;
	//root->addChild(group);
	
	//测试代码区
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode = node1->asGroup()->getChild(0)->asGeode();

	//drawableInfo* di = getTriangles(*root->getChild(0)->asGroup()->getChild(0)->asGroup()->getChild(0)->asGroup()->getChild(0)->asGeode()->getDrawable(0),root->getChild(0)->asGroup()->asGeode());
	//获取子节点的三角面片及其AABB，并将三角面片以及AABB的坐标转换为世界坐标系
	int ID = 0;//三角面片的ID，整个场景一起命名
	osg::Matrixf* mat1 = getWorldCoords(group->getChild(0));
	osg::Matrixf* mat2 = getWorldCoords(group->getChild(1));
 	DrawableInfo* di1 = getTriangles(*node1->asGroup()->getChild(0)->asGeode()->getDrawable(0), mat1, ID);
	DrawableInfo* di2 = getTriangles(*node2->asGroup()->getChild(0)->asGeode()->getDrawable(0), mat2, ID);
	
	
	
	

	//将各子节点的三角面片以及AABB数组合并
	DrawableInfo* all = new DrawableInfo;

	di2->triangleInfoArray.insert(di2->triangleInfoArray.end(),di1->triangleInfoArray.begin(),di1->triangleInfoArray.end());
	di2->triangleCandidateSplitPlaneArray.insert(di2->triangleCandidateSplitPlaneArray.end(),di1->triangleCandidateSplitPlaneArray.begin(),di1->triangleCandidateSplitPlaneArray.end());
	all->triangleInfoArray = di2->triangleInfoArray;
	all->triangleCandidateSplitPlaneArray = di2->triangleCandidateSplitPlaneArray;

	osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
	texture->setImage( osgDB::readImageFile("texImage.jpg") );
	texture->setFilter( osg::Texture::MIN_FILTER, osg::Texture::LINEAR );
	texture->setFilter( osg::Texture::MAG_FILTER, osg::Texture::LINEAR );
	texture->setWrap( osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER );
	texture->setWrap( osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER );
	texture->setBorderColor( osg::Vec4(1.0, 1.0, 0.0, 1.0) );

	/*osg::ref_ptr<osg::Group> node_tmp = new osg::Group;
	auto it = all->triangleInfoArray.begin();
	for(; it<all->triangleInfoArray.end(); it++)
	{
	osg::ref_ptr<osg::Geode> geodeTmp = new osg::Geode;
	createGeode((*it), geodeTmp, texture); 
	node_tmp->addChild(geodeTmp);
	}
	*/
	//system("echo the array of CandidatePlane has been prepared");
	//system("echo ====================================================");
	//system("pause");

	DWORD sortBeg = GetTickCount();


	//使用OpenCL的方法给三角面片进行排序
	int len = all->triangleCandidateSplitPlaneArray.size();
	std::vector<TriangleCandidateSplitPlane> input = all->triangleCandidateSplitPlaneArray;
	fillTo2PowerScale(input);

	cl_mem inputMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(TriangleCandidateSplitPlane)*input.size(), &input[0], &status);
	checkErr(status, "clCreateBuffer of inputMem error");
	int dir = 1;
	cl_mem dirMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &dir, &status);
	checkErr(status, "clCreateBuffer of dirMem error");

	clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputMem);
	clSetKernelArg(kernel, 3, sizeof(cl_mem), &dirMem);

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


			cl_mem groupSizeMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &groupSize, &status);
			cl_mem  lengthMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &j, &status);
			cl_mem flipMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &flip, &status);
			clSetKernelArg(kernel, 1, sizeof(cl_mem), &groupSizeMem);
			clSetKernelArg(kernel, 2, sizeof(cl_mem), &lengthMem);
			clSetKernelArg(kernel, 4, sizeof(cl_mem), &flipMem);

			status = clEnqueueNDRangeKernel(queue, kernel, 1, 0, &global, &local, 0, 0, 0);
			checkErr(status, "clEnqueueNDRangeKernel error");

			clReleaseMemObject(groupSizeMem);
			clReleaseMemObject(flipMem);
			clReleaseMemObject(lengthMem);

			//std::cout<< j << "\t";
		}
		//std::cout<<std::endl;
	}
	DWORD sortEnd = GetTickCount();
	std::cout<<"the sort diff is "<< sortEnd - sortBeg <<std::endl;

	/*std::vector<TriangleCandidateSplitPlane> res(len);
	clEnqueueReadBuffer(queue, inputMem, CL_FALSE, 0,sizeof(TriangleCandidateSplitPlane)*len, &res[0], 0, 0, 0);
	for (auto it = res.begin(); it < res.end(); it ++)
	{
		std::cout<<std::setprecision(8) <<it->xCandidateSplitPlane<<"\t";
	}*/

	//system("echo ====================================================");	
	//system("echo the BitonicSort has finished!");
	//system("echo begin to print the reslut!");
	//system("echo ====================================================");
	//system("pause");


	DWORD splitBeg = GetTickCount();

	//按照splitNode的结构来分割inputMem，并生成一个splitNode的数组
	cl_kernel kernelSAHSplit = clCreateKernel(program, "SAHSplit", 0);
	checkErr(status, "clCreateKernel of kernelSAHSplit error");
	
	clSetKernelArg(kernelSAHSplit, 0, sizeof(cl_mem), &inputMem);

	int maxSplitNodeArrayLength = GetNodeArrayMaxLength(input.size());
	SplitNode originSplitNode;
	InitialSplitNode(&originSplitNode);
	std::vector<SplitNode> splitNodeArray(maxSplitNodeArrayLength, originSplitNode);
	SplitNode firstSplitNode;
	firstSplitNode.beg = 0;
	firstSplitNode.end = all->triangleInfoArray.size() - 1;
	firstSplitNode.leftChild = -1;
	firstSplitNode.rightChild = -1;
	firstSplitNode.xMax = input[0].xMax;
	firstSplitNode.xMin = input[0].xMin;
	firstSplitNode.yMax = input[0].yMax;
	firstSplitNode.yMin = input[0].yMin;
	firstSplitNode.zMax = input[0].zMax;
	firstSplitNode.zMin = input[0].zMin;
	splitNodeArray[0] = firstSplitNode;

	cl_mem splitNodeArrayMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(SplitNode)*maxSplitNodeArrayLength, &splitNodeArray[0], &status);
	checkErr(status, "clCreateBuffer of splitNodeArrayMem error");
	
	clSetKernelArg(kernelSAHSplit, 1, sizeof(cl_mem), &splitNodeArrayMem);


	int maxLayerLenght = getMin2Power(input.size());
	std::vector<int> randArray(maxLayerLenght);
	for (int i = 0; i< maxLayerLenght; i++)
	{
		randArray[i] = rand() % maxLayerLenght;
	}
	cl_mem randArrayMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int)*maxLayerLenght, &randArray[0], &status);
	clSetKernelArg(kernelSAHSplit, 4, sizeof(cl_mem), &randArrayMem);

	std::vector<float> randPro(T0*NMAX);
	for (int i = 0; i < randPro.size(); i++)
	{
		randPro[i] = (rand()%10)/10.0;
		//std::cout<<randPro[i]<<std::endl;

	}
	cl_mem randProMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(float)*randPro.size(), &randPro[0], &status);
	clSetKernelArg(kernelSAHSplit, 5, sizeof(cl_mem), &randProMem);

	cl_mem maxSizeMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &maxSplitNodeArrayLength, &status);
	clSetKernelArg(kernelSAHSplit, 6, sizeof(cl_mem), &maxSizeMem);
	
	int depth = 0;
	for(int i = 1; (i < log((float)maxSplitNodeArrayLength) / log(2.0) + 1) && (depth < MAXDEPTH); i++)
	{
		
		int splitNodeArrayBeg = pow(2.0, i - 1) - 1;
		int splitNodeArrayEnd = pow(2.0, i) - 2;
		
		size_t layerLength = splitNodeArrayEnd - splitNodeArrayBeg + 1;
		const size_t globalSize = layerLength;
		const size_t localSize = 64;

		cl_mem splitNodeArrayBegMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &splitNodeArrayBeg, &status);
		checkErr(status, "clCreateBuffer of splitNodeArrayBegMem error");
		cl_mem splitNodeArrayEndMem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int), &splitNodeArrayEnd, &status);

		clSetKernelArg(kernelSAHSplit, 2, sizeof(cl_mem), &splitNodeArrayBegMem);
		clSetKernelArg(kernelSAHSplit, 3, sizeof(cl_mem), &splitNodeArrayEndMem);

		status = clEnqueueNDRangeKernel(queue, kernelSAHSplit, 1, 0, &globalSize, &localSize, 0, NULL, NULL);

		clReleaseMemObject(splitNodeArrayBegMem);
		clReleaseMemObject(splitNodeArrayEndMem);
		depth++;
	}

	DWORD splitEnd = GetTickCount();
	std::cout<<"the split time diff is "<<splitEnd - splitBeg<<std::endl;

	std::vector<SplitNode> nodeStructureArray(maxSplitNodeArrayLength);
	clEnqueueReadBuffer(queue, splitNodeArrayMem, CL_TRUE, 0, sizeof(SplitNode)*nodeStructureArray.size(), &nodeStructureArray[0],0, 0, 0);
	
	


	//释放资源
	clReleaseKernel(kernel);
	clReleaseKernel(kernelSAHSplit);
	clReleaseProgram(program);
	clReleaseMemObject(inputMem);
	clReleaseMemObject(randProMem);
	clReleaseMemObject(randArrayMem);
	clReleaseMemObject(splitNodeArrayMem);
	clReleaseMemObject(dirMem);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);

	
	//system("echo ====================================================");	
	//system("echo the splitNodeArray has finished!");
	//system("echo ====================================================");
	//system("pause");
	
	
	
	//分配面片
	osg::ref_ptr<osg::Node> resNode = DistributeTrianglesNode(&nodeStructureArray[0], nodeStructureArray,all, texture);
	root->addChild(createLight(resNode.get()));
	
	
	//展示整个场景
	//root->addChild(node_tmp);
	osg::ref_ptr<osgViewer::Viewer> viewer =  new osgViewer::Viewer;
	viewer->setUpViewInWindow(500,200,1000,800);
	viewer->setSceneData(root.get());
	//viewer->run();

	osg::Stats* stats = viewer->getStats();
	stats->collectStats("frame_rate", true);
	viewer->run();
	
	double renderTime = 0;
	double num = 0;
	
	stats->getAveragedAttribute("Frame rate", renderTime, true);

	std::cout<<"the render time is "<<std::setiosflags(std::ios::fixed)<<renderTime<<std::endl;

	osg::ref_ptr<osgViewer::Viewer> viewerSe =  new osgViewer::Viewer;
	viewerSe->setUpViewInWindow(500,200,1000,800);

	osg::ref_ptr<osg::Group> rootSe = new osg::Group;
	rootSe->addChild(axes1);
	rootSe->addChild(axes2);
	viewerSe->setSceneData(rootSe.get());

	osg::Stats* statsSe = viewerSe->getCamera()->getStats();
	statsSe->collectStats("scene", true);
	viewerSe->run();
	statsSe->getAveragedAttribute("Visible number of GL_POINTS", renderTime, true);
	std::cout<<"the render time is "<<std::setiosflags(std::ios::fixed)<<renderTime<<std::endl;
	
	system("echo in the end");
	system("pause");
}