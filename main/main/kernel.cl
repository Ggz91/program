
struct TriangleCandidateSplitPlane
{
	int triangleID;
	float xMin;
	float yMin;
	float zMin;
	float xMax;
	float yMax;
	float zMax;
};

struct SplitNode
{
	int beg;
	int end;
	int leftChild;
	int rightChild;
	float xMin;
	float xMax;
	float yMin;
	float yMax;
	float zMin;
	float zMax;
};



//用于矩阵的运算，转换坐标系用
struct mat4
{
	float4 l0;
	float4 l1;
	float4 l2;
	float4 l3;

	
};


float4  mul(struct mat4 m4, float4 l)
	{
		float4 res;
		res.x = m4.l0.x*l.x + m4.l0.y*l.y + m4.l0.z*l.z + m4.l0.w*l.w;
		res.y = m4.l1.x*l.x + m4.l1.y*l.y + m4.l1.z*l.z + m4.l1.w*l.w;
		res.z = m4.l2.x*l.x + m4.l2.y*l.y + m4.l2.z*l.z + m4.l2.w*l.w;
		res.w = m4.l3.x*l.x + m4.l3.y*l.y + m4.l3.z*l.z + m4.l3.w*l.w;

		return res;
		
	};

void UpdateSplitNodeWithAABBInfo(struct SplitNode *nodeArray,
								 int idx, 
								 struct TriangleCandidateSplitPlane *input)
{
	for(int i = nodeArray[idx].beg + 1; i <=  nodeArray[idx].end; i++)
	{
		 nodeArray[idx].xMin = ( nodeArray[idx].xMin < input[i].xMin)?  (nodeArray[idx].xMin):(input[i].xMin);
		 nodeArray[idx].xMax = ( nodeArray[idx].xMax > input[i].xMax)?  (nodeArray[idx].xMax):(input[i].xMax);
		 nodeArray[idx].yMin = ( nodeArray[idx].yMin < input[i].yMin)?  (nodeArray[idx].yMin):(input[i].yMin);
		 nodeArray[idx].yMax = ( nodeArray[idx].yMax > input[i].yMax)?  (nodeArray[idx].yMax):(input[i].yMax);
		 nodeArray[idx].zMin = ( nodeArray[idx].zMin < input[i].zMin)?  (nodeArray[idx].zMin):(input[i].zMin);
		 nodeArray[idx].zMax = ( nodeArray[idx].zMax > input[i].zMax)?  (nodeArray[idx].zMax):(input[i].zMax);
	}
}

float ComputeSAH(int pos, struct SplitNode* nodeArray, int idx, struct TriangleCandidateSplitPlane* input)
{
		int lNum = pos - nodeArray[idx].beg + 1;
		int rNum = nodeArray[idx].end - pos + 1;

		float  wholeSurfaceArea = 2*(nodeArray[idx].xMax -  nodeArray[idx].xMin)
								 *(nodeArray[idx].yMax - nodeArray[idx].yMin) 
								 + 2*(nodeArray[idx].xMax - nodeArray[idx].xMin)
								 *(nodeArray[idx].zMax - nodeArray[idx].zMin)
								 + 2*(nodeArray[idx].zMax - nodeArray[idx].zMin)
								 *(nodeArray[idx].yMax - nodeArray[idx].yMin);
		float leftSurfaceArea = 2*(input[pos].xMin -  nodeArray[idx].xMin)
								 *(nodeArray[idx].yMax - nodeArray[idx].yMin) 
								 + 2*(input[pos].xMin - nodeArray[idx].xMin)
								 *(nodeArray[idx].zMax - nodeArray[idx].zMin)
								 + 2*(nodeArray[idx].zMax - nodeArray[idx].zMin)
								 *(nodeArray[idx].yMax - nodeArray[idx].yMin);
		float rightSurfaceArea = 2*(nodeArray[idx].xMax -  input[pos].xMin)
								 *(nodeArray[idx].yMax - nodeArray[idx].yMin) 
								 + 2*(nodeArray[idx].xMax - input[idx].xMin)
								 *(nodeArray[idx].zMax - nodeArray[idx].zMin)
								 + 2*(nodeArray[idx].zMax - nodeArray[idx].zMin)
								 *(nodeArray[idx].yMax - nodeArray[idx].yMin);
		
		float SAH = (lNum*leftSurfaceArea + rNum*rightSurfaceArea) / wholeSurfaceArea;
		return SAH;

}

