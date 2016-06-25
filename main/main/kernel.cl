struct TriangleCandidateSplitPlane
{
	int triangleID;
	float xCandidateSplitPlane;
	float yCandidateSplitPlane;
	float zCandidateSplitPlane;
};


__kernel void bitonicSort (global struct TriangleCandidateSplitPlane* input,  
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
              
            if ( (*dir) == (input[a].xCandidateSplitPlane > input[b].xCandidateSplitPlane) ) {  
                struct TriangleCandidateSplitPlane temp = input[a];  
                input[a] = input[b];  
                input[b] = temp;  
            }  
        }  
    }  
}  