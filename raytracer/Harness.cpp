#include "HaxWell.h"
#include "Rand.h"
#include "PlyLoader.h"
#include "Matrix.h"
#include "PPMImage.h"

#include "tinyrt/TinyRT.h"
using namespace Simpleton;

#include <smmintrin.h>
#include <Windows.h>

#define PACKET_SIZE (1<<17)     // Number of rays allowed in flight
#define BUCKET_SIZE (1<<12)     // Number of rays per dispatch
#define NUM_PHOTONS 10000000
#define BOUNCE_LIMIT 4

    
struct Photon
{
    Vec3f Pos;
    Vec3f Normal;
    Vec3f Dir;
};

struct GPURay
{
    Vec3f O;
    float tmax;
    Vec3f D;
    unsigned int _pad;
};

struct HitInfo
{
    float hit_u;
    float hit_v;
    unsigned int hit_id;
    float tmax;
};

struct GPUNode
{
    float min[3];
    unsigned int offs;
    float max[3];
    unsigned int count_and_axis;
};

struct TrianglePP
{
    Vec3f P0;
    Vec3f v02;
    Vec3f v10;
    Vec3f v10x02;
};


struct Tracer
{
    HAXWell::ShaderHandle hShader;
    HAXWell::BufferHandle hRays;
    HAXWell::BufferHandle hHits;
    HAXWell::BufferHandle hVerts;
    HAXWell::BufferHandle hIndices;
    HAXWell::BufferHandle hNodes;
    HAXWell::BufferHandle hTrianglePP;

    void* pMappedRayBuffer;
    void* pMappedHitBuffer;

    size_t nRaysPerGroup;
    Simpleton::PlyMesh ply;

    Vec3f* pTriNormals;

};

void InitTracer( Tracer& scene, float sah )
{
    Simpleton::PlyMesh ply;
    if( !Simpleton::LoadPly( "raytracer/kitchen.ply", ply, 0 ) )
    {
        printf("Scene not found!");
        exit(1);
    }

    scene.ply = ply;

    typedef TinyRT::BasicMesh<TinyRT::Vec3f,unsigned int> Mesh;
    typedef TinyRT::AABBTree<Mesh> BVH;
    TinyRT::SahAABBTreeBuilder<Mesh> builder(sah);

    Mesh mesh((TinyRT::Vec3f*)ply.pPositions, ply.pVertexIndices, ply.nVertices, ply.nTriangles );
    
    BVH aabb;
    aabb.Build( &mesh,builder);

    unsigned int nNodes = aabb.GetNodeCount();

    // build GPU node structure from TinyRT structure
    GPUNode* pGPUNodes = new GPUNode[nNodes+1];
    BVH::Node* pNode = aabb.GetRoot();

    size_t nLeafs=0;
    size_t nLeafSum=0;
    for( unsigned int i=0; i<nNodes; i++ )
    {
        for( int k=0; k<3; k++ )
        {
            pGPUNodes[i].min[k] = pNode[i].GetAABB().Min()[k];
            pGPUNodes[i].max[k] = pNode[i].GetAABB().Max()[k];
        }

        if( pNode[i].IsLeaf() )
        {
            unsigned int start;
            unsigned int end;
            pNode[i].GetObjectRange(start,end);
            pGPUNodes[i].offs = start;
            pGPUNodes[i].count_and_axis = ((end-start)<<2) | 3; 
            nLeafs++;
            nLeafSum += end-start;
        }
        else
        {
            pGPUNodes[i].offs=pNode[i].GetLeftChildIndex();
            pGPUNodes[i].count_and_axis = pNode[i].GetSplitAxis();
        }
    }

    // kludge:  Insert a dummy node at position 1
    //  This way every pair of sibling nodes occupies the same cache line
    
    memmove( pGPUNodes+2, pGPUNodes+1, sizeof(GPUNode)*(nNodes-1) );
    for( size_t i=0; i<nNodes+1; i++ )
        if( (pGPUNodes[i].count_and_axis&3) != 3 )
            pGPUNodes[i].offs++;

    nNodes++;

    // preprocess triangles for intersection testing
    TrianglePP* pTris = new TrianglePP[ply.nTriangles];
    scene.pTriNormals = new Vec3f[ply.nTriangles];
    for( size_t i=0; i<ply.nTriangles; i++ )
    {
        Vec3f P0 = ply.pPositions[ ply.pVertexIndices[3*i+0] ];
        Vec3f P1 = ply.pPositions[ ply.pVertexIndices[3*i+1] ];
        Vec3f P2 = ply.pPositions[ ply.pVertexIndices[3*i+2] ];
        pTris[i].P0 = P0;
        pTris[i].v02 = P0 - P2;
        pTris[i].v10 = P1 - P0;
        pTris[i].v10x02 = TinyRT::Cross3( pTris[i].v10, pTris[i].v02 );
        scene.pTriNormals[i] = Normalize3(pTris[i].v10x02);
    }

    printf("Scene size: %.2f mb (%u tris)\n", (ply.nTriangles*sizeof(TrianglePP) + nNodes*sizeof(GPUNode))/(1024.0*1024.0), ply.nTriangles );
    printf("Mean tris/leaf: %.2f\n", (double)nLeafSum / (double)nLeafs );
    scene.hNodes   = HAXWell::CreateBuffer( pGPUNodes, sizeof(GPUNode)*nNodes );
    scene.hVerts   = HAXWell::CreateBuffer( ply.pPositions, 3*sizeof(float)*ply.nVertices );
    scene.hIndices = HAXWell::CreateBuffer( ply.pVertexIndices, 3*sizeof(unsigned int)*ply.nTriangles );
    scene.hRays    = HAXWell::CreateBuffer( 0, sizeof(GPURay)*PACKET_SIZE + 16 );
    scene.hHits    = HAXWell::CreateBuffer( 0, sizeof(HitInfo)*PACKET_SIZE);
    scene.hTrianglePP    = HAXWell::CreateBuffer( pTris, sizeof(TrianglePP)*ply.nTriangles);
    
    delete[]pTris;
    delete[]pGPUNodes;

    scene.pMappedHitBuffer = HAXWell::MapBuffer(scene.hHits);
    scene.pMappedRayBuffer = HAXWell::MapBuffer(scene.hRays);
}