__kernel void BitonicSort (global struct TriangleCandidateSplitPlane* input,  
                         global const int* group,  
                         global const int* length,  
                         global const int* dir,  
                         global const int* flip)  
{  
    int bid = get_group_id(0);  
    int groupsInKernel = get_num_groups(0);  
    int wid = get_local_id(0);  
    int itemsInGroup = get_local_size(0);  
      
    for(int gpos = bid; gpos < (*group); gpos += groupsInKernel) {  
        for(int pos = wid; pos < (*length)/2; pos += itemsInGroup) {  
            int begin = gpos * (*length);  
            int delta;  
            if ((*flip) == 1)   delta = (*length) - 1;  
            else                delta = (*length)/2;  
              
            int a = begin + pos;  
            int b = begin + delta - (*flip) * pos;  
              
            if ( (*dir) == (input[a].xMin > input[b].xMin) ) {  
                struct TriangleCandidateSplitPlane temp = input[a];  
                input[a] = input[b];  
                input[b] = temp;  
            }  
        }  
    }  
}  

__kernel void SAHSplit(global const struct TriangleCandidateSplitPlane* input,
						global struct SplitNode*	splitNodeArray,
						global const int* splitNodeArrayBeg,
						global const int* splitNodeArrayEnd,
						global const int* randPos,
						global const int* randPro,
						global const int* maxSize
						)
{
	int idx= get_global_id(0) + (*splitNodeArrayBeg);
	//printf("Beg:%d\t\tEnd:%d\t\tID:%d\t\t\nnodeBeg:%d\t\tnodeEnd:%d\t\t\n", *splitNodeArrayBeg, *splitNodeArrayEnd, idx, splitNodeArray[idx].beg, splitNodeArray[idx].end);
	if((idx>= *splitNodeArrayBeg)&&(idx<= *splitNodeArrayEnd) && (splitNodeArray[idx].end - splitNodeArray[idx].beg > 64) && (splitNodeArray[idx].beg != -1) && (splitNodeArray[idx].end != -1))
	{
		splitNodeArray[idx].xMax = input[splitNodeArray[idx].beg].xMax;
		splitNodeArray[idx].xMin = input[splitNodeArray[idx].beg].xMin;
		splitNodeArray[idx].yMax = input[splitNodeArray[idx].beg].yMax;
		splitNodeArray[idx].yMin = input[splitNodeArray[idx].beg].yMin;
		splitNodeArray[idx].zMax = input[splitNodeArray[idx].beg].zMax;
		splitNodeArray[idx].zMin = input[splitNodeArray[idx].beg].zMin;
		UpdateSplitNodeWithAABBInfo(splitNodeArray, idx, input);
		
		int i = 0;
		int j = 0;
		int currentPos = randPos[i++] % ((splitNodeArray[idx].end) - (splitNodeArray[idx].beg) + 1) + splitNodeArray[idx].beg;
		float currentSAH = ComputeSAH(currentPos, splitNodeArray, idx, input);
		
		//printf("Beg:%d\t\tEnd:%d\t\tID:%d\t\t\nnodeBeg:%d\t\tnodeEnd:%d\t\t\n", *splitNodeArrayBeg, *splitNodeArrayEnd, globalID, splitNodeArray[globalID].beg, splitNodeArray[globalID].end);
		//printf("before:\nrandPos:%d\t\tcurrentPos:%d\t\tcurrentSAH:%f\n",randPos[i-1], currentPos, currentSAH);

		int T = T0;
		while(T > 1)
		{
			int n = 0;
			while(n < NMAX)
			{
				int newPos = randPos[i++] % ((splitNodeArray[idx].end) - (splitNodeArray[idx].beg) + 1) + splitNodeArray[idx].beg;
				float newSAH = ComputeSAH(newPos, splitNodeArray, idx, input);

				if(newSAH < currentSAH)
				{
					currentPos = newPos;
					currentSAH = newSAH;
				}
				else
				{
					float r = exp(-(newSAH - currentSAH) / T);
					if(randPro[j++] < r)
					{
						currentPos = newPos;
						currentSAH = newSAH;
					}
				}//endif(newSAH < currentSAH)
				n++;
			}//end while(n<8)
			T = 0.2*T;
		}//end while(T > 1)

		//printf("after:\nrandPos:%d\t\tcurrentPos:%d\t\tcurrentSAH:%f\n",randPos[i-1], currentPos, currentSAH);

		splitNodeArray[idx*2 + 1].beg = splitNodeArray[idx].beg;
		splitNodeArray[idx*2 + 1].end = currentPos - 1;
		splitNodeArray[idx*2 + 2].beg = currentPos;
		splitNodeArray[idx*2 + 2].end = splitNodeArray[idx].end;
		if((idx*2 + 2) < *maxSize)
		{
			splitNodeArray[idx].leftChild = idx*2 + 1;
			splitNodeArray[idx].rightChild = idx*2 + 2;
		}
		
		
		
	}
	//printf("\n");
}

