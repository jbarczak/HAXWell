

curbe INDICES[2] = {{0,1,2,3,4,5,6,7},
                    {8,9,10,11,12,13,14,15}}
curbe TRI_OFFSETS[2] = {{ 0,1,2,2,  9,10,11,11 },
                        { 3,4,5,5,  6,7,8,8 } }

bind Rays       0x38  // { nrays,x,x,x [ox,oy,oz,tmax,dx,dy,dz,pad]... .}
bind HitInfo    0x39  // { bbmin(x,y,z),offs,bbmax(x,y,z),count_and_axis} .. see glsl shader for details
bind Nodes      0x3a
bind Triangles  0x3b

reg Stack[32]


reg ray_addr
reg ray_idx
reg num_rays

reg ray_Ox[2]
reg ray_Oy[2]
reg ray_Oz[2]
reg ray_Dx[2]
reg ray_Dy[2]
reg ray_Dz[2]
reg ray_inv_Dx[2]
reg ray_inv_Dy[2]
reg ray_inv_Dz[2]
reg ray_tmax

reg ray_signs
reg node_address
reg tmin_x
reg tmax_x
reg tmin_y
reg tmax_y
reg tmin_z
reg tmax_z
reg tmin
reg tmax
reg node
reg nearfar

reg blockwrite[2]

reg tri_id
reg tri_end
reg tri_count
reg tri_base
reg tri_addr[2]
reg tri_data[2]
reg tmp[6]
reg hit_u
reg hit_v
reg hit_id
reg crosses[6]

reg v0A[3]
reg ab[2]
reg v
reg t
reg c
reg ONE
reg axis_mask

begin:


send DwordLoad8(Rays), num_rays.u, INDICES.u

mul(8) ray_idx.u, r0.u1<0,1,0>, 8
add(8) ray_idx.u, ray_idx.u, INDICES.u
cmpge(8) (f0.0) null.u, ray_idx.u, num_rays.u<0,1,0>
pred(f0.0)
{
    // replicate last ray into any unused lanes to prevent divergence
    mov(8) ray_idx.u, num_rays.u<0,1,0>
    sub(8) ray_idx.u, ray_idx.u, 1
}


// load ray:
//  address = 8*tid + 4 
mul(8) ray_addr.u, ray_idx.u, 8
add(8) ray_addr.u, ray_addr.u, 4

send DwordLoad8(Rays), ray_Ox.u, ray_addr.u
add(8) ray_addr.u, ray_addr.u, 1

send DwordLoad8(Rays), ray_Oy.u, ray_addr.u
add(8) ray_addr.u, ray_addr.u, 1

send DwordLoad8(Rays), ray_Oz.u, ray_addr.u
add(8) ray_addr.u, ray_addr.u, 1

send DwordLoad8(Rays), ray_tmax.u, ray_addr.u
add(8) ray_addr.u, ray_addr.u, 1

send DwordLoad8(Rays), ray_Dx.u, ray_addr.u
add(8) ray_addr.u, ray_addr.u, 1

send DwordLoad8(Rays), ray_Dy.u, ray_addr.u
add(8) ray_addr.u, ray_addr.u, 1

send DwordLoad8(Rays), ray_Dz.u, ray_addr.u
add(8) ray_addr.u, ray_addr.u, 1


// load current node
mov(8) node_address.u, INDICES.u
send DwordLoad8(Nodes), node.u, node_address.u 
       
// duplicate ray directions for SIMD16 opts
// TODO: I haven't found a way to replicate a full register
//   using regioning.  <0,8,1> does not do what I expected it to
mov(8) ray_Dx1.f, ray_Dx.f
mov(8) ray_Dy1.f, ray_Dy.f
mov(8) ray_Dz1.f, ray_Dz.f // WARNING: These Dx1 instructions must be first
                           // otherwise HW register dependency check doesn't happen for rcp(16)
mov(8) ray_Ox1.f, ray_Ox.f
mov(8) ray_Oy1.f, ray_Oy.f
mov(8) ray_Oz1.f, ray_Oz.f

// precompute reciprocals
rcp(16) ray_inv_Dx.f, ray_Dx.f
rcp(16) ray_inv_Dy.f, ray_Dy.f
rcp(16) ray_inv_Dz.f, ray_Dz.f


// build ray axis mask based on first ray in packet. 
//  use for ordered traversal
shr(1) tmp0.u, ray_Dx.u0, 31
shr(1) tmp1.u, ray_Dy.u0, 30
shr(1) tmp2.u, ray_Dz.u0, 29
and(1) tmp1.u, tmp1.u, 2
and(1) tmp2.u, tmp2.u, 4
or(1)  tmp0.u, tmp0.u, tmp1.u
mov(1) ONE.u, 1
or(1)  axis_mask.u, tmp0.u, tmp1.u
xor(1) axis_mask.u1, axis_mask.u0, 0x7 // store inverted axis mask in second channel




