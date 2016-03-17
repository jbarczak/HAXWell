
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "GENAssembler.h"

#include "HAXWell.h"
#include "Misc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#define GLSL(...) "#version 430 core\n" #__VA_ARGS__
#define STRINGIFY(...) #__VA_ARGS__

const char* NBODYGLSL = GLSL(
    
    layout (local_size_x = 16) in;

        layout (std430,binding=0)
        buffer Output0
        {
           uint g_nBodies;
           float g_TimeStep;
           float g_GraviationConst;
        };

        layout (std430,binding=1)
        buffer BodiesIn
        {
           vec4 g_PositionAndMass[];
        };
        layout (std430,binding=2)
        buffer VelocitiesIn
        {
           vec4 g_Velocity[];
        };

        
        layout (std430,binding=3)
        buffer BodiesOut
        {
           vec4 g_PositionAndMassOut[];
        };
        layout (std430,binding=4)
        buffer VelocitiesOut
        {
           vec4 g_VelocityOut[];
        };


        void main() 
        {
            uint tid = gl_GlobalInvocationID.x;
            
            vec4 body  = g_PositionAndMass[tid];
            vec3 force = vec3(0.f,0.f,0.f);
            for( uint i=0; i<g_nBodies; i++ )
            {
                if( i != tid )
                {
                    vec4 otherBody = g_PositionAndMass[i];
                    vec3 D = body.xyz - otherBody.xyz;

                    // force = G*massi*massj * (D ) / length(D)^3
                    // length(D)^3 is dot(D,D) * sqrt(dot(D,D))
                    float lengthSq = dot( D, D );
                    float lengthCubed = lengthSq * sqrt(lengthSq);

                    float scale = ( g_GraviationConst * otherBody.w) / lengthCubed;
                    force += D*scale;
                }
            }

            vec3 velocity = g_Velocity[tid].xyz;
            vec3 accel    = force; // mass cancels out

            velocity += accel * g_TimeStep;
            body.xyz += velocity * g_TimeStep;

            g_PositionAndMassOut[tid].xyz = body.xyz;
            g_VelocityOut[tid].xyz = velocity.xyz;
        }


    );


#define SM_SIZE 256
const char* NBODYGLSL_SM = GLSL(
    
    layout (local_size_x = 256) in;

        layout (std430,binding=0)
        buffer Output0
        {
           uint g_nBodies;
           float g_TimeStep;
           float g_GraviationConst;
        };

        layout (std430,binding=1)
        buffer BodiesIn
        {
           vec4 g_PositionAndMass[];
        };
        layout (std430,binding=2)
        buffer VelocitiesIn
        {
           vec4 g_Velocity[];
        };

        
        layout (std430,binding=3)
        buffer BodiesOut
        {
           vec4 g_PositionAndMassOut[];
        };
        layout (std430,binding=4)
        buffer VelocitiesOut
        {
           vec4 g_VelocityOut[];
        };

        shared vec4 SHARED_Bodies[256];

        void main() 
        {
            uint tid = gl_GlobalInvocationID.x;
            uint tid_local = gl_LocalInvocationID.x;
            
            vec4 body  = g_PositionAndMass[tid];
            vec3 force = vec3(0.f,0.f,0.f);

            for( uint i=0; i<g_nBodies; i+= 256 )
            {
                // load a bunch of bodies into SM across the thread group
                vec4 other = g_PositionAndMass[i+tid_local];
                other.w = g_GraviationConst*other.w;
                SHARED_Bodies[tid_local] = other;
                memoryBarrierShared();

                // now add up all the forces
                for( uint j=0; j<256;j++ )
                {
                    if( (i+j) != tid )
                    {
                        vec4 otherBody = SHARED_Bodies[j];
                        vec3 D = body.xyz - otherBody.xyz;

                        // force = G*massi*massj * (D ) / length(D)^3
                        // length(D)^3 is dot(D,D) * sqrt(dot(D,D))
                        float lengthSq = dot( D, D );
                        float lengthCubed = lengthSq * sqrt(lengthSq);

                        float scale = (  otherBody.w) / lengthCubed;
                        force += D*scale;
                    }
                }
                memoryBarrierShared();

            }

            vec3 velocity = g_Velocity[tid].xyz;
            vec3 accel    = force; // mass cancels out

            velocity += accel * g_TimeStep;
            body.xyz += velocity * g_TimeStep;

            g_PositionAndMassOut[tid].xyz = body.xyz;
            g_VelocityOut[tid].xyz = velocity.xyz;
        }


    );