static void CopyDestStreaming( void* pOutput, const void* pInput, size_t nBytes )
{
    const char* pIn = (const char*) pInput;
    char* pOut = (char*) pOutput;
    for( size_t i=0; i<nBytes; i += 64 ) 
    {
        float* pfOut = (float*)(pOut+i);
        const float* pfIn = (const float*)(pIn+i);

        __m128 v0 = _mm_load_ps(pfIn+0) ;
        __m128 v1 = _mm_load_ps(pfIn+4) ;
        __m128 v2 = _mm_load_ps(pfIn+8) ;
        __m128 v3 = _mm_load_ps(pfIn+12);
        _mm_stream_ps( pfOut+0,  v0 ); 
        _mm_stream_ps( pfOut+4,  v1 ); 
        _mm_stream_ps( pfOut+8,  v2 ); 
        _mm_stream_ps( pfOut+12, v3 ); 
    }
    _mm_sfence();
}

static void CopySourceStreaming( void* pOutput, const void* pInput, size_t nBytes )
{

    const char* pIn = (const char*) pInput;
    char* pOut = (char*) pOutput;
    for( size_t i=0; i<nBytes; i += 64 ) 
    {
        __m128i* pvOut = (__m128i*) (pOut+i);
        __m128i* pvIn  = (__m128i*) (pIn+i);
        __m128i v0 = _mm_stream_load_si128( pvIn+0 );
        __m128i v1 = _mm_stream_load_si128( pvIn+1 );
        __m128i v2 = _mm_stream_load_si128( pvIn+2 );
        __m128i v3 = _mm_stream_load_si128( pvIn+3 );
        _mm_store_si128( pvOut+0, v0 ); 
        _mm_store_si128( pvOut+1, v1 ); 
        _mm_store_si128( pvOut+2, v2 ); 
        _mm_store_si128( pvOut+3, v3 ); 
    }
}



