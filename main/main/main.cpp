#include <stdlib.h>
#include <iostream>

#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osg/Node>
#include <osg/Geode>
#include <osg/Group>

#include <osg/MatrixTransform>
#include <osg/TriangleFunctor>
#include <vector>

#include <osg/ComputeBoundsVisitor>
#include <osg/ShapeDrawable>
#include <osg/PolygonMode>
#include <osg/LineWidth>



//用来记录一个结点对应三角面片跟AABB的结构体
struct drawableInfo
{
	std::vector<osg::ref_ptr<osg::Geode>> res_TP;
	std::vector<osg::ref_ptr<osg::Geode>> res_AABB;

};

//根据一个结点求对应的AABB
osg::ref_ptr<osg::Geode> createboundingbox(osg::Node* node)
{
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	osg::ComputeBoundsVisitor boundvisitor ;
	node->accept(boundvisitor);
	osg::BoundingBox bb = boundvisitor.getBoundingBox();

	float lengthx = bb.xMax()-bb.xMin();
	float lengthy = bb.yMax()-bb.yMin();
	float lengthz = bb.zMax()-bb.zMin();
	osg::Vec3 center= osg::Vec3((bb.xMax()+bb.xMin())/2,(bb.yMax()+bb.yMin())/2,(bb.zMax()+bb.zMin())/2);

	osg::ref_ptr<osg::ShapeDrawable>  drawable = new osg::ShapeDrawable(new osg::Box(center,lengthx,lengthy,lengthz));
	drawable->setColor(osg::Vec4(1.0,1.0,0.0,1.0));

	osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
	stateset= drawable->getOrCreateStateSet();
	osg::ref_ptr<osg::PolygonMode> polygon = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,osg::PolygonMode::LINE);
	stateset->setAttributeAndModes(polygon);

	//设置线宽
	osg::ref_ptr<osg::LineWidth> linewidth = new osg::LineWidth(3.0);
	stateset->setAttribute(linewidth);


	geode->addDrawable(drawable);
	return geode;

}

//根据三个顶点确定一个三维面片
osg::ref_ptr<osg::Geode> GetTrainglePlane(osg::Vec3f* p1,osg::Vec3f* p2,osg::Vec3f* p3)
{
	/*osg::Geode* geode=new osg::Geode;
	osg::Geometry* polyGeom = new osg::Geometry;*/

	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	osg::ref_ptr<osg::Geometry> polyGeom =  new osg::Geometry;

	osg::Vec3 myCoords[]=
	{
		*p1,
		*p2,
		*p3
	};

	int numCoords = sizeof(myCoords)/sizeof(osg::Vec3);
	osg::Vec3Array* vertices = new osg::Vec3Array(numCoords,myCoords);
	polyGeom->setVertexArray(vertices);
	polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES,0,numCoords));
	geode->addDrawable(polyGeom);
	geode->addDrawable(polyGeom);
	//getTriangles(*polyGeom);
	return geode;
}

//三角面片访问结构体
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


//获取一个Drawable的三角面片集合
//std::vector<osg::ref_ptr<osg::Geode>> getTriangles(osg::Drawable& drawable)
drawableInfo* getTriangles(osg::Drawable& drawable)
{

	std::vector<osg::ref_ptr<osg::Geode>> res_TP;
	std::vector<osg::ref_ptr<osg::Geode>> res_AABB;

	osg::Vec3* p1 = new osg::Vec3;
	osg::Vec3* p2 = new osg::Vec3;
	osg::Vec3* p3 = new osg::Vec3;

	osg::TriangleFunctor<GetVertex> tf;
	tf.vertexList=new osg::Vec3Array;

	drawable.accept(tf);
	int i = 0;
	for(osg::Vec3Array::iterator itr=tf.vertexList->begin();
		itr!=tf.vertexList->end();
		itr++)
	{
		osg::Vec3 vertex=*itr;
		//std::cout<<vertex<<std::endl;
		std::cout<<itr->x()<<" "<<itr->y()<<" "<<itr->z()<<std::endl;
		//std::cout<<std::endl;

		/*if (i == 2)
		{
			std::cout<<std::endl;
			i = 0;
		}
		else
		{
			i++;
		}*/

		osg::ref_ptr<osg::Geode> geode_TP = new osg::Geode;
		osg::ref_ptr<osg::Geode> geode_AABB = new osg::Geode;
		switch(i)
		{
			case 0:
				p1 = &vertex;
				i++;
				break;
			case 1:
				p2 = &vertex;
				i++;
				break;
			case 2:
				p3 = &vertex;
				std::cout<<std::endl;
				i = 0;
				geode_TP = GetTrainglePlane(p1, p2, p3);
				res_TP.push_back(geode_TP);

				geode_AABB = createboundingbox(geode_TP);
				res_AABB.push_back(geode_AABB);

				break;
		}

	}

	std::cout<<std::endl;

	drawableInfo* res = new drawableInfo;
	res->res_TP = res_TP;
	res->res_AABB = res_AABB;
	return res;
}


void main(int argc,char** argv[])
{
	//载入两个实验结点
	osg::ref_ptr<osg::Node> node1 = new osg::Node;
	osg::ref_ptr<osg::Node> node2 = new osg::Node;

	node1 = osgDB::readNodeFile("cow.osg");
	node2 = osgDB::readNodeFile("cessna.osg");

	osg::ref_ptr<osg::MatrixTransform> axes1 = new osg::MatrixTransform;
	osg::ref_ptr<osg::MatrixTransform> axes2 = new osg::MatrixTransform;

	axes1->setMatrix(osg::Matrix::translate(5,20,20));
	axes2->setMatrix(osg::Matrix::translate(5,0,0));

	axes1->addChild(node1);
	axes2->addChild(node2);

	osg::ref_ptr<osg::Group> group = new osg::Group;

	group->addChild(axes1);
	group->addChild(axes2);

	osg::ref_ptr<osg::Group> root =  new osg::Group;
	root->addChild(group);
	
	//测试代码区
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode = node1->asGroup()->getChild(0)->asGeode();

	getTriangles(*root->getChild(0)->asGroup()->getChild(0)->asGroup()->getChild(0)->asGroup()->getChild(0)->asGeode()->getDrawable(0));

	//展示整个场景
	osg::ref_ptr<osgViewer::Viewer> viewer =  new osgViewer::Viewer;
	viewer->setUpViewInWindow(500,200,1000,800);
	viewer->setSceneData(root.get());
	viewer->run();
	
}