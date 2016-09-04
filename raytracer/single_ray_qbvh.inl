

curbe TID[1]= {0}
curbe INDICES[1] = {0,1,2,3,4,5,6,7}
curbe HIT_INFO[1] = {0,0,0xffffffff,0}


bind Rays       0x38  // { nrays,x,x,x [ox,oy,oz,tmax,dx,dy,dz,pad]... .}
bind HitInfo    0x39  // { bbmin(x,y,z),offs,bbmax(x,y,z),count_and_axis} .. see glsl shader for details
bind Nodes      0x3a
bind Triangles  0x3b


reg ray_addr
reg ray_idx
reg num_rays
reg ray_data            // ox,oy,oz,tmax,dx,dy,dz,xxx
reg ray_data_rcp
reg axis_mask
reg near
reg far
reg node_address
reg t0
reg tmin
reg tmax
reg node
reg nearfar
reg Stack[8]
reg blockwrite[2]
reg leaf_count

reg TRI_OFFS[2]

reg tri_id
reg tri_end
reg tri_count
reg tri_base
reg tri_addr[4]
reg tri_data[4]
reg v0A
reg tmp[2]
reg crosses
reg v
reg t
reg ab

begin:

mov(8) TRI_OFFS0.us0, imm_uvec(0,1,2,2, 0,1,2,2)       // P0.xyzz, P0.xyzz
mov(8) TRI_OFFS0.us8, imm_uvec(4,5,3,3, 7,8,6,6)       // v02(yzxx), v10(yzxx)
mov(8) TRI_OFFS1.us0, imm_uvec(5,3,4,4, 8,6,7,7)       // v02(zxyy), v10(zxyy)
mov(8) TRI_OFFS1.us8, imm_uvec(9,10,11,12,9,10,11,12)  // v10x02 (replicated)
   /*
// if tid > num_rays, return
//mov(1) ray_idx.u, r0.u1
mul(1) ray_idx.u, r0.u1, 1
add(1) ray_idx.u, ray_idx.u, TID.u
send DwordLoad8(Rays), num_rays.u, INDICES.u
cmpge(1) (f0.0) null.u, ray_idx.u, num_rays.u
jmpif(f0.0) finished

*/
// load ray:
//  address = 8*tid + 4 + {lane_index}
mul(8) ray_addr.u, ray_idx.u<0,1,0>, 8
add(8) ray_addr.u, ray_addr.u, INDICES.u 
add(8) ray_addr.u, ray_addr.u, 4
send DwordLoad8(Rays), ray_data.f, ray_addr.u



// precompute reciprocals.  produce copy of ray_data with directions inverted
mov(8) ray_data_rcp.f, ray_data.f
mov(1) f0.us0, 0x70  // invert only lanes 4,5, and 6
pred(f0.0){ 
    rcp(8) ray_data_rcp.f, ray_data.f 
}

// compute axis mask from direction sign bits
cmplt(8)(f0.0) null.f, ray_data.f, 0.0f
shr(1) axis_mask.u, f0.u0, 4

mov(2) f0.u0, 0
mov(2) f1.u0, 0 // clear the flags, since we do a bunch of width-1 compares down in the loop

mov(8) leaf_count.u,0

mov(8) node_address.u, INDICES.u
mov(1) a0.us0, 0

