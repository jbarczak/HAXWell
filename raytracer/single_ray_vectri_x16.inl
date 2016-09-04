
curbe INDICES[2] = {{0,1,2,3,4,5,6,7},
                    {8,9,10,11,12,13,14,15}}



bind Rays       0x38  // { nrays,x,x,x [ox,oy,oz,tmax,dx,dy,dz,pad]... .}
bind HitInfo    0x39  // { bbmin(x,y,z),offs,bbmax(x,y,z),count_and_axis} .. see glsl shader for details
bind Nodes      0x3a
bind Triangles  0x3b

reg HIT_INFO

//NOTE: 'stack' must be declared early so that it is based near the top of the reg file
//  Otherwise we overflow the 9-bit signed address immediate field
reg Stack[32] 
reg ONE
reg axis_mask
reg node

reg blockwrite[2]
reg bvh_indices
reg ray_addr
reg ray_idx
reg ray_data
reg ray_data_rcp
reg node_address

reg tmin
reg tmax
reg nearfar


reg tmp[10]
reg ray_O
reg ray_D
reg ray_invD

reg ray_Dx_16x[2]
reg ray_Dy_16x[2]
reg ray_Dz_16x[2]
reg ray_tmax_16x[2]
reg hit_u_16x[2]
reg hit_v_16x[2]
reg hit_id_16x[2]

reg ray_tmax_8x
reg hit_u_8x
reg hit_v_8x
reg hit_id_8x

reg crosses[4]
reg v0A[6]
reg ab[4]
reg t[2]
reg c[2]
reg v[2]
reg tri_data[24]

reg tri_idx[2]
reg tri_id
reg tri_end
reg tri_base[2]
reg tri_count



begin:

mov(1) ray_idx.u, r0.u1

// load ray:
//  address = 8*tid + 4 + {lane_index}
mul(8) ray_addr.u, ray_idx.u<0,1,0>, 8
add(8) ray_addr.u, ray_addr.u, INDICES.u
add(8) ray_addr.u, ray_addr.u, 4
send DwordLoad8(Rays), ray_data.f, ray_addr.u

// compute axis mask from direction sign bits
cmplt(8)(f0.0) null.f, ray_data.f, 0.0f
shr(1) axis_mask.u, f0.u0, 4

// shuffle BVH min/max based on direction sign
//   for positive rays we want to fetch min(xyz) then max(xyz)
//   for negative rays, the reverse
mov(8) bvh_indices.u, INDICES.u
mov(8) tmp0.u, 0
and(1) tmp0.u, axis_mask.u, 1
shl(1) tmp0.u, tmp0.u, 2
and(1) tmp0.u1, axis_mask.u, 2
shl(1) tmp0.u1, tmp0.u1, 1
and(1) tmp0.u2, axis_mask.u, 4
xor(1) axis_mask.u1, axis_mask.u0, 0x7 // store inverted axis mask in second channel

add(4) bvh_indices.u, bvh_indices.u, tmp0.u
sub(4) bvh_indices.u4, bvh_indices.u4, tmp0.u
mov(8) node_address.u, bvh_indices.u
send DwordLoad8(Nodes), node.u, node_address.u  // load first node
    


// precompute ray reciprocals.  produce copy of ray_data with directions inverted
mov(1) f0.us0, 0x70  // invert only lanes 4,5, and 6
mov(8) ray_data_rcp.f, ray_data.f
pred(f0.0){ 
    rcp(8) ray_data_rcp.f, ray_data.f 
}

// pre-swizzle ray origin and inverse direction
//  using regioning inside the loop incurs a moderate perf hit
mov(8) ray_O.f,    ray_data.f<0,4,1>
mov(8) ray_D.f,    ray_data.f4<0,4,1>
mov(8) ray_invD.f, ray_data_rcp.f4<0,4,1>