struct t
{
	float txMax;
	float txMin;
	float tyMax;
	float tyMin;
	float tzMax;
	float tzMin;

	
};

bool bCross(struct t stT, float* tMin, float* tMax)
	{
		if((stT.txMax < stT.tyMin) || (stT.tyMax < stT.txMin)) return false;

		*tMin = (stT.txMin < stT.tyMin) ? stT.txMin : stT.tyMin;
		*tMax = (stT.txMax > stT.tyMax) ? stT.txMax : stT.tyMax;

		if((*tMax < stT.tzMin) || (stT.tzMax < *tMin)) return false;

		*tMin = (*tMin < stT.tzMin) ? *tMin : stT.tzMin;
		*tMax = (*tMax > stT.tzMax) ? *tMax : stT.tzMax;

		return true;

	

	};

struct cusVec3
{
	float x;
	float y;
	float z;
};

struct TriangleInfo
{
	int triangleID;

	struct cusVec3 vecInfo[3];

	
};

bool bIntersect(struct TriangleInfo sTri,float3 f3EyePos, float3 f3LightDir,float* tMin, float* tMax)
	{
		float a1 = f3LightDir.x;
		float a2 = f3LightDir.y;
		float a3 = f3LightDir.z;
		float b1 = sTri.vecInfo[0].x - sTri.vecInfo[1].x;
		float b2 = sTri.vecInfo[0].y - sTri.vecInfo[1].y;
		float b3 = sTri.vecInfo[0].z - sTri.vecInfo[1].z;
		float c1 = sTri.vecInfo[0].x - sTri.vecInfo[2].x;
		float c2 = sTri.vecInfo[0].y - sTri.vecInfo[2].y;
		float c3 = sTri.vecInfo[0].z - sTri.vecInfo[2].z;
		float d1 = f3EyePos.x - sTri.vecInfo[0].x;
		float d2 = f3EyePos.y - sTri.vecInfo[0].y;
		float d3 = f3EyePos.z - sTri.vecInfo[0].z;
		float detA = a1*b2*c3 + b1*c2*a3 + c1*a2*b3 - (c1*b2*a3 + b1*a2*c3 + a1*c2*b3);
		float detA1 = c1*b2*d3 + b1*d2*c3 + d1*c2*b3 - (d1*b2*c3 + b1*c2*d3 + c1*d2*b3);
		float detA2 = c1*d2*a3 + d1*a2*c3 + a1*c2*d3 - (a1*d2*c3 + d1*c2*a3 + c1*a2*d3);
		float detA3 = d1*b2*a3 + b1*a2*d3 + a1*d2*b3 - (a1*b2*d3 + b1*d2*a3 + d1*a2*b3);

		float t = detA1 / detA;
		float b = detA2 / detA;
		float c = detA3 / detA;

		if( ( t>(*tMax) ) || ( t<(*tMin) )) return false;
		if( ( c>1 ) || ( c<0 )) return false;
		if( ( b>1-c ) || ( b<0 )) return false;
		
	}

//遍历叶子节点的三角面片
float3 RayCrossTraingleTest(struct SplitNode node, float3 f3EyePos, float3 f3LightDir, float* fDst,  struct TriangleInfo* TriangleInfoArray, struct TriangleCandidateSplitPlane* input, float* tMin, float* tMax)
{
	float3 f3Res = (float3)(0, 0, 255);
	for(int i=node.beg; i<=node.end; i++)
	{
		int idx = input[i].triangleID;
		if( bIntersect(TriangleInfoArray[idx], f3EyePos, f3LightDir, tMin, tMax) )
		{
			f3Res = (float3)(255, 255, 255);
		}
	}
	return f3Res;
	
}


//创建栈
struct Stack
{
	int		top;
	int		size;
	struct	SplitNode* ssMemNode;

};

void InitStack(struct Stack* sSt, int size)
{
	(*sSt).size = size;
	(*sSt).top = 0;
}

void PushStack(struct Stack* sSt, struct SplitNode sSN)
{
	int idx = (*sSt).top;
	if( (*sSt).size < idx)
	{
		printf("the top is beyond the size of stack");
		return;
	}
	(*sSt).ssMemNode[idx] = sSN;
	(*sSt).top++;

}