float Rnd()
{
    return (float)rand() / (float)RAND_MAX;
}


struct Simulator
{
    HAXWell::ShaderHandle hKernel;
    size_t nBodiesPerThreadGroup;
    size_t nBodies;
    size_t nSteps;
    float dt;
    float GraviationalConstant;
};


float* Nbody( Simulator& sim )
{
    srand(4);

    float dt =sim.dt;
    size_t nSteps = sim.nSteps;
    size_t nBodies = sim.nBodies;

    // make some positions
    float* pPositions = new float[4*sim.nBodies];
    float* pMassInit  = new float[4*sim.nBodies];
    float* pVelocities = new float[4*sim.nBodies];
    for( size_t i=0; i<nBodies; i++ )
    {
        pPositions[4*i]   = 100.0f*Rnd();
        pPositions[4*i+1] = 100.0f*Rnd();
        pPositions[4*i+2] = 100.0f*Rnd();
        pPositions[4*i+3] = 0.5f + Rnd()*5.0f;

        pMassInit[4*i]   = 0;
        pMassInit[4*i+1] = 0;
        pMassInit[4*i+2] = 0;
        pMassInit[4*i+3] = pPositions[4*i+3];

    }
    memset( pVelocities,0,sizeof(float)*4*nBodies );
    

    HAXWell::BufferHandle hPositions[2] ={
        HAXWell::CreateBuffer( pPositions, 4*sizeof(float)*nBodies ),
        HAXWell::CreateBuffer( pMassInit, 4*sizeof(float)*nBodies )
    };

    HAXWell::BufferHandle hVelocities[2] ={
        HAXWell::CreateBuffer( pVelocities, 4*sizeof(float)*nBodies ),
        HAXWell::CreateBuffer( pVelocities, 4*sizeof(float)*nBodies )
    };

    struct Globals
    {
        unsigned int nBodies;
        float fTimeStep;
        float fGravitationalConstant;
    };

    Globals g;
    g.nBodies = nBodies;
    g.fTimeStep = dt;
    g.fGravitationalConstant = sim.GraviationalConstant;
    HAXWell::BufferHandle hGlobals = HAXWell::CreateBuffer(&g,sizeof(g));

    HAXWell::Finish(); // flush buffer creation

    HAXWell::TimerHandle hTimer = HAXWell::BeginTimer();

    for( size_t j=0; j<nSteps; j++ )
    {
        HAXWell::BufferHandle hDispatch[5] ={
            hGlobals,
            hPositions[0],
            hVelocities[0],
            hPositions[1],
            hVelocities[1]
        };
            
        HAXWell::DispatchShader( sim.hKernel, hDispatch, 5, nBodies/sim.nBodiesPerThreadGroup );

        std::swap(hPositions[0],hPositions[1]);
        std::swap(hVelocities[0],hVelocities[1]);
        HAXWell::Finish();// need a 'finish' in here, or else things go wrong in the Haxwell version.  Driver bug?
    }

    HAXWell::EndTimer(hTimer);
    HAXWell::Finish(); // flush compute
    HAXWell::timer_t nTime = HAXWell::ReadTimer(hTimer);

    printf("%012u\n", nTime);
    delete[] pVelocities;
    delete[] pMassInit;

    void* pResults = HAXWell::MapBuffer(hPositions[0]);
    memcpy( pPositions, pResults, 4*sizeof(float)*nBodies );
    HAXWell::UnmapBuffer(hPositions[0]);

    HAXWell::ReleaseBuffer(hPositions[0]);
    HAXWell::ReleaseBuffer(hPositions[1]);
    HAXWell::ReleaseBuffer(hVelocities[0]);
    HAXWell::ReleaseBuffer(hVelocities[1]);
    HAXWell::ReleaseBuffer(hGlobals);

    return pPositions;
}