static void DebugDump(char* pWhere, Photon* pPhotons)
{
    printf("Making debug image...\n");
    Simpleton::Vec3f vCameraPosition(3.750000, 2.100000, 3.600000);
    Simpleton::Vec3f vLookAt( 3.436536, 1.880186, 2.748239 );

    Simpleton::Matrix4f mView = Simpleton::MatrixLookAtLH(vCameraPosition, vLookAt, Simpleton::Vec3f(0,1,0) );
    Simpleton::Matrix4f mProj = Simpleton::MatrixPerspectiveFovLH( 1.0f, 60.0f, 1, 100 );
   
    Simpleton::Matrix4f mViewProj = mProj*mView;
    Simpleton::PPMImage img(512,512);
    for( int i=0; i<img.GetHeight(); i++ )
        for( int j=0; j<img.GetWidth(); j++ )
            img.SetPixel(i,j,0.0f,0.0f,0.0f);

    for( uint i=0; i<NUM_PHOTONS; i++ )
    {
        Simpleton::Vec3f view = Simpleton::AffineTransformPoint(mView,pPhotons[i].Pos );
        if( view.z < 0 )
            continue;

        float diff = -Simpleton::Dot3( pPhotons[i].Dir, pPhotons[i].Normal);
        diff = Simpleton::Max(diff*0.1f,0.0f);
       
        Vec3f vec = Simpleton::TransformPoint(mViewProj,pPhotons[i].Pos);
        float u = vec.x*0.5f + 0.5f;
        float v = (-vec.y)*0.5f + 0.5f;
        if(  u > 0 && u < 1 && v > 0 && v < 1 )
        {
            float rgb[3];
            int x = u*img.GetWidth();
            int y = v*img.GetHeight();
            img.GetPixel(x,y,rgb);
            for( int j=0; j<3; j++ )
                rgb[j] = Simpleton::Min(1.0f,rgb[j]+diff) ;
            img.SetPixel(x,y,rgb[0],rgb[1],rgb[2]);
        }
    }
    img.SaveFile(pWhere);
}










class RayQueue
{
public:

    RayQueue( Tracer* pTracer ) 
        : m_pTracer(pTracer), m_nFirstBucket(0), m_nLastBucket(0), 
          m_nInputRays(0), m_nOutputUsed(0), m_nOutputRays(0),
          m_nInFlight(0), m_WaitTime(0), m_GPUDuration(0)
    {
        m_pInputRays  = (GPURay*)_aligned_malloc( BUCKET_SIZE*sizeof(GPURay), 16 );
        m_pOutputRays = (GPURay*)_aligned_malloc( BUCKET_SIZE*sizeof(GPURay), 16 );
        m_pOutputHits = (HitInfo*)_aligned_malloc( BUCKET_SIZE*sizeof(HitInfo), 16 );

        for( size_t i=0; i<BUCKET_COUNT; i++ )
        {
            m_pBuckets[i].hFence = 0;
            m_pBuckets[i].nRays = 0;
            m_pBuckets[i].hHitInfoBuffer = HAXWell::CreateBuffer(0,sizeof(HitInfo)*BUCKET_SIZE);
            m_pBuckets[i].hRayBuffer = HAXWell::CreateBuffer(0, sizeof(GPURay)*BUCKET_SIZE + 16 );
            
            char* pMappedRays = (char*) HAXWell::MapBuffer(m_pBuckets[i].hRayBuffer);
            char* pMappedHit  = (char*) HAXWell::MapBuffer(m_pBuckets[i].hHitInfoBuffer);
            m_pBuckets[i].pMappedHitInfo  = (HitInfo*)pMappedHit;
            m_pBuckets[i].pMappedRays     = (GPURay*)(pMappedRays+16);
            m_pBuckets[i].pMappedRayCount = (unsigned int*)pMappedRays;
            m_pBuckets[i].pCPURayBuffer = (GPURay*)_aligned_malloc( BUCKET_SIZE*sizeof(GPURay), 16 );
        }
    }

    void PushRay( GPURay& r )
    {
        m_pInputRays[m_nInputRays++] = r;
        if( m_nInputRays == BUCKET_SIZE )
            FlushRays();

        m_nInFlight++;
        assert( m_nInFlight <= PACKET_SIZE );

    }

