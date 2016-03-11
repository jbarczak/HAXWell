
#include "GENCoder.h"
#include "GENDisassembler.h"
#include "HAXWell.h"
#include "Misc.h"
#include <stdio.h>

#define STRINGIFY(...) "#version 430 core\n" #__VA_ARGS__

const char* NBODYGLSL = STRINGIFY(
    
    layout (local_size_x = 16) in;

        layout (std430,binding=0)
        buffer Output0
        {
           uint g_nBodies;
           float g_TimeStep;
           float g_GraviationConst;
        };

        layout (std430,binding=1)
        buffer Bodies
        {
           vec4 g_PositionAndMass[];
        };
        layout (std430,binding=2)
        buffer Velocities
        {
           vec4 g_Velocity[];
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
                    force += scale*D;
                }
            }

            vec3 velocity = g_Velocity[tid].xyz;
            vec3 accel    = force; // mass cancels out

            velocity += accel * g_TimeStep;
            body.xyz += velocity * g_TimeStep;

            g_PositionAndMass[tid].xyz = body.xyz;
            g_Velocity[tid].xyz = velocity.xyz;
        }


    );





const char* NBODYGLSL_2 = STRINGIFY(
    
    layout (local_size_x = 16) in;

        layout (std430,binding=0)
        buffer Output0
        {
           uint g_nBodies;
           float g_TimeStep;
           float g_GraviationConst;
        };

        layout (std430,binding=1)
        buffer Bodies
        {
           vec4 g_PositionAndMass[];
        };
        layout (std430,binding=2)
        buffer Velocities
        {
           vec4 g_Velocity[];
        };


        void main() 
        {
            uint tid = gl_GlobalInvocationID.x;
            
            vec4 bodies[8];
            vec3 forces[8];

            for( uint i=0; i<8; i++ )
            {
                bodies[i] = g_PositionAndMass[8*tid+i];
                forces[i] = vec3(0.f,0.f,0.f);
            }

            for( uint i=0; i<g_nBodies; i++ )
            {
                vec4 otherBody = g_PositionAndMass[i];
               
                for( uint j=0; j<8; j++ )
                {
                    if( i != 8*tid + j )
                    {
                        vec3 D = bodies[j].xyz - otherBody.xyz;

                        // force = G*massi*massj * (D ) / length(D)^3
                        // length(D)^3 is dot(D,D) * sqrt(dot(D,D))
                        float lengthSq = dot( D, D );
                        float lengthCubed = lengthSq * sqrt(lengthSq);

                        float scale = ( g_GraviationConst * otherBody.w) / lengthCubed;
                        forces[j] += scale*D;
                    }
                }
            }

            for( uint j=0; j<8; j++ )
            {
                vec3 velocity = g_Velocity[tid*8 + j].xyz;
                vec3 accel    = forces[j]/bodies[j].w;
               
                velocity += accel;
                
                g_PositionAndMass[tid*8+j].xyz = bodies[j].xyz + velocity*g_TimeStep;
                g_Velocity[tid*8+j].xyz = velocity.xyz;
            }
        }


    );





void Nbody()
{
    HAXWell::Blob blob;
    HAXWell::RipIsaFromGLSL( blob, NBODYGLSL );

   // FILE* fp = fopen("nbody2.txt", "w");
    PrintISA(stdout, blob );
}