// begin traversal loop
traversal:

    // load current node
    send DwordLoad8(Nodes), node.u, node_address.u 
    
    //   min   max
    //   xxxx xxxx 
    //   yyyy yyyy 
    //   zzzz zzzz
    //    empty_leaf_mask (dword)
    //  precomp. traversal orders (2x dword)
    //   nnnn  <- child node ptrs
    //   nnnn  <- child node tri counts
    //
    sub(8) tx.f, node0.f, ray_ox.f
    sub(8) ty.f, node1.f, ray_oy.f
    sub(8) tz.f, node2.f, ray_oz.f

    mul(8) tx.f, tx.f, ray_inv_dx.f
    mul(8) ty.f, ty.f, ray_inv_dy.f
    mul(8) tz.f, tz.f, ray_inv_dz.f

    min(4) tmax.f, tx.f, ty.f
    max(4) tmin.f, tx.f, ty.f
    min(4) tmax.f, tmax.f, tz.f
    max(4) tmin.f, tmin.f, tz.f
    min(4) tmax.f, tmax.f, ray_tmax.f
    cmpgt(4) (f0.0) tmp0.f, tmin.f, tmax.f
    cmplt(4) (f1.0) tmp1.f, tmax.f, 0    


    /*

    // do ray-box intersection test
    //   We rely on certain magic regioning patterns here:
    //    reg.f<0,4,1>  ->  Replicates first 4 elements:  0,1,2,3,0,1,2,3
    //    reg.fn<4,0,1>  ->  Rotates a register left n places:  reg.f4<4,0,1> -> 4,5,6,7,0,1,2,3
    //    
    sub(8) t0.f, node.f, ray_data_rcp.f<0,4,1>  // t0 = BBMin-O(xyz),*,BBMax-O(xyz),*  
    mul(8) t0.f, t0.f,   ray_data_rcp.f4<0,4,1> // t0 = t0 * inv_dir(xyz)
    min(4) tmin.f, t0.f, t0.f4<4,0,1>  // tmin = min(t0, rotate_left(t0) )
    max(4) tmax.f, t0.f, t0.f4<4,0,1>  // tmax = max(t0, rotate_left(t0) )
    max(1) tmin.f, tmin.f, tmin.f1     // now reduce each set of 't' values
    min(1) tmax.f, tmax.f, tmax.f1
    max(1) tmin.f, tmin.f, tmin.f2
    min(1) tmax.f, tmax.f, tmax.f2
    
    min(1) tmax.f, tmax.f, ray_data_rcp.f3
    cmpgt(1) (f0.0) null.f, tmin.f, tmax.f
    cmplt(1) (f1.0) null.f, tmax.f, 0
    jmpif(f0.0) pop_stack     
    jmpif(f1.0) pop_stack // if ray missed, goto pop_stack
        */
    // we hit the node.  Is it a leaf?

    and(1) t0.u, node.u7, 3
    cmpeq(1) (f0.0) null.u, t0.u, 3
    jmpif(f0.0) visit_leaf  // yes. go intersect some triangles

    // no.  Manipulate stack and continue

    // choose traversal order based on direction signs
    shr(1) t0.u, axis_mask.u, t0.u
    and(1) nearfar.u0, t0.u,  1
    xor(1) nearfar.u1, nearfar.u0, 1
    add(2) nearfar.u, nearfar.u, node.u3<0,1,0>

    // push far node onto stack and visit near node
    mov(1) Stack[a0.0].u, nearfar.u1
    add(1) a0.us0, a0.us0, 4
    mul(8) node_address.u, nearfar.u0<0,1,0>, 8 
    add(8) node_address.u, node_address.u, INDICES.u
    
     
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
    
    // NOTE:  The 'half-byte' integers cannot be used directly in an 'add' unless
    //  the data type is short.   The hardware doesn't do expansion of half-byte vectors to dword
    //    Lame....
    
    add(16) tri_addr0.u, tri_base.u<0,1,0>, TRI_OFFS0.us
    send DwordLoad16(Triangles), tri_data.u, tri_addr.u

    add(16) tri_addr2.u, tri_base.u<0,1,0>, TRI_OFFS1.us
    send DwordLoad16(Triangles), tri_data2.u, tri_addr2.u
    

    // v0A = P0 - origin (replicated)
    sub(8) v0A.f, tri_data.f, ray_data.f<0,4,1>
    
    // compute two packed cross products
    //   v02x0a (lower lanes)
    //   v10x0a (upper lanes)
    
    mul(8) tmp0.f, tri_data1.f, v0A.f(zxy)  // v02.yzx * v0A.zxy   v10.yzx*v0A.zxy
    mul(8) tmp1.f, tri_data2.f, v0A.f(yzx)  // v02.zxy * v0A.yzx   v10.zxy*v0A.yzx
    sub(8) crosses.f, tmp0.f, tmp1.f        // crosses={v02x0a (xyz) ..  v10x0a(xyz) ..}
    dp3(4) v.f,  tri_data3.f, ray_data.f4
    
    dp3(8) ab.f, crosses.f, ray_data.f4<0,4,1> // ab = {dot(v02x0a,D)..,dot(v10x0a,D)..}
    rcp(4) v.f, v.f
    
    dp3(4) t.f, tri_data3.f, v0A.f
    mul(8) ab.f, ab.f, v.f<0,4,1> // ab = ab * V
    mul(4) t.f, t.f, v.f
    add(4) tmp0.f, ab.f, ab.f4<0,4,1>
    
    // TODO: Tried this, but it hangs
    //cmpgt(4) (f0.0) null.f, t.f, ray_data_rcp.f3<0,1,0>
    //cmplt(4) (f0.1) null.f, t.f, 0
    //cmplt(8) (f1.0) null.f, ab.f, 0
    //cmpgt(4) (f1.1) null.f, tmp0.f, 1.0f
    //or(1) f0.u0, f0.u0, f1.u0
    //jmpif(f0.0) next_iter
    //jmpif(f0.1) next_iter
    
    mov(2) f0.us0, 0
    cmpge(1) (f0.0) null.f, t.f, ray_data_rcp.f3
    jmpif(f0.0) next_iter

    cmplt(1) (f0.0) null.f, t.f, 0.0f
    jmpif(f0.0) next_iter
    
    cmplt(8) (f0.0) null.f, ab.f, 0.0f
    jmpif(f0.0) next_iter
    
    cmpgt(1) (f0.0) null.f, tmp0.f, 1.0f
    jmpif(f0.0) next_iter
   
    
    // if we reach this point we have hit the triangle
    // TODO: I am slow
    mov(1) HIT_INFO.u2, tri_id.u
    mov(1) HIT_INFO.f0, ab.f0
    mov(1) HIT_INFO.f1, ab.f4
    mov(1) HIT_INFO.f3, t.f0
    mov(1) ray_data_rcp.f3, t.f0

next_iter:
    mov(2) f0.us0, 0
    add(1) tri_id.u, tri_id.u, 1
    cmplt(1) (f0.0) null.u, tri_id.u, tri_end.u
    jmpif(f0.0) isect_loop
    

pop_stack:

    // bail out if we've reached the bottom of the stack
    cmpeq(1) (f0.0) null.u, a0.us0, 0
    jmpif(f0.0) finished

    // pop the next node address off the stack
    add(1) a0.us0, a0.us0, -4       // NOTE: our 'sub' pneumonic doesn't work for immediate operands
    mov(1) t0.u, Stack[a0.0].u
    mul(8) node_address.u, t0.u<0,1,0>, 8
    add(8) node_address.u, node_address.u, INDICES.u
    
    jmp traversal


finished:

    // Store hit info

    mov(8) blockwrite.u, 0
    mov(1) blockwrite.u2, ray_idx.u
    mov(8) blockwrite1.u, HIT_INFO.u
    send OWordBlockWrite(HitInfo), null.u, blockwrite.u

end

