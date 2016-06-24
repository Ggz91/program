#ifndef CUSLIB_H	
#define CUSLIB_H

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



//用来记录三家片相关信息的结构体
struct TriangleInfo
{
	osg::Vec3f* vecInfo[3];
};

//用来记录一个结点对应三角面片跟AABB的结构体
struct DrawableInfo
{
	/*std::vector<osg::ref_ptr<osg::Geode>> res_TP;
	std::vector<osg::ref_ptr<osg::Geode>> res_AABB;*/
	std::vector<TriangleInfo*> triangleInfoArray;
	osg::Vec3Array* vertexList;
};

//根据一个结点求对应的AABB（跟节点物体的方向无关，AABB各个面都是轴平行的）
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
osg::ref_ptr<osg::Geode> GetTrainglePlane(osg::Vec3f* p1,osg::Vec3f* p2,osg::Vec3f* p3,osg::ref_ptr<osg::Geode> parent)
{
	/*osg::Geode* geode=new osg::Geode;
	osg::Geometry* polyGeom = new osg::Geometry;*/

	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	osg::ref_ptr<osg::Geometry> polyGeom =  new osg::Geometry;

	osg::Vec3f p1t = (*p1)*computeLocalToWorld(parent->getParentalNodePaths()[0]);
	osg::Vec3f p2t = (*p2)*computeLocalToWorld(parent->getParentalNodePaths()[0]);
	osg::Vec3f p3t = (*p3)*computeLocalToWorld(parent->getParentalNodePaths()[0]);
	
	osg::Vec3 myCoords[]=
	{
		p1t,
		p2t,
		p3t
	};

	
	int numCoords = sizeof(myCoords)/sizeof(osg::Vec3);
	osg::Vec3Array* vertices = new osg::Vec3Array(numCoords,myCoords);
	polyGeom->setVertexArray(vertices);
	polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES,0,numCoords));
	geode->addDrawable(polyGeom);
	//geode->addDrawable(polyGeom);
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

void GetTraingleInfo(osg::Vec3f* p1, osg::Vec3f* p2, osg::Vec3f* p3, TriangleInfo* res)
{
	res->vecInfo[0] =(osg::Vec3f*) p1;
	res->vecInfo[1] =(osg::Vec3f*) p2;
	res->vecInfo[2] =(osg::Vec3f*) p3;
}


//获取一个Drawable的三角面片集合
//DrawableInfo* getTriangles(osg::Drawable& drawable,osg::ref_ptr<osg::Geode> geode)
//{
//
//	/*std::vector<osg::ref_ptr<osg::Geode>> res_TP;
//	std::vector<osg::ref_ptr<osg::Geode>> res_AABB;*/
//
//	std::vector<TriangleInfo*> resInfo;
//
//	osg::Vec3* p1 = new osg::Vec3;
//	osg::Vec3* p2 = new osg::Vec3;
//	osg::Vec3* p3 = new osg::Vec3;
//
//	osg::TriangleFunctor<GetVertex> tf;
//	tf.vertexList=new osg::Vec3Array;
//
//	drawable.accept(tf);
//	int i = 0;
//	for(osg::Vec3Array::iterator itr=tf.vertexList->begin();
//		itr!=tf.vertexList->end();
//		itr++)
//	{
//		osg::Vec3 vertex=*itr;
//		//std::cout<<vertex<<std::endl;
//		std::cout<<itr->x()<<" "<<itr->y()<<" "<<itr->z()<<std::endl;
//		//std::cout<<std::endl;
//
//		/*if (i == 2)
//		{
//			std::cout<<std::endl;
//			i = 0;
//		}
//		else
//		{
//			i++;
//		}*/
//
//		//osg::ref_ptr<osg::Geode> geode_TP = new osg::Geode;
//		//osg::ref_ptr<osg::Geode> geode_AABB = new osg::Geode;
//		TriangleInfo* traingleInfo;
//		switch(i)
//		{
//			case 0:
//				p1 = &vertex;
//				i++;
//				break;
//			case 1:
//				p2 = &vertex;
//				i++;
//				break;
//			case 2:
//				p3 = &vertex;
//				std::cout<<std::endl;
//				i = 0;
//				/*geode_TP = GetTrainglePlane(p1, p2, p3,geode);
//				res_TP.push_back(geode_TP);
//
//				geode_AABB = createboundingbox(geode_TP);
//				res_AABB.push_back(geode_AABB);*/
//
//				traingleInfo = GetTraingleInfo(p1, p2, p3);
//				resInfo.push_back(traingleInfo);
//				break;
//		}
//
//	}
//	
//	std::cout<<std::endl;
//
//	DrawableInfo* res = new DrawableInfo;
//	/*res->res_TP = res_TP;
//	res->res_AABB = res_AABB;*/
//
//	res->triangleInfoArray = resInfo;
//
//	return res;
//}

