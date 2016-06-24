#include <stdlib.h>
#include <iostream>

#include <CL/cl.h>
#include <vector>
#include <fstream>

#include "cusLib.h"


//用来记录一个结点对应三角面片跟AABB的结构体

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
		system("pause");
		exit(0);
	}

	std::cout<<"the number of valid platforms is :"<<platformNum<<std::endl;


	std::vector<cl_platform_id>  platformIds(platformNum);
	status = clGetPlatformIDs(platformNum,&platformIds[0],NULL);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clGetPlatformsIDs error!"<<std::endl;
		system("pause");
		exit(0);
	}

	cl_uint deviceNum;
	status = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 0, 0, &deviceNum);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clGetDeviceIDs error!"<<std::endl;
		system("pause");
		exit(0);
	}

	std::cout<<"the num of this platform's devices is :"<<deviceNum<<std::endl;

	std::vector<cl_device_id> deviceIds(deviceNum);
	status = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 1, &deviceIds[0], NULL);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clGetDeviceIDs error!"<<std::endl;
		system("pause");
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
		system("pause");
		exit(0);
	}

	system("echo ====================================================");	
	std::cout<<"The context of OpenCL is build successfully!"<<std::endl;
	system("echo ====================================================");
	system("pause");

	//载入kernel项目
	std::ifstream file("kernel.cl",std::ios_base::binary);

	if (!file.is_open())
	{
		std::cout<<"Error in the open of kernel file!"<<std::endl;
		system("pause");
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
	const char options[] ="";
	status = clBuildProgram(program, 1, &deviceIds[0], options, 0, 0);

	if (CL_SUCCESS != status)
	{
		std::cout<<"clBuildProgram error"<<std::endl;
		clGetProgramBuildInfo(program,deviceIds[0],CL_PROGRAM_BUILD_LOG,0x10000,tbuf,NULL);
		std::cout<<tbuf<<std::endl;
		system("pause");
		exit(0);
	}

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
	
	
	
	system("echo the array of CandidatePlane has been prepared");
	system("echo ====================================================");
	system("pause");

	//将各子节点的三角面片以及AABB数组合并
	DrawableInfo* all = new DrawableInfo;

	di2->triangleInfoArray.insert(di2->triangleInfoArray.end(),di1->triangleInfoArray.begin(),di1->triangleInfoArray.end());
	di2->triangleCandidateSplitPlane.insert(di2->triangleCandidateSplitPlane.end(),di1->triangleCandidateSplitPlane.begin(),di1->triangleCandidateSplitPlane.end());
	all->triangleInfoArray = di2->triangleInfoArray;
	all->triangleCandidateSplitPlane = di2->triangleCandidateSplitPlane;

	auto it = all->triangleInfoArray.begin();
	for(; it<all->triangleInfoArray.end(); it++)
	{
		osg::ref_ptr<osg::Geode> geodeTmp = new osg::Geode;
		createGeode((*it), geodeTmp); 
		root->addChild(geodeTmp);

	}
	
	//root->addChild(group);
	//展示整个场景
	osg::ref_ptr<osgViewer::Viewer> viewer =  new osgViewer::Viewer;
	viewer->setUpViewInWindow(500,200,1000,800);
	viewer->setSceneData(root.get());
	viewer->run();
	system("echo in the end");
	system("pause");
}