    void PopRay( GPURay* pR, HitInfo* pHit )
    {
        assert( m_nInFlight);

        // rays left in staging buffer?
        if( m_nOutputUsed < m_nOutputRays )
        {
            // get one
            *pR   = m_pOutputRays[m_nOutputUsed];
            *pHit = m_pOutputHits[m_nOutputUsed];
            m_nOutputUsed++;
            m_nInFlight--;
            return;
        }

        // finish next batch and try again
        Bucket* pPendingBucket = &m_pBuckets[m_nFirstBucket % BUCKET_COUNT ];
        m_nFirstBucket++;

        LARGE_INTEGER ts,te;
        QueryPerformanceCounter(&ts);
        HAXWell::WaitFence(pPendingBucket->hFence);
        QueryPerformanceCounter(&te);

        m_GPUDuration += HAXWell::ReadTimer(pPendingBucket->hTimer) / (1000000000.0);;
        m_WaitTime += te.QuadPart - ts.QuadPart;


        pPendingBucket->hFence=0;

        size_t nRays = pPendingBucket->nRays;
        std::swap( m_pOutputRays, pPendingBucket->pCPURayBuffer );

        CopySourceStreaming( m_pOutputHits,  pPendingBucket->pMappedHitInfo, nRays*sizeof(HitInfo));

        m_nOutputUsed = 0;
        m_nOutputRays = nRays;

        PopRay(pR,pHit);
    }

    void FlushRays()
    {
        if( !m_nInputRays )
            return;

        Bucket* pNewBucket = &m_pBuckets[m_nLastBucket % BUCKET_COUNT];
        m_nLastBucket++;

        *(pNewBucket->pMappedRayCount) = m_nInputRays;
        CopyDestStreaming( pNewBucket->pMappedRays, m_pInputRays, sizeof(GPURay)*m_nInputRays );
        
        
        size_t nGroups = m_nInputRays / m_pTracer->nRaysPerGroup;
        if( nGroups*m_pTracer->nRaysPerGroup < m_nInputRays )
            nGroups++;

        HAXWell::BufferHandle pBuffers[] = {
            pNewBucket->hRayBuffer,
            pNewBucket->hHitInfoBuffer,
            m_pTracer->hNodes,
            m_pTracer->hTrianglePP,
        };
        pNewBucket->hTimer = HAXWell::BeginTimer();
        pNewBucket->hFence = HAXWell::BeginFence();
        pNewBucket->nRays = m_nInputRays;
        std::swap( pNewBucket->pCPURayBuffer, m_pInputRays );
        HAXWell::DispatchShader( m_pTracer->hShader, pBuffers, 4, nGroups );
        HAXWell::EndTimer(pNewBucket->hTimer);
        HAXWell::Flush();
        
        m_nInputRays=0;
    }

    uint64 m_WaitTime;
    double m_GPUDuration;
    
    enum
    {
        BUCKET_COUNT = PACKET_SIZE/BUCKET_SIZE
    };

private:

   
    struct Bucket
    {
        HAXWell::BufferHandle hRayBuffer;
        HAXWell::BufferHandle hHitInfoBuffer;
        HAXWell::FenceHandle hFence;
        HAXWell::TimerHandle hTimer;

        size_t nRays;
        unsigned int* pMappedRayCount;
        GPURay* pMappedRays;
        HitInfo* pMappedHitInfo;
        GPURay* pCPURayBuffer;


    };

    Tracer* m_pTracer;
    
    Bucket m_pBuckets[BUCKET_COUNT];
    size_t m_nFirstBucket;
    size_t m_nLastBucket;

    GPURay* m_pInputRays;
    size_t m_nInputRays;

    GPURay*  m_pOutputRays;
    HitInfo* m_pOutputHits;
    size_t m_nOutputRays;
    size_t m_nOutputUsed;

    size_t m_nInFlight;
};




