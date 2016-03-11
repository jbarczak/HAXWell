
#include "GENAssembler.h"
#include "GENDisassembler.h"
#include "GENCoder.h"

#define STRINGIFY(...) #__VA_ARGS__

// assembler features:
//   reg naming
//   offsetting from named regs using numbers:   x0  etc...
#define M 64

const char* TEST = STRINGIFY(

curbe OFFSETS[2] = {{0,4,8,12,16,20,24,28},
                    {32,36,40,44,48,52,56}}

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
reg tmp[2]
reg BodyPair

bind Globals        0x38  // {#bodies,timestep,gravitation}
bind PositionIn     0x39    // xyzm
bind VelocityIn     0x40
bind PositionOut    0x41    // xyzm
bind VelocityOut    0x42

// buffer bind points:  bind 0:  xyzm per body, bind 1:  velocity (xyz) per body
//  
// threads evaluate M bodies each

begin:

// Load blocks of body positions which this thread will process
mul(16)  base.u, r0.u2<0,1,0>, 256 // base address for this thread's bodies
add(16)  addrx.u, base.u, OFFSETS.u  // x addresses for first 16-body group 
add(16)  addry.u, base.u, 1          // y addresses for first 16-body group 
add(16)  addrz.u, base.u, 2          // z addresses for first 16-body group 
send     DwordLoad16(PositionIn), X0.f, addrx.u       
send     DwordLoad16(PositionIn), Y0.f, addry.u       
send     DwordLoad16(PositionIn), Z0.f, addrz.u
add(16)  addrx.u, addrx.u, 64       // skip 16 bodies (64 dwords)
add(16)  addry.u, addry.u, 64       // skip 16 bodies (64 dwords)    
add(16)  addrz.u, addrz.u, 64       // skip 16 bodies (64 dwords)   
send     DwordLoad16(PositionIn), X2.f, addrx.u       
send     DwordLoad16(PositionIn), Y2.f, addry.u       
send     DwordLoad16(PositionIn), Z2.f, addrz.u
add(16)  addrx.u, addrx.u, 64  
add(16)  addry.u, addry.u, 64        
add(16)  addrz.u, addrz.u, 64       
send     DwordLoad16(PositionIn), X4.f, addrx.u       
send     DwordLoad16(PositionIn), Y4.f, addry.u       
send     DwordLoad16(PositionIn), Z4.f, addrz.u
add(16)  addrx.u, addrx.u, 64  
add(16)  addry.u, addry.u, 64        
add(16)  addrz.u, addrz.u, 64       
send     DwordLoad16(PositionIn), X6.f, addrx.u       
send     DwordLoad16(PositionIn), Y6.f, addry.u       
send     DwordLoad16(PositionIn), Z6.f, addrz.u

// Load velocities up front as well.  We've got plenty of regs and we know we'll eventually need them
mul(16)  base.u, r0.u2<0,1,0>, 192 // base address for this thread's bodies
add(16)  addrx.u, base.u, OFFSETS.u  // x addresses for first 16-body group 
add(16)  addry.u, base.u, 1          // y addresses for first 16-body group 
add(16)  addrz.u, base.u, 2          // z addresses for first 16-body group 
send     DwordLoad16(VelocityIn), VX0.f, addrx.u       
send     DwordLoad16(VelocityIn), VY0.f, addry.u       
send     DwordLoad16(VelocityIn), VZ0.f, addrz.u
add(16)  addrx.u, addrx.u, 48       // skip 16 bodies (48 dwords)
add(16)  addry.u, addry.u, 48       // skip 16 bodies (48 dwords)    
add(16)  addrz.u, addrz.u, 48       // skip 16 bodies (48 dwords)   
send     DwordLoad16(VelocityIn), VX2.f, addrx.u       
send     DwordLoad16(VelocityIn), VY2.f, addry.u       
send     DwordLoad16(VelocityIn), VZ2.f, addrz.u
add(16)  addrx.u, addrx.u, 48  
add(16)  addry.u, addry.u, 48        
add(16)  addrz.u, addrz.u, 48       
send     DwordLoad16(VelocityIn), VX4.f, addrx.u       
send     DwordLoad16(VelocityIn), VY4.f, addry.u       
send     DwordLoad16(VelocityIn), VZ4.f, addrz.u
add(16)  addrx.u, addrx.u, 48  
add(16)  addry.u, addry.u, 48        
add(16)  addrz.u, addrz.u, 48       
send     DwordLoad16(VelocityIn), VX6.f, addrx.u       
send     DwordLoad16(VelocityIn), VY6.f, addry.u       
send     DwordLoad16(VelocityIn), VZ6.f, addrz.u

// load the global sim constants
mov(8) base.u, 0
mov(8) i.u,  0    // 
mov(1) f0.u, 0
mov(1) f1.u, 0
send DwordLoad8(Globals), globals.u, base.u 

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

cmpge(1)(f0.0) null.u, i.u0, globals.u0
jmpif(f0.0) write_out

loop:

// TODO: load position/mass for two bodies into one reg
//   use SIMD16 ops and broadcasting to compute forces
//   predicate final force adds on body index to avoid singularities
mul(1) base.u, i.u, 4
add(1) i.u0 , i.u0<1,1,1>, 2
cmplt(1)(f0.0) null.u, i.u0, globals.u0

send     DwordLoad8(PositionIn), BodyPair.f, base.u   

mul(1) BodyPair.f3, BodyPair.f3, globals.f2 // pre-multiply body mass by gravitation constant
mul(1) BodyPair.f7, BodyPair.f7, globals.f2 // TODO: Do in one SIMD4x2 op with write masking...?

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
cmpne(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpne(16)(f1.1) null.u, lengthSq1.f, 0.0f
pred(f1.0)
{
    rsq(16) tmp.f, lengthSq0.f
    mul(16) lengthSq0.f, lengthSq0.f, tmp.f
    mul(16) lengthSq0.f, lengthSq0.f, BodyPair.f3<0,1,0> // body masses
    fma(16) Fx0.f, Dx0.f, lengthSq0.f
    fma(16) Fy0.f, Dy0.f, lengthSq0.f
    fma(16) Fz0.f, Dz0.f, lengthSq0.f
}
pred(f1.1)
{
    rsq(16) tmp.f, lengthSq1.f
    mul(16) lengthSq1.f, lengthSq1.f, tmp.f
    mul(16) lengthSq1.f, lengthSq1.f, BodyPair.f7<0,1,0> // body masses
    fma(16) Fx0.f, Dx1.f, lengthSq1.f
    fma(16) Fy0.f, Dy1.f, lengthSq1.f
    fma(16) Fz0.f, Dz1.f, lengthSq1.f
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
cmpne(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpne(16)(f1.1) null.u, lengthSq1.f, 0.0f
pred(f1.0)
{
  rsq(16) tmp.f, lengthSq0.f
  mul(16) lengthSq0.f, lengthSq0.f, tmp.f
  mul(16) lengthSq0.f, lengthSq0.f, BodyPair.f3<0,1,0> // body masses
  fma(16) Fx2.f, Dx0.f, lengthSq0.f
  fma(16) Fy2.f, Dy0.f, lengthSq0.f
  fma(16) Fz2.f, Dz0.f, lengthSq0.f
}
pred(f1.1)
{
  rsq(16) tmp.f, lengthSq1.f
  mul(16) lengthSq1.f, lengthSq1.f, tmp.f
  mul(16) lengthSq1.f, lengthSq1.f, BodyPair.f7<0,1,0> // body masses
  fma(16) Fx2.f, Dx1.f, lengthSq1.f
  fma(16) Fy2.f, Dy1.f, lengthSq1.f
  fma(16) Fz2.f, Dz1.f, lengthSq1.f
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
cmpne(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpne(16)(f1.1) null.u, lengthSq1.f, 0.0f
pred(f1.0)
{
  rsq(16) tmp.f, lengthSq0.f
  mul(16) lengthSq0.f, lengthSq0.f, tmp.f
  mul(16) lengthSq0.f, lengthSq0.f, BodyPair.f3<0,1,0> // body masses
  fma(16) Fx4.f, Dx0.f, lengthSq0.f
  fma(16) Fy4.f, Dy0.f, lengthSq0.f
  fma(16) Fz4.f, Dz0.f, lengthSq0.f
}
pred(f1.1)
{
  rsq(16) tmp.f, lengthSq1.f
  mul(16) lengthSq1.f, lengthSq1.f, tmp.f
  mul(16) lengthSq1.f, lengthSq1.f, BodyPair.f7<0,1,0> // body masses
  fma(16) Fx4.f, Dx1.f, lengthSq1.f
  fma(16) Fy4.f, Dy1.f, lengthSq1.f
  fma(16) Fz4.f, Dz1.f, lengthSq1.f
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
cmpne(16)(f1.0) null.u, lengthSq0.f, 0.0f
cmpne(16)(f1.1) null.u, lengthSq1.f, 0.0f
pred(f1.0)                   
{
  rsq(16) tmp.f, lengthSq0.f
  mul(16) lengthSq0.f, lengthSq0.f, tmp.f
  mul(16) lengthSq0.f, lengthSq0.f, BodyPair.f3<0,1,0> // body masses
  fma(16) Fx6.f, Dx0.f, lengthSq0.f
  fma(16) Fy6.f, Dy0.f, lengthSq0.f
  fma(16) Fz6.f, Dz0.f, lengthSq0.f
}
pred(f1.1)
{
  rsq(16) tmp.f, lengthSq1.f
  mul(16) lengthSq1.f, lengthSq1.f, tmp.f
  mul(16) lengthSq1.f, lengthSq1.f, BodyPair.f7<0,1,0> // body masses
  fma(16) Fx6.f, Dx1.f, lengthSq1.f
  fma(16) Fy6.f, Dy1.f, lengthSq1.f
  fma(16) Fz6.f, Dz1.f, lengthSq1.f
}


jmpif(f0.0) loop


write_out:

// TODO: try fma, but I'm skeptical because mul/add can do the "1.5x-issue" thing

// velocity += force * timestep
mul(16) Fx0.f, Fx0.f, globals.u1<0,1,0> 
mul(16) Fx2.f, Fx2.f, globals.u1<0,1,0>
mul(16) Fx4.f, Fx4.f, globals.u1<0,1,0>
mul(16) Fx6.f, Fx6.f, globals.u1<0,1,0>
mul(16) Fy0.f, Fy0.f, globals.u1<0,1,0>
mul(16) Fy2.f, Fy2.f, globals.u1<0,1,0>
mul(16) Fy4.f, Fy4.f, globals.u1<0,1,0>
mul(16) Fy6.f, Fy6.f, globals.u1<0,1,0>
mul(16) Fz0.f, Fz0.f, globals.u1<0,1,0>
mul(16) Fz2.f, Fz2.f, globals.u1<0,1,0>
mul(16) Fz4.f, Fz4.f, globals.u1<0,1,0>
mul(16) Fz6.f, Fz6.f, globals.u1<0,1,0>
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
mul(16)  base.u, r0.u2<0,1,0>, 192 // base address for this thread's bodies
add(16)  addrx.u, base.u, OFFSETS.u  // x addresses for first 16-body group 
add(16)  addry.u, base.u, 1          // x addresses for first 16-body group 
add(16)  addrz.u, base.u, 2          // x addresses for first 16-body group 
mov(16)  addrx2.f, VX0.f
mov(16)  addry2.f, VY0.f
mov(16)  addrz2.f, VZ0.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 48  
add(16)  addry.u, addry.u, 48        
add(16)  addrz.u, addrz.u, 48  
mov(16)  addrx2.f, VX2.f
mov(16)  addry2.f, VY2.f
mov(16)  addrz2.f, VZ2.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 48  
add(16)  addry.u, addry.u, 48        
add(16)  addrz.u, addrz.u, 48  
mov(16)  addrx2.f, VX4.f
mov(16)  addry2.f, VY4.f
mov(16)  addrz2.f, VZ4.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 48  
add(16)  addry.u, addry.u, 48        
add(16)  addrz.u, addrz.u, 48  
mov(16)  addrx2.f, VX6.f
mov(16)  addry2.f, VY6.f
mov(16)  addrz2.f, VZ6.f
send     DwordStore16(VelocityOut), null.u, addrx.u       
send     DwordStore16(VelocityOut), null.u, addry.u       
send     DwordStore16(VelocityOut), null.u, addrz.u


// position += velocity* timestep
mul(16) VX0.f, VX0.f, globals.u1<0,1,0> 
mul(16) VX2.f, VX2.f, globals.u1<0,1,0>
mul(16) VX4.f, VX4.f, globals.u1<0,1,0>
mul(16) VX6.f, VX6.f, globals.u1<0,1,0>
mul(16) VY0.f, VY0.f, globals.u1<0,1,0>
mul(16) VY2.f, VY2.f, globals.u1<0,1,0>
mul(16) VY4.f, VY4.f, globals.u1<0,1,0>
mul(16) VY6.f, VY6.f, globals.u1<0,1,0>
mul(16) VZ0.f, VZ0.f, globals.u1<0,1,0>
mul(16) VZ2.f, VZ2.f, globals.u1<0,1,0>
mul(16) VZ4.f, VZ4.f, globals.u1<0,1,0>
mul(16) VZ6.f, VZ6.f, globals.u1<0,1,0>
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
mul(16)  base.u, r0.u2<0,1,0>, 256 // base address for this thread's bodies
add(16)  addrx.u, base.u, OFFSETS.u  // x addresses for first 16-body group 
add(16)  addry.u, base.u, 1          // y addresses for first 16-body group 
add(16)  addrz.u, base.u, 2          // z addresses for first 16-body group 
mov(16)  addrx2.f, X0.f
mov(16)  addry2.f, Y0.f
mov(16)  addrz2.f, Z0.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64
add(16)  addry.u, addrx.u, 64
add(16)  addrz.u, addrx.u, 64
mov(16)  addrx2.f, X2.f
mov(16)  addry2.f, Y2.f
mov(16)  addrz2.f, Z2.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64
add(16)  addry.u, addrx.u, 64
add(16)  addrz.u, addrx.u, 64
mov(16)  addrx2.f, X4.f
mov(16)  addry2.f, Y4.f
mov(16)  addrz2.f, Z4.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u
add(16)  addrx.u, addrx.u, 64
add(16)  addry.u, addrx.u, 64
add(16)  addrz.u, addrx.u, 64
mov(16)  addrx2.f, X6.f
mov(16)  addry2.f, Y6.f
mov(16)  addrz2.f, Z6.f
send     DwordStore16(PositionOut), null.u, addrx.u       
send     DwordStore16(PositionOut), null.u, addry.u       
send     DwordStore16(PositionOut), null.u, addrz.u




end


    );

void AssemblerTest()
{
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
   if( program.Assemble( &encoder, TEST, &pr ) )
   {
        GEN::Disassemble( pr, &decoder, program.GetIsa(), program.GetIsaLengthInBytes() );

        if( program.GetCURBERegCount() )
        {
            printf("\nCURBE regs: %u \n", program.GetCURBERegCount() );

            unsigned int* pCURBE = (unsigned int*) program.GetCURBE();
            for( size_t i=0; i<program.GetCURBERegCount(); i++ )
            {
                printf("{0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x}\n",
                       pCURBE[0],
                       pCURBE[1],
                       pCURBE[2],
                       pCURBE[3],
                       pCURBE[4],
                       pCURBE[5],
                       pCURBE[6],
                       pCURBE[7] );
                pCURBE += 8;
            }
        }

   }


}