DrawableInfo* getTriangles(osg::Drawable& drawable, osg::ref_ptr<osg::Node> node)
{
	osg::TriangleFunctor<GetVertex> tf;
	tf.vertexList=new osg::Vec3Array;

	drawable.accept(tf);

	TriangleInfo* resTrianglesInfo = new TriangleInfo;
	DrawableInfo* res = new DrawableInfo;
	osg::Vec3f* p1 = new osg::Vec3f;
	osg::Vec3f* p2 = new osg::Vec3f;
	osg::Vec3f* p3 = new osg::Vec3f;

	res->vertexList = tf.vertexList;

	int i = 1;
	for(osg::Vec3Array::iterator itr=tf.vertexList->begin();
		itr!=tf.vertexList->end();
		itr++)
	{
		//std::cout<<itr->x()<<" "<<itr->y()<<" "<<itr->z()<<std::endl;
		//std::cout<<std::endl;



		switch(i)
		{
		case 1:
			p1 = new osg::Vec3f;
			p1 = &(*itr);
			break;
		case 2:
			p2 = new osg::Vec3f;
			p2 = &(*itr);

			break;
		case 3:
			p3 = new osg::Vec3f;
			p3 = &(*itr);

			//	std::cout<<"================================"<<std::endl;
			resTrianglesInfo = new TriangleInfo;
			GetTraingleInfo(p1, p2, p3, resTrianglesInfo);
			res->triangleInfoArray.push_back(&(*resTrianglesInfo));
			//	std::cout<<std::endl;
			i = 0;
			break;
		default:
			break;
		}



		i++;
	}

	//	std::cout<<std::endl;
	return res;
}

DrawableInfo* transDIfromLocalToWorld(DrawableInfo* di)
{
	DrawableInfo* res = new DrawableInfo;

	//std::vector<osg::ref_ptr<osg::Geode>>::iterator it1 = di->res_AABB.begin();
	//std::vector<osg::ref_ptr<osg::Geode>>::iterator it2 = di->res_TP.begin();

	//for (;it1 != di->res_AABB.end();it1++)
	//{
	//	/*int i ;
	//	i = (*it1)->getNumParents();*/
	//	(*it1)->getBound().center()*osg::computeLocalToWorld((*it1)->getParentalNodePaths()[0]);
	//	//printf("%d",i);
	//}

	return res;

}

void createGeode(TriangleInfo* triangleInfo, osg::Geode* geode)
{
	osg::Geometry* polyGeom = new osg::Geometry;
	osg::Vec3f myCoords[]=
	{
		*(triangleInfo->vecInfo[0]),
		*(triangleInfo->vecInfo[1]),
		*(triangleInfo->vecInfo[2]),

	};

	int numCoords = sizeof(myCoords)/sizeof(osg::Vec3f);
	osg::Vec3Array* vertices = new osg::Vec3Array(numCoords,myCoords);
	polyGeom->setVertexArray(vertices);
	polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES,0,numCoords));
	geode->addDrawable(polyGeom);
}


#endif