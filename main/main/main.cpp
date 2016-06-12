#include <stdlib.h>
#include <iostream>

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osg/Node>
#include <osg/Geode>
#include <osg/Group>

#include <osg/MatrixTransform>
#include <osg/TriangleFunctor>

struct GetVertex
{
	void operator() (const osg::Vec3& v1,const osg::Vec3& v2,const osg::Vec3& v3, bool) const 
	{
		vertexList->push_back(v1);
		vertexList->push_back(v2);
		vertexList->push_back(v3);
	}

	osg::Vec3Array* vertexList;

};

void getTriangles(osg::Drawable& drawable)
{
	osg::TriangleFunctor<GetVertex> tf;
	tf.vertexList=new osg::Vec3Array;

	drawable.accept(tf);
	int i = 1;
	for(osg::Vec3Array::iterator itr=tf.vertexList->begin();
		itr!=tf.vertexList->end();
		itr++)
	{
		osg::Vec3 vertex=*itr;
		//std::cout<<vertex<<std::endl;
		std::cout<<itr->x()<<" "<<itr->y()<<" "<<itr->z()<<std::endl;
		//std::cout<<std::endl;

		if (i == 3)
		{
			std::cout<<std::endl;
			i = 0;
		}

		i++;
	}

	std::cout<<std::endl;
}


void main(int argc,char** argv[])
{
	osg::ref_ptr<osg::Node> node1 = new osg::Node;
	osg::ref_ptr<osg::Node> node2 = new osg::Node;

	node1 = osgDB::readNodeFile("cow.osg");
	node2 = osgDB::readNodeFile("cessna.osg");

	osg::ref_ptr<osg::MatrixTransform> axes1 = new osg::MatrixTransform;
	osg::ref_ptr<osg::MatrixTransform> axes2 = new osg::MatrixTransform;

	axes1->setMatrix(osg::Matrix::translate(5,10,10));
	axes2->setMatrix(osg::Matrix::translate(5,0,0));

	axes1->addChild(node1);
	axes2->addChild(node2);

	osg::ref_ptr<osg::Group> group = new osg::Group;

	group->addChild(axes1);
	group->addChild(axes2);

	osg::ref_ptr<osg::Group> root =  new osg::Group;
	root->addChild(group);

	osg::ref_ptr<osgViewer::Viewer> viewer =  new osgViewer::Viewer;
	viewer->setUpViewInWindow(500,200,1000,800);
	viewer->setSceneData(root.get());
	viewer->run();
	
}