struct SplitNode PopStack(struct Stack* sSt)
{
	int idx = (*sSt).top;
	if( 0 == idx)
	{
		printf("the top of Stack is zero");
	}
	struct SplitNode sRes = (*sSt).ssMemNode[idx];
	(*sSt).top--;
	return sRes;
}

bool IsEmpty(struct Stack* sSt)
{
	if( 0==(*sSt).top ) return true;
	return false;
}


float3 RayCrossAABBTest(struct SplitNode root, float3 f3EyePos, float3 f3LightDir, struct SplitNode* spSplitNodeArray, float* fDst, struct TriangleInfo* TriangleInfoArray, struct TriangleCandidateSplitPlane* input, float* tMin, float* tMax, int* length)
{
	float3 f3Res;
	////判断是不是最终的叶子节点
	//if( (root.leftChild == -1 ) && (root.rightChild == -1)) 
	//{
	//	f3Res = RayCrossTraingleTest(root, f3EyePos, f3LightDir, fDst, TriangleInfoArray, input, tMin, tMax);
	//	return f3Res;
	//}

	//struct t sT;
	//sT.txMax = (root.xMax - f3EyePos.x)/f3LightDir.x;
	//sT.txMin = (root.xMin - f3EyePos.x)/f3LightDir.x;
	//sT.tyMax = (root.yMax - f3EyePos.y)/f3LightDir.y;
	//sT.tyMin = (root.yMin - f3EyePos.y)/f3LightDir.y;
	//sT.tzMax = (root.zMax - f3EyePos.z)/f3LightDir.z;
	//sT.tzMin = (root.zMin - f3EyePos.z)/f3LightDir.z;

	//if( bCross(sT, tMin, tMax) )
	//{
	//	float fLeftDst, fRightDst;
	//	float3 f3LeftPixel = RayCrossAABBTest(spSplitNodeArray[root.leftChild], f3EyePos, f3LightDir, spSplitNodeArray, &fLeftDst, TriangleInfoArray, input, tMin, tMax);
	//	float3 f3RightPixel = RayCrossAABBTest(spSplitNodeArray[root.rightChild], f3EyePos, f3LightDir, spSplitNodeArray, &fRightDst, TriangleInfoArray, input, tMin, tMax);
	//	f3Res = (fLeftDst < fRightDst) ? f3LeftPixel : f3RightPixel;
	//}
	//else 
	//{
	//	//不经过当前包围盒的话，
	//	f3Res = (float3)(0, 0, 255);
	//	//fLeftDst = 10000;
	//}
	
	struct Stack* sStack;
	InitStack(sStack, (*length));
	PushStack(sStack, root);
	while( IsEmpty(sStack) )
	{
		struct SplitNode snCurNode = PopStack(sStack);
		//判断是否为叶子节点
		if( ( snCurNode.leftChild == -1) && (snCurNode.rightChild == -1) )
		{
			f3Res = RayCrossTraingleTest( snCurNode, f3EyePos, f3LightDir, fDst, TriangleInfoArray, input, tMin, tMax);
		}
		else
		{
			PushStack(sStack, spSplitNodeArray[snCurNode.leftChild]);
			PushStack(sStack, spSplitNodeArray[snCurNode.rightChild]);
		}
	}

	return f3Res;
}

__kernel void RayTrace(__global struct SplitNode* spSplitNodeArray, __global int* iWinWidth, __global int* iWinHeight, __global char* pcResPB, __global struct TriangleInfo* TriangleInfoArray, __global struct TriangleCandidateSplitPlane* input, __global int*  length)
{
	float tMin = 0;
	float tMax = 0;
	float fDst = 10000;
	float3 f3EyePos = (float3)(0, 0, 5);
	int idx = get_global_id(0);

	for(int i = 0; i<(*iWinHeight); i++)
	{
		float3 f3PixPos = (float3)(idx,i, 4.9);
		float3 f3LightDir = normalize(f3PixPos - f3EyePos);
		
		float3 f3Res = RayCrossAABBTest(spSplitNodeArray[0], f3EyePos, f3LightDir, spSplitNodeArray, &fDst, TriangleInfoArray, input, &tMin, &tMax, length);
		pcResPB[(idx*(*iWinWidth)+i)*3] = f3Res.x;
		pcResPB[(idx*(*iWinWidth)+i)*3 + 1] = f3Res.y;
		pcResPB[(idx*(*iWinWidth)+i)*3 + 2] = f3Res.z;


	}
}