mov(2) f0.us0, 0
mov(2) f1.us0, 0 // clear the flags, since we do a bunch of width-1 compares down in the loop
mov(1) a0.us0, 0

mov(8) hit_u.f, 0.0f
mov(8) hit_v.f, 0.0f
mov(8) hit_id.u, 0xffffffff



// begin traversal loop
traversal:

    sub(8)  tmp0.f, node.f0<0,1,0>, ray_Ox.f
    sub(8)  tmp1.f, node.f4<0,1,0>, ray_Ox.f
    sub(8)  tmp2.f, node.f1<0,1,0>, ray_Oy.f
    sub(8)  tmp3.f, node.f5<0,1,0>, ray_Oy.f
    sub(8)  tmp4.f, node.f2<0,1,0>, ray_Oz.f
    sub(8)  tmp5.f, node.f6<0,1,0>, ray_Oz.f
    mul(16) tmp0.f, tmp0.f, ray_inv_Dx.f
    mul(16) tmp2.f, tmp2.f, ray_inv_Dy.f
    mul(16) tmp4.f, tmp4.f, ray_inv_Dz.f

    min(8) tmin_x.f, tmp0.f, tmp1.f
    max(8) tmax_x.f, tmp0.f, tmp1.f
    min(8) tmin_y.f, tmp2.f, tmp3.f
    max(8) tmax_y.f, tmp2.f, tmp3.f
    min(8) tmin_z.f, tmp4.f, tmp5.f
    max(8) tmax_z.f, tmp4.f, tmp5.f

    min(8) tmax.f, tmax_x.f, tmax_y.f
    max(8) tmin.f, tmin_x.f, tmin_y.f
    min(8) tmax.f, tmax.f, tmax_z.f
    max(8) tmin.f, tmin.f, tmin_z.f
    min(8) tmax.f, tmax.f, ray_tmax.f

    cmple(8) (f0.0) tmp0.f, tmin.f, tmax.f
    cmpge(8) (f0.1) tmp1.f, tmax.f, 0.0f
    and(8) tmp0.u, tmp0.u, tmp1.u
    cmpgt(8) (f0.0) null.u, tmp0.u, 0
    jmpif(!f0.0) pop_stack

    // we hit the node.  Is it a leaf?
    and(1) tmp0.u, node.u7, 3
    cmpeq(1) (f1.0) null.u, tmp0.u, 3
    jmpif(f1.0) visit_leaf  // yes. go intersect some triangles

    
    // no.  Manipulate stack and continue
    
    // choose traversal order based on direction signs
    //  axis_mask.u0 contains the ray direction sign bits
    //  axis_mask.u1 contains the inverse
    //  near node is 0 (positive,axis bit=0) or 1 (negative)
    bfe(2) nearfar.u, ONE.u, t0.u, axis_mask.u
    add(2) nearfar.u, nearfar.u, node.u3<0,1,0>

     // start reading near node into working register for next traversal step
    mul(8) tmp0.u, nearfar.u0<0,1,0>, 8 
    add(8) node_address.u, tmp0.u, INDICES.u
    send DwordLoad8(Nodes), node.u, node_address.u 
    
    // start reading far node into stack head for some future traversal step
    mul(8) tmp0.u, nearfar.u1<0,1,0>, 8 
    add(8) node_address.u, node_address.u, 8
    send DwordLoad8(Nodes), Stack[a0.0].u, node_address.u 
    add(1) a0.us0,a0.us0, 32
    
    jmp traversal


visit_leaf:
    
    mov(1) tri_id.u, node.u3
    shr(1) tri_count.u, node.u7, 2
    add(1) tri_end.u, tri_id.u, tri_count.u
    