static void HarnessAsync( Tracer& scene, Photon* pPhotons, HAXWell::ShaderHandle hShader, size_t nRaysPerGroup )
{
    printf("Pack size: %u.  Bucket: %u\n", PACKET_SIZE, BUCKET_SIZE );

    Simpleton::PlyMesh& ply = scene.ply;

    srand(0);
    
    size_t nPhotons=0;

    float LIGHT_SIZE = 0.5f;
    Vec3f vLightCenter = Vec3f(2.15,2.1,2.15);
    Vec3f vLightCorners[4] = { vLightCenter + Vec3f(-LIGHT_SIZE,0,LIGHT_SIZE),
                               vLightCenter + Vec3f(LIGHT_SIZE,0,LIGHT_SIZE),
                               vLightCenter + Vec3f(LIGHT_SIZE,0,-LIGHT_SIZE),
                               vLightCenter + Vec3f(-LIGHT_SIZE,0,-LIGHT_SIZE) };


    long nTotalRays =0;
    long nPackets=0;
    double fTraceTime = 0;
   
    LARGE_INTEGER tstart;
    LARGE_INTEGER ttrace;
    ttrace.QuadPart=0;
    tstart.QuadPart=0;
    QueryPerformanceCounter(&tstart);

    RayQueue q(&scene);
    
    size_t k=0;
    while( nPhotons < NUM_PHOTONS && nTotalRays < NUM_PHOTONS*5 )
    {
        // generate rays
        for( size_t i=0; i<PACKET_SIZE; i++ )
        {
            float u  = Simpleton::Rand();
            float v  = Simpleton::Rand();
            Vec3f v0 = TinyRT::Lerp3(
                            TinyRT::Lerp3( vLightCorners[0], vLightCorners[1], u ),
                            TinyRT::Lerp3( vLightCorners[2], vLightCorners[3], u ), v );

            float du  = Simpleton::Rand();
            float dv  = Simpleton::Rand();
            Vec3f dir = TinyRT::UniformSampleHemisphere(du,dv);
            dir.y *= -1;

            GPURay r;
            r.O = v0;
            r.D = dir;
            r.tmax = 99999999;

            q.PushRay(r);
        }

        
        size_t nRays = PACKET_SIZE;
        size_t nBounce = 0;
        do
        {
            q.FlushRays();
        

            // for each ray, store photon hits 
            size_t nFirstPhoton = nPhotons;

            size_t nParentRays = nRays;
            nTotalRays += nParentRays;
            nPackets++;
            for( size_t r=0; r<nParentRays; r++ )
            {
                HitInfo hit;
                GPURay ray;
                q.PopRay(&ray,&hit);

                if( hit.hit_id != 0xffffffff )
                {
                    pPhotons[nPhotons].Pos.x = ray.O[0] + hit.tmax*ray.D[0];
                    pPhotons[nPhotons].Pos.y = ray.O[1] + hit.tmax*ray.D[1];
                    pPhotons[nPhotons].Pos.z = ray.O[2] + hit.tmax*ray.D[2];
                    pPhotons[nPhotons].Dir.x = ray.D[0];
                    pPhotons[nPhotons].Dir.y = ray.D[1];
                    pPhotons[nPhotons].Dir.z = ray.D[2];

                    size_t id = hit.hit_id;
                    pPhotons[nPhotons].Normal = scene.pTriNormals[id];;
                    
                    /*
                    Vec3f v0 = Vec3f(ply.pPositions[ ply.pVertexIndices[3*id+0] ] );
                    Vec3f v1 = Vec3f(ply.pPositions[ ply.pVertexIndices[3*id+1] ] );
                    Vec3f v2 = Vec3f(ply.pPositions[ ply.pVertexIndices[3*id+2] ] );
                    Vec3f N = Simpleton::Normalize3( Simpleton::Cross3( v1-v0, v2-v0 ) );
                    pPhotons[nPhotons].Normal = N;
                    */
                    
                    nPhotons++;

                    if( nPhotons == NUM_PHOTONS )
                        break;
                }
            }

            if( nPhotons == NUM_PHOTONS )
                break;
            
            nBounce++;
            if( nBounce < BOUNCE_LIMIT )
            {
                // create secondary rays
                nRays=0;
                for( size_t p=nFirstPhoton; p<nPhotons; p++ )
                {
                    float rng = Simpleton::Rand();
                    if( rng < 0.7f )
                    {
                        float s = Simpleton::Rand();
                        float t = Simpleton::Rand();
                        Vec3f v = Simpleton::UniformSampleHemisphere(s,t);
                        Vec3f T,B;
                        Simpleton::BuildTangentFrame( pPhotons[p].Normal, T, B );
                        Vec3f Dir = (pPhotons[p].Normal * v.z) +
                                    (T * v.x) +
                                    (B * v.y) ;

                        GPURay r;
                        r.O[0] = Dir.x*0.0001f + pPhotons[p].Pos.x;
                        r.O[1] = Dir.y*0.0001f + pPhotons[p].Pos.y;
                        r.O[2] = Dir.z*0.0001f + pPhotons[p].Pos.z;
                        r.D[0] = Dir.x;
                        r.D[1] = Dir.y;
                        r.D[2] = Dir.z;
                        r.tmax = 999999999999;

                        q.PushRay(r);
                        nRays++;
                    }
                }
            }

        }while( nBounce < BOUNCE_LIMIT );
    }
    
    LARGE_INTEGER tend;
    LARGE_INTEGER freq;
    QueryPerformanceCounter(&tend);
    QueryPerformanceFrequency(&freq);

    double fTotalTime = (tend.QuadPart-tstart.QuadPart)/(double)freq.QuadPart;

    double fWaitTime = q.m_WaitTime / (double)freq.QuadPart;
    double fKRays = (nTotalRays/1000.0) / fTotalTime;
    double fKRaysAdjusted = (nTotalRays/1000.0) / fTraceTime;
    
    
    printf("Rays shot: %.2fM.  Packs shot: %u\n", nTotalRays/(1000000.0), nPackets );
    printf("Mean rays/pack: %.2f\n", nTotalRays / (double)nPackets );
    printf("total: %.2f s (%.2f KRays/s) \n", fTotalTime, fKRays );
    printf("wait time: %.2f s (%.2f%%)\n", fWaitTime, 100.0*(fWaitTime/fTotalTime) );
    printf("gpu duration: %.2f s (%.2f%%)\n", q.m_GPUDuration, 100.0*(q.m_GPUDuration/fTotalTime) );

//    printf("trace: %.2f s (%.2f KRays/s). (%.2f ratio)\n", fTraceTime, fKRaysAdjusted, fTraceTime/fTotalTime );
}






