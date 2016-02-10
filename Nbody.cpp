
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
                if( i == tid )
                    continue;

                vec4 otherBody = g_PositionAndMass[i];
                vec3 D = body.xyz - otherBody.xyz;

                // force = G*massi*massj * (D ) / length(D)^3
                // length(D)^3 is dot(D,D) * sqrt(dot(D,D))
                float lengthSq = dot( D, D );
                float lengthCubed = lengthSq * sqrt(lengthSq);

                float scale = ( g_GraviationConst * body.w * otherBody.w) / lengthCubed;
                force += scale*D;
            }

            vec3 velocity = g_Velocity[tid].xyz;
            vec3 accel    = force/body.w;

            velocity += accel;
            body.xyz += velocity * g_TimeStep;

            g_PositionAndMass[tid].xyz = body.xyz;
            g_Velocity[tid].xyz = velocity.xyz;
        }


    );



void Nbody()
{
    HAXWell::Blob blob;
    HAXWell::RipIsaFromGLSL( blob, NBODYGLSL );

    PrintISA(stdout, blob );
}