// replicate ray components for 18x intersection testing
mov(16)  ray_Dx_16x.f,    ray_D.f0<0,1,0>
mov(16)  ray_Dy_16x.f,    ray_D.f1<0,1,0>
mov(16)  ray_Dz_16x.f,    ray_D.f2<0,1,0>
mov(16)  ray_tmax_16x.f,  ray_data.f3<0,1,0>
mov(16)  hit_u_16x.f, 0
mov(16)  hit_v_16x.f, 0
mov(16)  hit_id_16x.u, 0xffffffff


mov(8) ONE.u, 1
mov(8) a0.us0, 0
mov(2) f0.us0, 0
mov(2) f1.us0, 0 // clear the flags, since we do a bunch of width-1 compares down in the loop


// begin traversal loop
traversal:

        
    // do ray-box intersection test
    //   We rely on certain magic regioning patterns here:
    //    reg.f<0,4,1>  ->  Replicates first 4 elements:  0,1,2,3,0,1,2,3
    //    reg.fn<4,0,1>  ->  Rotates a register left n places:  reg.f4<4,0,1> -> 4,5,6,7,0,1,2,3
    //    
    
    sub(8) tmp0.f, node.f,  ray_O.f  // t0 = BBMin-O(xyz),*,BBMax-O(xyz),*  
    mul(8) tmp1.f, tmp0.f,  ray_invD.f // t0 = t0 * inv_dir(xyz)
    min(1) tmax.f, tmp1.f4, tmp1.f5
    max(1) tmin.f, tmp1.f0, tmp1.f1     // now reduce each set of 't' values
    min(1) tmax.f, tmax.f,  tmp1.f6
    max(1) tmin.f, tmin.f,  tmp1.f2

    // NOTE: cmp instructions above do not use tmp0/tmp1, 
    //   but using null dest reg forces a thread switch, and it turns out this is expensive

    cmpgt(1)  (f0.0) tmp0.f, tmin.f, tmax.f
    cmplt(1)  (f0.1) tmp1.f, tmax.f, 0    
    cmpgt(16) (f1.0) tmp2.f, tmin.f<0,1,0>, ray_tmax_16x.f // fail if any( tmin>hit ).

    jmpif(f0.0) pop_stack     
    jmpif(f0.1) pop_stack 
    jmpif(f1.0) pop_stack 

    
    // see if node is a leaf
    and(2) tmp0.u, node.u7<0,1,0>, 3
    cmpeq(1) (f1.1) tmp1.u, tmp0.u, 3
    
    
    // choose traversal order based on direction signs
    //  axis_mask.u0 contains the ray direction sign bits
    //  axis_mask.u1 contains the inverse
    //  near node is 0 (positive,axis bit=0) or 1 (negative)
    bfe(2) nearfar.u, ONE.u, tmp0.u, axis_mask.u
    add(2) nearfar.u, nearfar.u, node.u3<0,1,0>

    jmpif(f1.1) visit_leaf  // node is a leaf. go intersect some triangles
                            // jump is done here to hide cmp() latency in these other ops
                            //   sizable speedup this way
    
    // start reading near node into working register for next traversal step
    mul(8) tmp0.u, nearfar.u0<0,1,0>, 8 
    add(8) node_address.u, tmp0.u, bvh_indices.u
    send DwordLoad8(Nodes), node.u, node_address.u 
    
    // start reading far node into stack head for some future traversal step
    mul(8) tmp0.u, nearfar.u1<0,1,0>, 8 
    add(8) node_address.u, tmp0.u, bvh_indices.u
    send DwordLoad8(Nodes), Stack[a0.0].u, node_address.u 
    add(1) a0.us0, a0.us0, 32
    
    jmp traversal

visit_leaf:
    
    mov(1) tri_id.u, node.u3
    shr(1) tri_count.u, node.u7, 2
    add(1) tri_end.u, tri_id.u, tri_count.u
    
  // mov(1) tri_id.u, 0
  // mov(1) tri_end.u, 5804