static void HarnessAsyncInterleaved( Tracer& scene, Photon* pPhotons, HAXWell::ShaderHandle hShader, size_t nRaysPerGroup )
{
    printf("Pack size: %u.  Bucket: %u\n", PACKET_SIZE, BUCKET_SIZE );

    Simpleton::PlyMesh& ply = scene.ply;

    srand(0);
    
    size_t nPhotons=0;

    float LIGHT_SIZE = 0.5f;
    Vec3f vLightCenter = Vec3f(2.15,2.1,2.15);
    Vec3f vLightCorners[4] = { vLightCenter + Vec3f(-LIGHT_SIZE,0,LIGHT_SIZE),
                               vLightCenter + Vec3f(LIGHT_SIZE,0,LIGHT_SIZE),
                               vLightCenter + Vec3f(LIGHT_SIZE,0,-LIGHT_SIZE),
                               vLightCenter + Vec3f(-LIGHT_SIZE,0,-LIGHT_SIZE) };


    long nTotalRays =0;
    double fTraceTime = 0;
   
    LARGE_INTEGER tstart;
    LARGE_INTEGER ttrace;
    ttrace.QuadPart=0;
    tstart.QuadPart=0;
    QueryPerformanceCounter(&tstart);

    RayQueue q(&scene);

    // prime ray queue
    for( size_t i=0; i<PACKET_SIZE; i++ )
    {
        float u  = Simpleton::Rand();
        float v  = Simpleton::Rand();
        Vec3f v0 = TinyRT::Lerp3(
                        TinyRT::Lerp3( vLightCorners[0], vLightCorners[1], u ),
                        TinyRT::Lerp3( vLightCorners[2], vLightCorners[3], u ), v );

        float du  = Simpleton::Rand();
        float dv  = Simpleton::Rand();
        Vec3f dir = TinyRT::UniformSampleHemisphere(du,dv);
        dir.y *= -1;

        GPURay r;
        r.O = v0;
        r.D = dir;
        r.tmax = 99999999;
        r._pad = 0;

        q.PushRay(r);
        nTotalRays++;
    }

    while( nPhotons < NUM_PHOTONS && nTotalRays < NUM_PHOTONS*10 )
    {
        GPURay ray;
        HitInfo hit;
        q.PopRay(&ray,&hit);

        if( hit.hit_id != 0xffffffff )
        {
            size_t id = hit.hit_id;
            Vec3f N = scene.pTriNormals[id];
            Vec3f P = Vec3f(
                 ray.O[0] + hit.tmax*ray.D[0],
                 ray.O[1] + hit.tmax*ray.D[1],
                 ray.O[2] + hit.tmax*ray.D[2] );

            pPhotons[nPhotons].Pos = P;
            pPhotons[nPhotons].Dir.x = ray.D[0];
            pPhotons[nPhotons].Dir.y = ray.D[1];
            pPhotons[nPhotons].Dir.z = ray.D[2];
            pPhotons[nPhotons].Normal = N;
            
                
            // Re-compute normals instead of using cached ones...
            //Vec3f v0 = Vec3f(ply.pPositions[ ply.pVertexIndices[3*id+0] ] );
            //Vec3f v1 = Vec3f(ply.pPositions[ ply.pVertexIndices[3*id+1] ] );
            //Vec3f v2 = Vec3f(ply.pPositions[ ply.pVertexIndices[3*id+2] ] );
            //Vec3f N = Simpleton::Normalize3( Simpleton::Cross3( v1-v0, v2-v0 ) );
            //pPhotons[nPhotons].Normal = N;
            
                    
            nPhotons++;

            if( nPhotons == NUM_PHOTONS )
                break;

            if( ray._pad < BOUNCE_LIMIT )
            {
                float rng = Simpleton::Rand();
                if( rng < 0.7f )
                {
                    // shoot a secondary ray
                    float s = Simpleton::Rand();
                    float t = Simpleton::Rand();
                    Vec3f v = Simpleton::UniformSampleHemisphere(s,t);
                    Vec3f T,B;
                    Simpleton::BuildTangentFrame( N, T, B );
                    Vec3f Dir = (N * v.z) +
                                (T * v.x) +
                                (B * v.y) ;

                    GPURay r;
                    r.O[0] = Dir.x*0.0001f + P.x;
                    r.O[1] = Dir.y*0.0001f + P.y;
                    r.O[2] = Dir.z*0.0001f + P.z;
                    r.D[0] = Dir.x;
                    r.D[1] = Dir.y;
                    r.D[2] = Dir.z;
                    r.tmax = 999999999999;
                    r._pad = ray._pad+1;

                    q.PushRay(r);
                    nTotalRays++;
                    continue;
                }
            }
        }

        if( nPhotons == NUM_PHOTONS )
            break;

        // start a new photon path to replace this ray
        float u  = Simpleton::Rand();
        float v  = Simpleton::Rand();
        Vec3f v0 = TinyRT::Lerp3(
                        TinyRT::Lerp3( vLightCorners[0], vLightCorners[1], u ),
                        TinyRT::Lerp3( vLightCorners[2], vLightCorners[3], u ), v );

        float du  = Simpleton::Rand();
        float dv  = Simpleton::Rand();
        Vec3f dir = TinyRT::UniformSampleHemisphere(du,dv);
        dir.y *= -1;

        GPURay r;
        r.O = v0;
        r.D = dir;
        r.tmax = 99999999;
        r._pad = 0;

        q.PushRay(r);
        nTotalRays++;
    }


    
    
    LARGE_INTEGER tend;
    LARGE_INTEGER freq;
    QueryPerformanceCounter(&tend);
    QueryPerformanceFrequency(&freq);

    double fTotalTime = (tend.QuadPart-tstart.QuadPart)/(double)freq.QuadPart;

    double fWaitTime = q.m_WaitTime / (double)freq.QuadPart;
    double fKRays = (nTotalRays/1000.0) / fTotalTime;
    double fKRaysAdjusted = (nTotalRays/1000.0) / fTraceTime;
    

    printf("Rays shot: %.2fM. \n", nTotalRays/(1000000.0) );
    printf("total: %.2f s (%.2f KRays/s) \n", fTotalTime, fKRays );
    printf("wait time: %.2f s (%.2f%%)\n", fWaitTime, 100.0*(fWaitTime/fTotalTime) );
    printf("gpu duration: %.2f s (%.2f%%)\n", q.m_GPUDuration, 100.0*(q.m_GPUDuration/fTotalTime) );
    printf("gpu ray-rate: %.2f KRays/s\n", (nTotalRays/1000.0) / q.m_GPUDuration );

//    printf("trace: %.2f s (%.2f KRays/s). (%.2f ratio)\n", fTraceTime, fKRaysAdjusted, fTraceTime/fTotalTime );
}



void RaytraceHarness( HAXWell::ShaderHandle hShader, size_t nRaysPerGroup, float sah )
{
    Tracer tr;
    InitTracer(tr,sah);
    tr.hShader = hShader;
    tr.nRaysPerGroup = nRaysPerGroup;

    Photon* pPhotons = new Photon[NUM_PHOTONS];
    HarnessAsyncInterleaved( tr, pPhotons, hShader, nRaysPerGroup );

    DebugDump("dump.ppm", pPhotons);
    delete[] pPhotons;
}