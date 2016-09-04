
curbe INDICES[2] = {{0,1,2,3,4,5,6,7},
                    {8,9,10,11,12,13,14,15}}

curbe TRI_OFFSETS[2] = {{ 0,1,2,2,  9,10,11,11 },
                        { 3,4,5,5,  6,7,8,8 } }

curbe HIT_INFO[1] = {0,0,0xffffffff,0}


bind Rays       0x38  // { nrays,x,x,x [ox,oy,oz,tmax,dx,dy,dz,pad]... .}
bind HitInfo    0x39  // { bbmin(x,y,z),offs,bbmax(x,y,z),count_and_axis} .. see glsl shader for details
bind Nodes      0x3a
bind Triangles  0x3b

//NOTE: 'stack' must be declared early so that it is based near the top of the reg file
//  Otherwise we overflow the 9-bit signed address immediate field
reg Stack[32] 
reg ray_addr
reg ray_idx
reg num_rays
reg ray_data            // ox,oy,oz,tmax,dx,dy,dz,xxx
reg ray_data_rcp
reg axis_mask
reg node_address
reg t0
reg tmin
reg tmax
reg node
reg nearfar

reg blockwrite[2]
reg leaf_count

reg bvh_indices
reg tri_id
reg tri_end
reg tri_count
reg tri_base
reg tri_addr[4]
reg tri_data[4]
reg v0A
reg tmp[6]
reg crosses
reg v
reg t
reg c
reg ab
reg ONE

reg ray_O
reg ray_D
reg reg_invD


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
//    reg.f<0,4,1>  ->  Replicates first 4 elements:  0,1,2,3,0,1,2,3
    
mov(8) ray_O.f, ray_data.f<0,4,1>
mov(8) ray_D.f, ray_data.f4<0,4,1>
mov(8) ray_invD.f, ray_data_rcp.f4<0,4,1>

mov(8) ONE.u, 1
mov(8) a0.us0, 0
mov(2) f0.us0, 0
mov(2) f1.us0, 0 // clear the flags, since we do a bunch of width-1 compares down in the loop



// begin traversal loop
traversal:

        
    // do ray-box intersection test
    //   We rely on certain magic regioning patterns here:
    //    
    sub(8) tmp0.f, node.f,  ray_O.f  // t0 = BBMin-O(xyz),*,BBMax-O(xyz),*  
    mul(8) tmp1.f, tmp0.f,  ray_invD.f // t0 = t0 * inv_dir(xyz)
    min(1) tmax.f, tmp1.f4, tmp1.f5
    max(1) tmin.f, tmp1.f0, tmp1.f1     // now reduce each set of 't' values
    min(1) tmax.f, tmax.f,  tmp1.f6
    max(1) tmin.f, tmin.f,  tmp1.f2
    min(1) tmax.f, tmax.f,  ray_data_rcp.f3

    cmpgt(1) (f0.0) tmp0.f, tmin.f, tmax.f
    cmplt(1) (f0.1) tmp1.f, tmax.f, 0
    jmpif(f0.0) pop_stack     
    jmpif(f0.1) pop_stack // if ray missed, goto pop_stack
    
    // NOTE: cmp instructions above do not use results in tmp0/tmp1, 
    //   but using null dest reg forces a thread switch, and it turns out this is expensive

    // see if node is a leaf
    and(2) t0.u, node.u7<0,1,0>, 3
    cmpeq(1) (f1.1) tmp0.u, t0.u, 3
    
    
    
    // choose traversal order based on direction signs
    //  axis_mask.u0 contains the ray direction sign bits
    //  axis_mask.u1 contains the inverse
    //  near node is 0 (positive,axis bit=0) or 1 (negative)
    bfe(2) nearfar.u, ONE.u, t0.u, axis_mask.u
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
    
isect_loop:

    mul(8) tri_base.u, tri_id.u<0,1,0>, 12
    
    //struct TrianglePP
    //{
    //    vec3 P0;      // 0,1,2
    //    vec3 v02;     // 3,4,5
    //    vec3 v10;     // 6,7,8
    //    vec3 v10x02;
    //};
    
    // tri_data reg pair looks like this:
    //    P0(xyz) . v10x02(xyz).
    //    v02(xyz) . v10(xyz) .

    add(16) tri_addr.u, tri_base.u<0,1,0>, TRI_OFFSETS.u
    send DwordLoad16(Triangles), tri_data.u, tri_addr.u

    dp3(4) v.f,  tri_data0.f4, ray_data.f4 // v = dot(v10x02,Dir)
    
    // v0A = P0 - origin .   Replicate this to lower and upper halves of v0A
    sub(8) v0A.f, tri_data.f0<0,4,1>, ray_O.f
    
    rcp(4) v.f, v.f // v = 1/v
    
    // compute two packed cross products
    //   v02x0a (lower lanes)
    //   v10x0a (upper lanes)
    mul(8) tmp0.f, tri_data1.f(yzx), v0A.f(zxy)  // v02.yzx * v0A.zxy   v10.yzx*v0A.zxy
    mul(8) tmp1.f, tri_data1.f(zxy), v0A.f(yzx)  // v02.zxy * v0A.yzx   v10.zxy*v0A.yzx
    dp3(4) t.f, tri_data0.f4, v0A.f  // t = dot(v10x02,v0A)
    sub(8) crosses.f, tmp0.f, tmp1.f        // crosses={v02x0a (xyz) ..  v10x0a(xyz) ..}
    mul(4) t.f, t.f, v.f
    dp3(8) ab.f, crosses.f, ray_D.f // ab = {dot(v02x0a,D)..,dot(v10x0a,D)..}
    mul(8) ab.f, ab.f, v.f<0,4,1> // ab = ab * V
    add(4) c.f, ab.f, ab.f4
    
    
    cmplt(4)(f0.0) tmp0.f, t.f, ray_data_rcp.f3
    cmpgt(4)(f0.0) tmp1.f, t.f, 0.0f
    cmple(4)(f0.0) tmp2.f, c.f, 1.0f
    cmpge(8)(f0.0) tmp3.f, ab.f0, 0.0f

    and(1) tmp0.u, tmp0.u, tmp1.u
    and(1) tmp1.u, tmp2.u, tmp3.u
    and(1) tmp0.u, tmp0.u, tmp3.u4
    and(1) tmp1.u, tmp0.u, tmp1.u
    
    mov(2) f0.us0, 0
    cmpeq(1) (f0.0) tmp2.u, tmp1.u, 0
    jmpif(f0.0) next_iter
    

  
    
    // if we reach this point we have hit the triangle
    // TODO: I am slow and should use the ddchk bit
    mov(1) HIT_INFO.u2, tri_id.u
    mov(1) HIT_INFO.f0, ab.f0
    mov(1) HIT_INFO.f1, ab.f4
    mov(1) HIT_INFO.f3, t.f0
    mov(1) ray_data_rcp.f3, t.f0



next_iter:
    mov(2) f0.us0, 0
    add(1) tri_id.u, tri_id.u, 1
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

    // Store hit info

    mov(8) blockwrite.u, 0
    mov(1) blockwrite.u2, ray_idx.u
    mov(8) blockwrite1.u, HIT_INFO.u
    send OWordBlockWrite(HitInfo), null.u, blockwrite.u

end






