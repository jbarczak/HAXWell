//=====================================================================================================================
//
//   TRTMultiBVHTraversal.h
//
//   Single-ray traversal through multi-branching BVH data structures
//
//   Part of the TinyRT Raytracing Library.
//   Author: Joshua Barczak
//
//   Copyright 2008 Joshua Barczak.  All rights reserved.
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRT_MULTIBVHTRAVERSAL_H_
#define _TRT_MULTIBVHTRAVERSAL_H_


namespace TinyRT
{
    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Searches for the first intersection between a ray and an object in an N-ary BVH
    /// \param MBVH_T       Must implement MBVH_C
    /// \param ObjectSet_T  Must implement ObjectSet_C
    /// \param HitInfo_T    Must implement HitInfo_C
    /// \param Ray_T        Must implement Ray_C
    //=====================================================================================================================
    template< typename MBVH_T, typename ObjectSet_T, typename HitInfo_T, typename Ray_T >
    void RaycastMultiBVH( const MBVH_T* pBVH, const ObjectSet_T* pObjects, Ray_T& rRay, HitInfo_T& rHitInfo, const typename MBVH_T::ConstNodeHandle pRoot, ScratchMemory& rScratch )
    {
        typedef typename MBVH_T::ConstNodeHandle ConstNodeHandle;
        typedef typename MBVH_T::obj_id obj_id;

        const Vec3f& rOrigin = rRay.Origin();
        const Vec3f& rInvDir = rRay.InvDirection();

        int nDirSigns[4] = {
            rInvDir.x > 0 ? 0 : 1,
            rInvDir.y > 0 ? 0 : 1,
            rInvDir.z > 0 ? 0 : 1
        };
        nDirSigns[3] =  (nDirSigns[2] << 2) | (nDirSigns[1] << 1) | (nDirSigns[0]);
        nDirSigns[0] <<= 4;
        nDirSigns[1] <<= 4;
        nDirSigns[2] <<= 4;

        SimdVec4f vSIMDRay[6] = {
            SimdVec4f( rInvDir.x ), SimdVec4f( rOrigin.x ),
            SimdVec4f( rInvDir.y ), SimdVec4f( rOrigin.y ),
            SimdVec4f( rInvDir.z ), SimdVec4f( rOrigin.z )
        };

        size_t nStackSize = pBVH->GetStackDepth()*(MBVH_T::BRANCH_FACTOR);
        ScratchArray<ConstNodeHandle> pStackMem( rScratch, nStackSize );
        ConstNodeHandle* pStack = pStackMem;
        const ConstNodeHandle* pStackBottom = pStack;
        (*pStack++) = pRoot;


        while( pStack != pStackBottom )
        {
            pStack--;
            ConstNodeHandle pNode = *pStack;

            if( pBVH->IsNodeLeaf( pNode ) )
            {
                obj_id nFirstObject;
                obj_id nLastObject;
                pBVH->GetNodeObjectRange( pNode, nFirstObject, nLastObject );
                
                pObjects->RayIntersect( rRay, rHitInfo, nFirstObject, nLastObject );
            }
            else
            {
                pStack = pBVH->RayIntersectChildren( pNode, vSIMDRay, rRay, pStack, nDirSigns );
            }
        }
    }

}

#endif // _TRT_MULTIBVHTRAVERSAL_H_