isect_loop:
    
    mul(8) tri_base.u, tri_id.u<0,1,0>, 12
    
    //struct TrianglePP
    //{
    //    vec3 P0;      // 0,1,2
    //    vec3 v02;     // 3,4,5
    //    vec3 v10;     // 6,7,8
    //    vec3 v10x02;
    //};
    

    add(16) tri_addr0.u, tri_base.u<0,1,0>, INDICES.u
    send DwordLoad16(Triangles), tri_data.u, tri_addr.u

    // v0A = P0 - origin (replicated)
    sub(8) v0A0.f, tri_data.f0<0,1,0>, ray_Ox.f
    sub(8) v0A1.f, tri_data.f1<0,1,0>, ray_Oy.f
    sub(8) v0A2.f, tri_data.f2<0,1,0>, ray_Oz.f

    // compute two packed cros s products
    //   crosses[6] contains:  
    //      v02x0A (x)
    //      v10x0A (x)
    //      v02x0A (y)
    //      v10x0A (y) 
    //      v02x0A (z)
    //      v10x0A (z) 
    mul(8) tmp0.f, tri_data.f4<0,1,0>, v0A2.f // y*z
    mul(8) tmp2.f, tri_data.f5<0,1,0>, v0A1.f // z*y
    mul(8) tmp1.f, tri_data.f7<0,1,0>, v0A2.f
    mul(8) tmp3.f, tri_data.f8<0,1,0>, v0A1.f
    sub(16) crosses0.f, tmp0.f, tmp2.f
    mul(8) tmp0.f, tri_data.f5<0,1,0>, v0A0.f // z*x
    mul(8) tmp2.f, tri_data.f3<0,1,0>, v0A2.f // x*z
    mul(8) tmp1.f, tri_data.f8<0,1,0>, v0A0.f
    mul(8) tmp3.f, tri_data.f6<0,1,0>, v0A2.f
    sub(16) crosses2.f, tmp0.f, tmp2.f
    mul(8) tmp0.f, tri_data.f3<0,1,0>, v0A1.f // x*y
    mul(8) tmp2.f, tri_data.f4<0,1,0>, v0A0.f // y*x
    mul(8) tmp1.f, tri_data.f6<0,1,0>, v0A1.f
    mul(8) tmp3.f, tri_data.f7<0,1,0>, v0A0.f
    sub(16) crosses4.f, tmp0.f, tmp2.f

    // v = dot(v10x02,ray_dir)
    // t = dot(v10x02,v0A)
    //  dot products interleaved
    // ab = {dot(v02x0a,D)..,dot(v10x0a,D)..}
    //   NOTE: Can't use mads because of swizzles on tri_data.f9

    mov(8) tmp0.f, tri_data.f9<0,1,0>
    mul(16) ab.f, crosses.f,  ray_Dx.f
    mul(8) v.f, tmp0.f,  ray_Dx.f
    mul(8) t.f, tmp0.f,  v0A0.f
    
    mov(8) tmp0.f, tri_data.f10<0,1,0>
    fma(16) ab.f, crosses2.f, ray_Dy.f
    fma(8) v.f, tmp0.f, ray_Dy.f
    fma(8) t.f, tmp0.f, v0A1.f
    
    mov(8) tmp0.f, tri_data.f11<0,1,0>
    fma(16) ab.f, crosses4.f, ray_Dz.f
    fma(8) v.f, tmp0.f, ray_Dz.f
    fma(8) t.f, tmp0.f, v0A2.f
    rcp(8) v.f, v.f
  
    
    mul(8) ab.f, ab.f,   v.f
    mul(8) ab1.f, ab1.f, v.f
    mul(8) t.f, t.f, v.f
    add(8) c.f, ab0.f, ab1.f
    
    cmplt(8) (f0.0) tmp0.f, t.f, ray_tmax.f
    cmpgt(8) (f0.0) tmp1.f, t.f, 0.0f
    cmpge(8) (f0.0) tmp2.f, ab0.f, 0.0f
    cmpge(8) (f0.0) tmp3.f, ab1.f, 0.0f
    cmple(8) (f0.0) tmp4.f, c.f, 1.0f
    and(16) tmp0.u, tmp0.u, tmp2.u
    and(8)  tmp0.u, tmp0.u, tmp1.u
    and(8)  tmp0.u, tmp0.u, tmp4.u
    cmpgt(8) (f0.0) null.u, tmp0.u, 0
    pred(f0.0)
    {
        // if we reach this point we have hit the triangle
        mov(8) hit_id.u, tri_id.u<0,1,0>
        mov(8) hit_u.f, ab0.f
        mov(8) hit_v.f, ab1.f
        mov(8) ray_tmax.f, t.f
    }
    mov(2) f0.us0, 0
    
next_iter:
    add(1) tri_id.u, tri_id.u, 1
    cmplt(1) (f1.0) null.u, tri_id.u, tri_end.u
    jmpif(f1.0) isect_loop


pop_stack:

  // bail out if we've reached the bottom of the stack
    cmpeq(1) (f1.0) tmp0.u, a0.us0, 0

    // decrement stack ptr before the jump, because if the jump is taken, we don't care about underflow
    //  This gives a healthy speedup because the jump can cover the latency of the write to a0,
    //   and the add can cover the latency of the cmp
    add(1) a0.us0, a0.us0, -32       // NOTE: our 'sub' pneumonic doesn't work for immediate operands
    
    jmpif(f1.0) finished

    // pop the next node off the stack
    mov(8) node.u, Stack[a0.0].u
    
    jmp traversal


finished:

    // Store hit info
    mul(8) tmp0.u, ray_idx.u, 4
    mov(8) tmp1.u, hit_u.u
    send DwordStore8(HitInfo), null.u, tmp0.u

    add(8) tmp0.u, tmp0.u, 1
    mov(8) tmp1.u, hit_v.u
    send DwordStore8(HitInfo), null.u, tmp0.u

    add(8) tmp0.u, tmp0.u, 1
    mov(8) tmp1.u, hit_id.u
    send DwordStore8(HitInfo), null.u, tmp0.u

    add(8) tmp0.u, tmp0.u, 1
    mov(8) tmp1.u, ray_tmax.u
    send DwordStore8(HitInfo), null.u, tmp0.u


end