static const char* NBODY_HAXWELL = STRINGIFY(

    curbe OFFSETS[2] = {{0,4,8,12,16,20,24,28},{32,36,40,44,48,52,56,60}}

curbe INDICES[1] = {0,1,2,3,4,5,6,7}
threads 64 // launch 64 HW threads at once

reg globals
reg i
reg base[2]
reg addrx[4]
reg addry[4]
reg addrz[4]
reg X   [8]
reg Y   [8]
reg Z   [8]
reg VX  [8]
reg VY  [8]
reg VZ  [8]
reg Fx  [8]
reg Fy  [8]
reg Fz  [8]

reg Dx0 [2]
reg Dx1 [2]
reg Dy0 [2]
reg Dy1 [2]
reg Dz0 [2]
reg Dz1 [2]
reg lengthSq0 [2]
reg lengthSq1 [2]
reg length0[2]
reg length1[2]
reg BodyPair

bind Globals        0x38  // {#bodies,timestep,gravitation}
bind PositionIn     0x39    // xyzm
bind VelocityIn     0x3a    // xyz-
bind PositionOut    0x3b    // xyzm
bind VelocityOut    0x3c

// buffer bind points:  bind 0:  xyzm per body, bind 1:  velocity (xyz) per body
//  
// threads evaluate M bodies each

begin:

// Load blocks of body positions which this thread will process
// Load velocities up front as well.  We've got plenty of regs and we know we'll eventually need them

mul(16)  base.u, r0.u1<0,1,0>, 256        // base address for this thread's bodies (64 bodies * 4 dwords)
add(16)  addrx.u, base.u, OFFSETS.u     // x addresses for first 16-body group 
add(16)  addry.u, addrx.u, 1            // y addresses for first 16-body group 
add(16)  addrz.u, addrx.u, 2            // z addresses for first 16-body group 
send     DwordLoad16(PositionIn), X0.f, addrx.u       
send     DwordLoad16(PositionIn), Y0.f, addry.u       
send     DwordLoad16(PositionIn), Z0.f, addrz.u
send     DwordLoad16(VelocityIn), VX0.f, addrx.u       
send     DwordLoad16(VelocityIn), VY0.f, addry.u       
send     DwordLoad16(VelocityIn), VZ0.f, addrz.u
add(16)  addrx.u, addrx.u, 64       // skip 16 bodies (64 dwords)
add(16)  addry.u, addry.u, 64       // skip 16 bodies (64 dwords)    
add(16)  addrz.u, addrz.u, 64       // skip 16 bodies (64 dwords)   
send     DwordLoad16(PositionIn), X2.f, addrx.u       
send     DwordLoad16(PositionIn), Y2.f, addry.u       
send     DwordLoad16(PositionIn), Z2.f, addrz.u
send     DwordLoad16(VelocityIn), VX2.f, addrx.u       
send     DwordLoad16(VelocityIn), VY2.f, addry.u       
send     DwordLoad16(VelocityIn), VZ2.f, addrz.u
add(16)  addrx.u, addrx.u, 64  
add(16)  addry.u, addry.u, 64        
add(16)  addrz.u, addrz.u, 64       
send     DwordLoad16(PositionIn), X4.f, addrx.u       
send     DwordLoad16(PositionIn), Y4.f, addry.u       
send     DwordLoad16(PositionIn), Z4.f, addrz.u
send     DwordLoad16(VelocityIn), VX4.f, addrx.u       
send     DwordLoad16(VelocityIn), VY4.f, addry.u       
send     DwordLoad16(VelocityIn), VZ4.f, addrz.u
add(16)  addrx.u, addrx.u, 64  
add(16)  addry.u, addry.u, 64        
add(16)  addrz.u, addrz.u, 64       
send     DwordLoad16(PositionIn), X6.f, addrx.u       
send     DwordLoad16(PositionIn), Y6.f, addry.u       
send     DwordLoad16(PositionIn), Z6.f, addrz.u
send     DwordLoad16(VelocityIn), VX6.f, addrx.u       
send     DwordLoad16(VelocityIn), VY6.f, addry.u       
send     DwordLoad16(VelocityIn), VZ6.f, addrz.u


// load the global sim constants
send DwordLoad8(Globals), globals.u, INDICES.u 

// clear force accumulators
mov(16) Fx0.f, 0.0f
mov(16) Fx2.f, 0.0f
mov(16) Fx4.f, 0.0f
mov(16) Fx6.f, 0.0f
mov(16) Fy0.f, 0.0f
mov(16) Fy2.f, 0.0f
mov(16) Fy4.f, 0.0f
mov(16) Fy6.f, 0.0f
mov(16) Fz0.f, 0.0f
mov(16) Fz2.f, 0.0f
mov(16) Fz4.f, 0.0f
mov(16) Fz6.f, 0.0f


mov(1) f0.u, 0
mov(1) f1.u, 0
mov(8) i.u,  0    
cmpge(1)(f0.0) null.u, i.u0<1,1,1>, globals.u0<1,1,1>
jmpif(f0.0) write_out


loop:


mul(8) base.u, i.u<0,1,0>, 4
add(8) base.u, base.u<0,1,0>, INDICES.u
add(1) i.u0 , i.u0<1,1,1>, 2
send     DwordLoad8(PositionIn), BodyPair.f, base.u   
cmplt(1)(f0.0) null.u, i.u0<1,1,1>, globals.u0<1,1,1>


mul(1) BodyPair.f3, BodyPair.f3<1,1,1>, globals.f2<1,1,1> // pre-multiply body mass by gravitation constant
mul(1) BodyPair.f7, BodyPair.f7<1,1,1>, globals.f2<1,1,1> // TODO: Do in one SIMD4x2 op with write masking...?




//vec3 D = body.xyz - otherBody.xyz;
//float lengthSq = dot( D, D );
//float lengthCubed = lengthSq * sqrt(lengthSq);
//float scale = ( g_GraviationConst * otherBody.w) / lengthCubed;
//force += scale*D;

// Bodies 0-16
sub(16) Dx0.f, X0.f, BodyPair.f0<0,1,0>
sub(16) Dy0.f, Y0.f, BodyPair.f1<0,1,0>
sub(16) Dz0.f, Z0.f, BodyPair.f2<0,1,0>
sub(16) Dx1.f, X0.f, BodyPair.f4<0,1,0>
sub(16) Dy1.f, Y0.f, BodyPair.f5<0,1,0>
sub(16) Dz1.f, Z0.f, BodyPair.f6<0,1,0>
mul(16) lengthSq0.f, Dx0.f, Dx0.f
mul(16) lengthSq1.f, Dx1.f, Dx1.f
fma(16) lengthSq0.f, Dy0.f, Dy0.f
fma(16) lengthSq1.f, Dy1.f, Dy1.f
fma(16) lengthSq0.f, Dz0.f, Dz0.f
fma(16) lengthSq1.f, Dz1.f, Dz1.f
sqrt(16) length0.f, lengthSq0.f
sqrt(16) length1.f, lengthSq1.f
cmpgt(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpgt(16)(f1.1) null.u, lengthSq1.f, 0.0f

pred(f1.0){ mul(16) length0.f, length0.f, lengthSq0.f }
pred(f1.1){ mul(16) length1.f, length1.f, lengthSq1.f }
pred(f1.0){ rcp(16) length0.f, length0.f }
pred(f1.1){ rcp(16) length1.f, length1.f }
pred(f1.0){ mul(16) length0.f, length0.f, BodyPair.f3<0,1,0> }
pred(f1.1){ mul(16) length1.f, length1.f, BodyPair.f7<0,1,0> }
pred(f1.0){
    fma(16) Fx0.f, Dx0.f, length0.f
    fma(16) Fy0.f, Dy0.f, length0.f
    fma(16) Fz0.f, Dz0.f, length0.f
}
pred(f1.1){
    fma(16) Fx0.f, Dx1.f, length1.f
    fma(16) Fy0.f, Dy1.f, length1.f
    fma(16) Fz0.f, Dz1.f, length1.f
}


// Bodies 16-32
sub(16) Dx0.f, X2.f, BodyPair.f0<0,1,0>
sub(16) Dy0.f, Y2.f, BodyPair.f1<0,1,0>
sub(16) Dz0.f, Z2.f, BodyPair.f2<0,1,0>
sub(16) Dx1.f, X2.f, BodyPair.f4<0,1,0>
sub(16) Dy1.f, Y2.f, BodyPair.f5<0,1,0>
sub(16) Dz1.f, Z2.f, BodyPair.f6<0,1,0>
mul(16) lengthSq0.f, Dx0.f, Dx0.f
mul(16) lengthSq1.f, Dx1.f, Dx1.f
fma(16) lengthSq0.f, Dy0.f, Dy0.f
fma(16) lengthSq1.f, Dy1.f, Dy1.f
fma(16) lengthSq0.f, Dz0.f, Dz0.f
fma(16) lengthSq1.f, Dz1.f, Dz1.f
sqrt(16) length0.f, lengthSq0.f
sqrt(16) length1.f, lengthSq1.f
cmpgt(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpgt(16)(f1.1) null.u, lengthSq1.f, 0.0f

pred(f1.0){ mul(16) length0.f, length0.f, lengthSq0.f }
pred(f1.1){ mul(16) length1.f, length1.f, lengthSq1.f }
pred(f1.0){ rcp(16) length0.f, length0.f }
pred(f1.1){ rcp(16) length1.f, length1.f }
pred(f1.0){ mul(16) length0.f, length0.f, BodyPair.f3<0,1,0> }
pred(f1.1){ mul(16) length1.f, length1.f, BodyPair.f7<0,1,0> }
pred(f1.0){
    fma(16) Fx2.f, Dx0.f, length0.f
    fma(16) Fy2.f, Dy0.f, length0.f
    fma(16) Fz2.f, Dz0.f, length0.f
}
pred(f1.1){
    fma(16) Fx2.f, Dx1.f, length1.f
    fma(16) Fy2.f, Dy1.f, length1.f
    fma(16) Fz2.f, Dz1.f, length1.f
}

// Bodies 32-48
sub(16) Dx0.f, X4.f, BodyPair.f0<0,1,0>
sub(16) Dy0.f, Y4.f, BodyPair.f1<0,1,0>
sub(16) Dz0.f, Z4.f, BodyPair.f2<0,1,0>
sub(16) Dx1.f, X4.f, BodyPair.f4<0,1,0>
sub(16) Dy1.f, Y4.f, BodyPair.f5<0,1,0>
sub(16) Dz1.f, Z4.f, BodyPair.f6<0,1,0>
mul(16) lengthSq0.f, Dx0.f, Dx0.f
mul(16) lengthSq1.f, Dx1.f, Dx1.f
fma(16) lengthSq0.f, Dy0.f, Dy0.f
fma(16) lengthSq1.f, Dy1.f, Dy1.f
fma(16) lengthSq0.f, Dz0.f, Dz0.f
fma(16) lengthSq1.f, Dz1.f, Dz1.f
sqrt(16) length0.f, lengthSq0.f
sqrt(16) length1.f, lengthSq1.f
cmpgt(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpgt(16)(f1.1) null.u, lengthSq1.f, 0.0f

pred(f1.0){ mul(16) length0.f, length0.f, lengthSq0.f }
pred(f1.1){ mul(16) length1.f, length1.f, lengthSq1.f }
pred(f1.0){ rcp(16) length0.f, length0.f }
pred(f1.1){ rcp(16) length1.f, length1.f }
pred(f1.0){ mul(16) length0.f, length0.f, BodyPair.f3<0,1,0> }
pred(f1.1){ mul(16) length1.f, length1.f, BodyPair.f7<0,1,0> }
pred(f1.0){
    fma(16) Fx4.f, Dx0.f, length0.f
    fma(16) Fy4.f, Dy0.f, length0.f
    fma(16) Fz4.f, Dz0.f, length0.f
}
pred(f1.1){
    fma(16) Fx4.f, Dx1.f, length1.f
    fma(16) Fy4.f, Dy1.f, length1.f
    fma(16) Fz4.f, Dz1.f, length1.f
}

// Bodies 48-64
sub(16) Dx0.f, X6.f, BodyPair.f0<0,1,0>
sub(16) Dy0.f, Y6.f, BodyPair.f1<0,1,0>
sub(16) Dz0.f, Z6.f, BodyPair.f2<0,1,0>
sub(16) Dx1.f, X6.f, BodyPair.f4<0,1,0>
sub(16) Dy1.f, Y6.f, BodyPair.f5<0,1,0>
sub(16) Dz1.f, Z6.f, BodyPair.f6<0,1,0>
mul(16) lengthSq0.f, Dx0.f, Dx0.f
mul(16) lengthSq1.f, Dx1.f, Dx1.f
fma(16) lengthSq0.f, Dy0.f, Dy0.f
fma(16) lengthSq1.f, Dy1.f, Dy1.f
fma(16) lengthSq0.f, Dz0.f, Dz0.f
fma(16) lengthSq1.f, Dz1.f, Dz1.f
sqrt(16) length0.f, lengthSq0.f
sqrt(16) length1.f, lengthSq1.f
cmpgt(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpgt(16)(f1.1) null.u, lengthSq1.f, 0.0f

pred(f1.0){ mul(16) length0.f, length0.f, lengthSq0.f }
pred(f1.1){ mul(16) length1.f, length1.f, lengthSq1.f }
pred(f1.0){ rcp(16) length0.f, length0.f }
pred(f1.1){ rcp(16) length1.f, length1.f }
pred(f1.0){ mul(16) length0.f, length0.f, BodyPair.f3<0,1,0> }
pred(f1.1){ mul(16) length1.f, length1.f, BodyPair.f7<0,1,0> }
pred(f1.0){
    fma(16) Fx6.f, Dx0.f, length0.f
    fma(16) Fy6.f, Dy0.f, length0.f
    fma(16) Fz6.f, Dz0.f, length0.f
}
pred(f1.1){
    fma(16) Fx6.f, Dx1.f, length1.f
    fma(16) Fy6.f, Dy1.f, length1.f
    fma(16) Fz6.f, Dz1.f, length1.f
}


jmpif(f0.0) loop


write_out:

// TODO: try fma, but I'm skeptical because mul/add can do the "1.5x-issue" thing

// velocity += force * timestep
mul(16) Fx0.f, Fx0.f, globals.f1<0,1,0> 
mul(16) Fx2.f, Fx2.f, globals.f1<0,1,0>
mul(16) Fx4.f, Fx4.f, globals.f1<0,1,0>
mul(16) Fx6.f, Fx6.f, globals.f1<0,1,0>
mul(16) Fy0.f, Fy0.f, globals.f1<0,1,0>
mul(16) Fy2.f, Fy2.f, globals.f1<0,1,0>
mul(16) Fy4.f, Fy4.f, globals.f1<0,1,0>
mul(16) Fy6.f, Fy6.f, globals.f1<0,1,0>
mul(16) Fz0.f, Fz0.f, globals.f1<0,1,0>
mul(16) Fz2.f, Fz2.f, globals.f1<0,1,0>
mul(16) Fz4.f, Fz4.f, globals.f1<0,1,0>
mul(16) Fz6.f, Fz6.f, globals.f1<0,1,0>
add(16) VX0.f, VX0.f, Fx0.f
add(16) VX2.f, VX2.f, Fx2.f
add(16) VX4.f, VX4.f, Fx4.f
add(16) VX6.f, VX6.f, Fx6.f
add(16) VY0.f, VY0.f, Fy0.f
add(16) VY2.f, VY2.f, Fy2.f
add(16) VY4.f, VY4.f, Fy4.f
add(16) VY6.f, VY6.f, Fy6.f
add(16) VZ0.f, VZ0.f, Fz0.f
add(16) VZ2.f, VZ2.f, Fz2.f
add(16) VZ4.f, VZ4.f, Fz4.f
add(16) VZ6.f, VZ6.f, Fz6.f


// write velocities
mul(16)  base.u, r0.u1<0,1,0>, 256 // base address for this thread's bodies
add(16)  addrx.u, base.u, OFFSETS.u  // x addresses for first 16-body group 
add(16)  addry.u, addrx.u, 1         // y addresses for first 16-body group 
add(16)  addrz.u, addrx.u, 2         // z addresses for first 16-body group 
mov(16)  addrx2.f, VX0.f
mov(16)  addry2.f, VY0.f
mov(16)  addrz2.f, VZ0.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64  
add(16)  addry.u, addry.u, 64        
add(16)  addrz.u, addrz.u, 64  
mov(16)  addrx2.f, VX2.f
mov(16)  addry2.f, VY2.f
mov(16)  addrz2.f, VZ2.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64  
add(16)  addry.u, addry.u, 64        
add(16)  addrz.u, addrz.u, 64  
mov(16)  addrx2.f, VX4.f
mov(16)  addry2.f, VY4.f
mov(16)  addrz2.f, VZ4.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64 
add(16)  addry.u, addry.u, 64       
add(16)  addrz.u, addrz.u, 64 
mov(16)  addrx2.f, VX6.f
mov(16)  addry2.f, VY6.f
mov(16)  addrz2.f, VZ6.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u



// position += velocity* timestep
mul(16) VX0.f, VX0.f, globals.f1<0,1,0> 
mul(16) VX2.f, VX2.f, globals.f1<0,1,0>
mul(16) VX4.f, VX4.f, globals.f1<0,1,0>
mul(16) VX6.f, VX6.f, globals.f1<0,1,0>
mul(16) VY0.f, VY0.f, globals.f1<0,1,0>
mul(16) VY2.f, VY2.f, globals.f1<0,1,0>
mul(16) VY4.f, VY4.f, globals.f1<0,1,0>
mul(16) VY6.f, VY6.f, globals.f1<0,1,0>
mul(16) VZ0.f, VZ0.f, globals.f1<0,1,0>
mul(16) VZ2.f, VZ2.f, globals.f1<0,1,0>
mul(16) VZ4.f, VZ4.f, globals.f1<0,1,0>
mul(16) VZ6.f, VZ6.f, globals.f1<0,1,0>
add(16) X0.f, X0.f, VX0.f
add(16) X2.f, X2.f, VX2.f
add(16) X4.f, X4.f, VX4.f
add(16) X6.f, X6.f, VX6.f
add(16) Y0.f, Y0.f, VY0.f
add(16) Y2.f, Y2.f, VY2.f
add(16) Y4.f, Y4.f, VY4.f
add(16) Y6.f, Y6.f, VY6.f
add(16) Z0.f, Z0.f, VZ0.f
add(16) Z2.f, Z2.f, VZ2.f
add(16) Z4.f, Z4.f, VZ4.f
add(16) Z6.f, Z6.f, VZ6.f
 

// write positions
mul(16)  base.u, r0.u1<0,1,0>, 256    // base address for this thread's bodies
add(16)  addrx.u, base.u, OFFSETS.u  // x addresses for first 16-body group 
add(16)  addry.u, addrx.u, 1          // y addresses for first 16-body group 
add(16)  addrz.u, addrx.u, 2          // z addresses for first 16-body group 
mov(16)  addrx2.f, X0.f
mov(16)  addry2.f, Y0.f
mov(16)  addrz2.f, Z0.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u  
add(16)  addrx.u, addrx.u, 64
add(16)  addry.u, addry.u, 64
add(16)  addrz.u, addrz.u, 64
mov(16)  addrx2.f, X2.f
mov(16)  addry2.f, Y2.f
mov(16)  addrz2.f, Z2.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64
add(16)  addry.u, addry.u, 64
add(16)  addrz.u, addrz.u, 64
mov(16)  addrx2.f, X4.f
mov(16)  addry2.f, Y4.f
mov(16)  addrz2.f, Z4.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64
add(16)  addry.u, addry.u, 64
add(16)  addrz.u, addrz.u, 64
mov(16)  addrx2.f, X6.f
mov(16)  addry2.f, Y6.f
mov(16)  addrz2.f, Z6.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u


end

    );



// even:  42/96
// odd: 52/106/10 -> result first buffer


void Nbody()
{
    
 //   HAXWell::Blob blob;
 //   HAXWell::RipIsaFromGLSL( blob, NBODYGLSL );
 //   PrintISA(stdout, blob );
    
    
    Simulator sim;
    sim.hKernel = HAXWell::CreateGLSLShader( NBODYGLSL );
    sim.nBodiesPerThreadGroup = 16;
    sim.nBodies = 64*256;
    sim.dt = 1.0f/10.0f;
    sim.nSteps = 33;
    sim.GraviationalConstant = 10.01f; // whatever...
   
  
    float* pGLSL = Nbody(sim);
    HAXWell::ReleaseShader(sim.hKernel);
    
    sim.hKernel = HAXWell::CreateGLSLShader( NBODYGLSL_SM );
    sim.nBodiesPerThreadGroup = SM_SIZE;
  
    float* pGLSLSM = Nbody(sim);
    HAXWell::ReleaseShader(sim.hKernel);
    

    class Printer : public GEN::IPrinter{
    public:
        virtual void Push( const char* p )
        {
            printf("%s", p );
        }
    };

    GEN::Encoder encoder;
    GEN::Decoder decoder;

   Printer pr;
   GEN::Assembler::Program program;
   if( program.Assemble( &encoder, NBODY_HAXWELL, &pr ) )
   {
        HAXWell::ShaderArgs args;
        args.nCURBEAllocsPerThread = program.GetCURBERegCount();
        args.nDispatchThreadCount = program.GetThreadsPerDispatch();
        args.nSIMDMode = 16;
        args.nIsaLength = program.GetIsaLengthInBytes();
        args.pCURBE = program.GetCURBE();
        args.pIsa = program.GetIsa();

        sim.hKernel = HAXWell::CreateShader( args );
        sim.nBodiesPerThreadGroup = 64*program.GetThreadsPerDispatch();

  //      PrintISA( stdout,args.pIsa, args.nIsaLength);
        float* pHSW = Nbody(sim);
        
        /*
        for( size_t i=0; i<4*sim.nBodies; i+= 4 )
        {
            float dx = (pGLSL[i]-pGLSLSM[i])/pGLSL[i];
           if( fabs(dx) > 0.1 )
               printf("foo(%u) %f, %f\n", i/4, pGLSL[i], pGLSLSM[i]);
        }*/
   //  for( size_t i=0; i<4*sim.nBodies; i+= 4 )
   //     printf( "%f,%f,%f -- %f,%f,%f\n", pGLSL[i],pGLSL[i+1],pGLSL[i+2], 
   //                                       pHSW[i], pHSW[i+1],pHSW[i+2] );
        
   }


}