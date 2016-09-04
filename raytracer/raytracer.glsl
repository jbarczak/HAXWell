

// NOTES: 
//  1.Forcing SIMD8 for the shader instead of SIMD16 yields an enormous perf. boost (4500 K/rays to 5500)
//  2.Tried skipping the extra min/max in ray/box test by using precomputed offsets
//     but couldn't get compiler to generate good code
//  3. Using vectorized loads is beneficial.  Compiler doesn't seem to do lots of scalar loads intelligently
#version 430 core
layout (local_size_x = 8) in;

  
struct Ray
{
    // input
    vec4 O_tmax;
    vec4 D;
};

struct HitInfo
{
    float hit_u;
    float hit_v;
    uint hit_id;
    float tmax;
};

struct BVHNode
{
    vec3 BBMin;
    uint nOffs;
    vec3 BBMax;
    uint nAxisAndTriCount;
};



layout (std430,binding=0)
buffer Rays
{
    uint g_nRays;
    uint _PAD0;
    uint _PAD1;
    uint _PAD2; // 16 byte align
    Ray g_Rays[];
};

layout (std430,binding=1)
buffer Hits
{
    HitInfo g_Hits[];
};


layout (std430,binding=2)
buffer Nodes
{
    vec4 g_Nodes[];
};


BVHNode FetchNode( uint id )
{
    vec4 n0 = g_Nodes[2*id];
    vec4 n1 = g_Nodes[2*id+1];

    BVHNode n;
    n.BBMin = n0.xyz;
    n.nOffs   = floatBitsToUint(n0.w);
    n.BBMax = n1.xyz;
    n.nAxisAndTriCount = floatBitsToUint(n1.w);
    
    return n;
}



struct TrianglePP
{
    vec3 P0;
    vec3 v02;
    vec3 v10;
    vec3 v10x02;
};

layout (std430,binding=3)
buffer TriPreproc
{
    vec4 g_Preproc[];
};


TrianglePP FetchPreprocessedTri( uint id )
{
    vec4 pp0 = g_Preproc[3*id];
    vec4 pp1 = g_Preproc[3*id+1];
    vec4 pp2 = g_Preproc[3*id+2]; // Intel GLSL compiler does a better job if the loads are vectorized
    TrianglePP tri;
    tri.P0      = vec3( pp0.x, pp0.y, pp0.z );
    tri.v02     = vec3( pp0.w, pp1.x, pp1.y );
    tri.v10     = vec3( pp1.z, pp1.w, pp2.x );
    tri.v10x02  = vec3( pp2.y, pp2.z, pp2.w );
    return tri;
}


bool IsLeaf( BVHNode node ) { return (node.nAxisAndTriCount & 3) == 3; }
uint GetNodeSplitAxis( BVHNode node ) { return node.nAxisAndTriCount&3; }
uint FirstTriangle( BVHNode node ) { return node.nOffs; }
uint GetLeftChildIndex( BVHNode node ) { return node.nOffs; }
uint TriangleCount( BVHNode node ) { return node.nAxisAndTriCount>>2; }

///uvec3 ReadTri( uint t ) { return uvec3( g_Indices[3*t], g_Indices[3*t+1], g_Indices[3*t+2] ); };
//vec3 ReadVert( uint v ) { return vec3( g_Vertices[3*v], g_Vertices[3*v+1], g_Vertices[3*v+2] ); };

bool BoxTest( vec3 O, vec3 DInv, float tmax, BVHNode node )
{
    vec3 vBBMin = node.BBMin;
    vec3 vBBMax = node.BBMax;
    vec3 vMin   = (vBBMin - O )*DInv;
    vec3 vMax   = (vBBMax - O )*DInv;
    vec3 t0     = min(vMin,vMax);
    vec3 t1     = max(vMin,vMax);
    float start = max( max(t0.x,t0.y), t0.z );
    float end   = min( min(t1.x,t1.y), t1.z );
    return (start <= min(end,tmax)) && (end>=0) ;
}

void main() 
{
    uint tid = gl_GlobalInvocationID.x;
    if( tid > g_nRays )
        return;

    Ray ray    = g_Rays[tid];
    vec3 O     = ray.O_tmax.xyz;
    vec3 D     = ray.D.xyz;
    vec3 DInv  = 1.0f / D;
    float tmax = ray.O_tmax.w;

    float hit_u  = 0;
    float hit_v  = 0;
    uint hit_id = 0xffffffff;

    uint axis_bits = ((D.x<0) ? 1 : 0) |
                     ((D.y<0) ? 2 : 0) |
                     ((D.z<0) ? 4 : 0);

    uint Stack[32];
    uint nStackPtr=0;

    BVHNode node = FetchNode(0);
            
    while( true )
    {
        while( !IsLeaf(node)  )
        {
            if( BoxTest(O,DInv,tmax,node) )
            {
                // visit subtrees in split-axis order
                uint axis  = GetNodeSplitAxis(node);
                uint near = ((axis_bits)>>axis)&1;
                uint far  =  near^1;
                uint child = GetLeftChildIndex(node);
                near += child;
                far += child;
                Stack[nStackPtr++] = far;
                node = FetchNode( near );
            }
            else
            {
                // resume with next node from stack
                if( nStackPtr == 0 )
                {
                    // no stack nodes.   Store results and stop
                    g_Hits[tid].hit_u  = hit_u;
                    g_Hits[tid].hit_v  = hit_v;
                    g_Hits[tid].hit_id = hit_id;
                    g_Hits[tid].tmax   = tmax;
                    return;
                }
                node = FetchNode( Stack[--nStackPtr] );
            }
        }

        // intersect some triangles in a leaf
        if( BoxTest(O,DInv,tmax,node) )
        {
            uint first = FirstTriangle(node);
            uint count = TriangleCount(node);
            for( uint i=0; i<count; i++ )
            {
                TrianglePP tri = FetchPreprocessedTri( first+i );
                vec3 P0     = tri.P0;
                vec3 v02    = tri.v02;
                vec3 v10    = tri.v10;
                vec3 v10x02 = tri.v10x02;

                vec3 v0A = ( P0 - O );
                vec3 v02x0a = cross( v02, v0A );

                float V = 1.0f / dot( v10x02, D );
                float A = V * dot( v02x0a, D );
     
                vec3 v10x0a = cross( v10, v0A );
                float B = V * dot( v10x0a, D );
            
                float VA = dot( v10x02, v0A );
                float T = VA*V;
                if( A>=0.0f && B>=0.0f && (A+B)<=1.0f )
                {
                    if( T < tmax && T > 0.0f )
                    {
                        tmax   = T;
                        hit_u  = A;
                        hit_v  = B;
                        hit_id = first+i;
                    }
                }
            }
        }

                
        if( nStackPtr == 0 )
        {
            // no stack nodes.  Store results and stop
            g_Hits[tid].hit_u  = hit_u;
            g_Hits[tid].hit_v  = hit_v;
            g_Hits[tid].hit_id = hit_id;
            g_Hits[tid].tmax   = tmax;
            return;
        }
                
        node = FetchNode( Stack[--nStackPtr] );
    } 
}