isect_loop:


    // intersect 8 independent tris starting with 'tri_id'
    add(16) tri_idx.u,   tri_id.u<0,1,0>, INDICES.u
    
    
    //struct TrianglePP
    //{
    //    vec3 P0;      // 0,1,2
    //    vec3 v02;     // 3,4,5
    //    vec3 v10;     // 6,7,8
    //    vec3 v10x02;
    //};
    
    // Load 12 dwords per triangle
   
    mul(16) tri_base.u, tri_idx.u, 48 // 48 bytes/tri
    send UntypedRead16x4(Triangles), tri_data.u, tri_base.u
    add(16) tri_base.u, tri_base.u, 16
    send UntypedRead16x4(Triangles), tri_data8.u, tri_base.u
    add(16) tri_base.u, tri_base.u, 16
    send UntypedRead16x4(Triangles), tri_data16.u, tri_base.u


    
    // v0A = P0 - origin (replicated)
    sub(16) v0A0.f, tri_data0.f, ray_O.f0<0,1,0>
    sub(16) v0A2.f, tri_data2.f, ray_O.f1<0,1,0>
    sub(16) v0A4.f, tri_data4.f, ray_O.f2<0,1,0>

    // compute two packed cros s products
    //   crosses[6] contains:  
    //      v02x0A (x)
    //      v10x0A (x)
    //      v02x0A (y)
    //      v10x0A (y) 
    //      v02x0A (z)
    //      v10x0A (z) 
    // v = dot(v10x02,ray_dir)
    // t = dot(v10x02,v0A)
    //  dot products interleaved
    // ab = {dot(v02x0a,D)..,dot(v10x0a,D)..}
    
    mul(16) tmp0.f,     tri_data8.f,  v0A4.f // y*z
    mul(16) tmp4.f,     tri_data10.f, v0A2.f // z*y
    mul(16) tmp2.f,     tri_data14.f, v0A4.f
    mul(16) tmp6.f,     tri_data16.f, v0A2.f
    sub(16) crosses0.f, tmp0.f, tmp4.f
    sub(16) crosses2.f, tmp2.f, tmp6.f
    mul(16) ab.f,  crosses.f,   ray_Dx_16x.f
    mul(16) ab2.f, crosses2.f,  ray_Dx_16x.f

    mul(16) tmp0.f,     tri_data10.f, v0A0.f // z*x
    mul(16) tmp4.f,     tri_data6.f,  v0A4.f // x*z
    mul(16) tmp2.f,     tri_data16.f, v0A0.f
    mul(16) tmp6.f,     tri_data12.f, v0A4.f
    sub(16) crosses0.f, tmp0.f, tmp4.f
    sub(16) crosses2.f, tmp2.f, tmp6.f
    fma(16) ab.f, crosses0.f,  ray_Dy_16x.f
    fma(16) ab2.f, crosses2.f, ray_Dy_16x.f
    
    mul(16) tmp0.f,     tri_data6.f, v0A2.f // x*y
    mul(16) tmp4.f,     tri_data8.f, v0A0.f // y*x
    mul(16) tmp2.f,     tri_data12.f, v0A2.f
    mul(16) tmp6.f,     tri_data14.f, v0A0.f
    sub(16) crosses0.f, tmp0.f, tmp4.f
    sub(16) crosses2.f,tmp2.f, tmp6.f
    fma(16) ab.f,  crosses0.f, ray_Dz_16x.f
    fma(16) ab2.f, crosses2.f, ray_Dz_16x.f
    
    mul(16)   v.f, tri_data18.f,  ray_Dx_16x.f
    mul(16)   t.f, tri_data18.f,  v0A0.f
    
    fma(16)   v.f, tri_data20.f, ray_Dy_16x.f
    fma(16)   t.f, tri_data20.f, v0A2.f
    
    fma(16)   v.f, tri_data22.f, ray_Dz_16x.f
    fma(16)   t.f, tri_data22.f, v0A4.f
    rcp(16)   v.f, v.f
    
    mul(16) ab.f,  ab.f,  v.f
    mul(16) ab2.f, ab2.f, v.f
    mul(16) t.f, t.f, v.f
    add(16) c.f, ab0.f, ab2.f
    
    cmplt(16) (f1.0) tmp0.f, t.f,   ray_tmax_16x.f
    cmpgt(16) (f1.0) tmp2.f, t.f,   0.0f
    cmpge(16) (f1.0) tmp4.f, ab0.f, 0.0f
    cmpge(16) (f1.0) tmp6.f, ab1.f, 0.0f
    cmple(16) (f1.0) tmp8.f, c.f,   1.0f
    and(16)  tmp0.u, tmp0.u, tmp2.u
    and(16)  tmp2.u, tmp4.u, tmp6.u
    and(16)  tmp0.u, tmp0.u, tmp2.u
    and(16)  tmp0.u, tmp0.u, tmp8.u
    cmpgt(16) (f1.0) tmp0.u, tmp0.u, 0
    

    // For lanes which hit, transfer hit information into 8-wide regs
    pred(f1.0)
    {
        mov(16) ray_tmax_16x.f, t.f 
        mov(16) hit_u_16x.f,  ab0.f
        mov(16) hit_v_16x.f,  ab1.f
    }

    // reduce down from 16 lanes to 8
    cmplt(8) (f1.0) tmp0.u, ray_tmax_16x.f, ray_tmax_16x1.f
    pred(f1.0)
    {
        mov(8) ray_tmax_8x.f, ray_tmax_16x.f0
        mov(8) hit_id_8x.u, hit_id_16x.u0
        mov(8) hit_u_8x.f, hit_u_16x.f0
        mov(8) hit_v_8x.f, hit_v_16x.f0
    }
    pred(~f1.0)
    {
        mov(8) ray_tmax_8x.f, ray_tmax_16x.f1
        mov(8) hit_id_8x.u, hit_id_16x.u1
        mov(8) hit_u_8x.f, hit_u_16x.f1
        mov(8) hit_v_8x.f, hit_v_16x.f1
    }




next_iter:
    mov(2) f0.us0, 0
    add(1) tri_id.u, tri_id.u, 8
    cmplt(1) (f0.0) tmp0.u, tri_id.u, tri_end.u
    jmpif(f0.0) isect_loop
    

pop_stack:
    
    // bail out if we've reached the bottom of the stack
    cmpeq(1) (f0.0) tmp0.u, a0.us0, 0

    // decrement stack ptr before the jump, because if the jump is taken, we don't care about underflow
    //  This gives a healthy speedup because the jump can cover the latency of the write to a0,
    //   and the add can cover the latency of the cmp
    add(1) a0.us0, a0.us0, -32       // NOTE: our 'sub' pneumonic doesn't work for immediate operands
    
    jmpif(f0.0) finished

    // pop the next node off the stack
    mov(8) node.u, Stack[a0.0].u
    
    jmp traversal


finished:

    // reduce down from 16 lanes to 8
    cmplt(8) (f1.0) tmp0.u, ray_tmax_16x.f, ray_tmax_16x1.f
    pred(f1.0)
    {
        mov(8) ray_tmax_8x.f, ray_tmax_16x.f0
        mov(8) hit_id_8x.u, hit_id_16x.u0
        mov(8) hit_u_8x.f, hit_u_16x.f0
        mov(8) hit_v_8x.f, hit_v_16x.f0
    }
    pred(~f1.0)
    {
        mov(8) ray_tmax_8x.f, ray_tmax_16x.f1
        mov(8) hit_id_8x.u, hit_id_16x.u1
        mov(8) hit_u_8x.f, hit_u_16x.f1
        mov(8) hit_v_8x.f, hit_v_16x.f1
    }


    // reduce vectorized hit information and pull out the nearest hit point

    // TODO:  This would suck a lot less if we could use DD control
    mov(1) HIT_INFO.f0, hit_u_8x.f0
    mov(1) HIT_INFO.f1, hit_v_8x.f0
    mov(1) HIT_INFO.u2, hit_id_8x.u0
    mov(1) HIT_INFO.f3, ray_tmax_8x.f0
    
    cmplt(1) (f0.0) null.f, ray_tmax_8x.f1, HIT_INFO.f3
    pred(f0.0)
    {
        mov(1) HIT_INFO.f0, hit_u_8x.f1
        mov(1) HIT_INFO.f1, hit_v_8x.f1
        mov(1) HIT_INFO.u2, hit_id_8x.u1
        mov(1) HIT_INFO.f3, ray_tmax_8x.f1
    }
    cmplt(1) (f0.0) null.f, ray_tmax_8x.f2, HIT_INFO.f3
    pred(f0.0)
    {
        mov(1) HIT_INFO.f0, hit_u_8x.f2
        mov(1) HIT_INFO.f1, hit_v_8x.f2
        mov(1) HIT_INFO.u2, hit_id_8x.u2
        mov(1) HIT_INFO.f3, ray_tmax_8x.f2
    }
    cmplt(1) (f0.0) null.f, ray_tmax_8x.f3, HIT_INFO.f3
    pred(f0.0)
    {
        mov(1) HIT_INFO.f0, hit_u_8x.f3
        mov(1) HIT_INFO.f1, hit_v_8x.f3
        mov(1) HIT_INFO.u2, hit_id_8x.u3
        mov(1) HIT_INFO.f3, ray_tmax_8x.f3
    }
    cmplt(1) (f0.0) null.f, ray_tmax_8x.f4, HIT_INFO.f3
    pred(f0.0)
    {
        mov(1) HIT_INFO.f0, hit_u_8x.f4
        mov(1) HIT_INFO.f1, hit_v_8x.f4
        mov(1) HIT_INFO.u2, hit_id_8x.u4
        mov(1) HIT_INFO.f3, ray_tmax_8x.f4
    }
    cmplt(1) (f0.0) null.f, ray_tmax_8x.f5, HIT_INFO.f3
    pred(f0.0)
    {
        mov(1) HIT_INFO.f0, hit_u_8x.f5
        mov(1) HIT_INFO.f1, hit_v_8x.f5
        mov(1) HIT_INFO.u2, hit_id_8x.u5
        mov(1) HIT_INFO.f3, ray_tmax_8x.f5
    }
    cmplt(1) (f0.0) null.u, ray_tmax_8x.f6, HIT_INFO.f3
    pred(f0.0)
    {
        mov(1) HIT_INFO.f0, hit_u_8x.f6
        mov(1) HIT_INFO.f1, hit_v_8x.f6
        mov(1) HIT_INFO.u2, hit_id_8x.u6
        mov(1) HIT_INFO.f3, ray_tmax_8x.f6
    }
    cmplt(1) (f0.0) null.f, ray_tmax_8x.f7, HIT_INFO.f3
    pred(f0.0)
    {
        mov(1) HIT_INFO.f0, hit_u_8x.f7
        mov(1) HIT_INFO.f1, hit_v_8x.f7
        mov(1) HIT_INFO.u2, hit_id_8x.u7
        mov(1) HIT_INFO.f3, ray_tmax_8x.f7
    }

    // HACK:  just min-reduce t values
   // min(4) tmp0.f, ray_tmax_8x.f0, ray_tmax_8x.f4
   // min(2) tmp1.f, tmp0.f, tmp0.f2
   // min(1) tmp2.f, tmp1.f, tmp1.f1
   // mov(1) HIT_INFO.f3, tmp2.f
    

    // Store hit info
    mov(8) blockwrite.u, 0
    mov(1) blockwrite.u2, ray_idx.u
    mov(8) blockwrite1.u, HIT_INFO.u
    send OWordBlockWrite(HitInfo), null.u, blockwrite.u

end






