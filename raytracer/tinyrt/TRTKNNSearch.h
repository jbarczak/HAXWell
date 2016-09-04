//=====================================================================================================================
//
//   TRTKNNSearch..h
//
//   Definition of class: TinyRT::KNNSearch
//
//   Part of the TinyRT raytracing library.
//   Author: Joshua Barczak
//   Copyright 2010 Joshua Barczak.  All rights reserved.
//
//   See  Doc/LICENSE.txt for terms and conditions.
//
//=====================================================================================================================

#ifndef _TRTKNNSEARCH_H_
#define _TRTKNNSEARCH_H_


namespace TinyRT
{

    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Utility class to store the results of a K-Nearest neighbor search
    ///
    ///  This is a query class that can be used to implement K-nearest search based solely on point positions.
    ///  If additional search criteria are required, then this class can be subclassed or wrapped as needed
    ///
    ///  Implements the PointQuery_C concept
    ///
    /// \sa KNNSearchKDTree
    /// \sa PointQuery_C
    //=====================================================================================================================
    template< class Point_T >
    class KNNQuery
    {
    public:
    
        /// A point result returned from a KNN query
        struct Result
        {
            const Point_T* pPoint;  ///< A point which is one of the K-nearest neighbors
            float fDistanceSq;      ///< Squared distance from the query position to the point

            /// Comparison operator based on squared distances
            inline bool operator<( const Result& rhs ) const { return fDistanceSq < rhs.fDistanceSq; };
        };
      
        /// \param K            Number of nearest points to locate
        /// \param pResults     Array of K result structures for the query.  These are user-supplied. 
        ///                       The result structures must persist for the lifetime of the query
        /// \param fMaxRadius   Initial maximum search radius for the query
        /// \param rLocation    Query location
        inline KNNQuery( uint K, Result* pResults, float fMaxRadius, const Vec3f& rLocation ) 
            : m_pResults(pResults), 
              m_nMaxResults(K), 
              m_nResults(0), 
              m_fMaxRadiusSq( fMaxRadius*fMaxRadius ),
              m_vLocation(rLocation)
        {
        };

        /// Accessor for the query results
        inline const Result* Results( ) const { return m_pResults; };
        
        /// Returns the number of query results
        inline uint ResultCount( ) const { return m_nResults; };


        /// Returns the search position
        inline const Vec3f& GetQueryPosition() const { return m_vLocation; };

        /// Returns the current maximum (squared) distance to any point.  If there are less than K points, the query's radius is returned
        inline float GetQueryRadiusSq() const { return (m_nResults < m_nMaxResults) ? m_fMaxRadiusSq : m_pResults[0].fDistanceSq; };

        /// Called during KNN search to add a point to the result set
        inline void AddPoint( const Point_T* pPoint, float fDistanceSq )
        {
            if( m_nResults == m_nMaxResults )
            {
                // results are in heap-order.  Top is furthest.  Discard top one, then percollate new one down
                
                uint nRootIndex=0;
                while( 1 )
                {
                    uint nFirstChild = (2*nRootIndex)+1;

                    // leaf node. Insert here....
                    if( nFirstChild >= m_nResults )
                        break;

                    uint nSecondChild = nFirstChild+1;

                    // figure out which node is largest..... root, or one of root's children

                    // find largest child
                    uint nLargest;
                    if( nSecondChild >= m_nResults || m_pResults[nFirstChild].fDistanceSq >= m_pResults[nSecondChild].fDistanceSq )
                       nLargest = nFirstChild;
                    else
                       nLargest = nSecondChild;

                    // if new value is larger than each of its children.  We will insert it here
                    if( fDistanceSq >= m_pResults[nLargest].fDistanceSq )
                        break;

                    // one of the children is larger than new node.  Move it up to the root and walk downwards
                    m_pResults[nRootIndex] = m_pResults[nLargest];
                    nRootIndex = nLargest;
                }

                // done traversing.  Put the new point into its place
                m_pResults[nRootIndex].pPoint = pPoint;
                m_pResults[nRootIndex].fDistanceSq = fDistanceSq;
            }
            else
            {
                m_pResults[m_nResults].pPoint = pPoint;
                m_pResults[m_nResults].fDistanceSq = fDistanceSq;
                m_nResults++;
                
                // if we've filled the result set, build a max-heap out of them so we can discard the largest 
                if( m_nResults == m_nMaxResults )
                    std::make_heap( m_pResults, m_pResults + m_nResults );                
            }
        }

    private:

        Result* m_pResults;
        uint    m_nResults;
        uint    m_nMaxResults;
        float   m_fMaxRadiusSq;
        Vec3f   m_vLocation;
    };


    //=====================================================================================================================
    /// \ingroup TinyRT
    /// \brief Utility class to store the results of a 1-Nearest neighbor search
    ///
    ///  Implements the PointQuery_C concept
    ///
    /// \sa KNNSearchKDTree
    /// \sa PointQuery_C
    //=====================================================================================================================
    template< class Point_T >
    class NearestQuery
    {
    public:

        /// \param rLocation    The query position
        /// \param fMaxDistance Maximum distance over which to search
        inline NearestQuery( const Vec3f& rLocation, float fMaxDistance ) 
            : m_fRadiusSq(fMaxDistance*fMaxDistance), 
            m_pResult(0), 
            m_vLocation(rLocation)
        {
        }
 
        /// Returns the point that was found (or NULL)
        inline const Point_T* GetResult() const { return m_pResult; };

        /// Returns the search position
        inline const Vec3f& GetQueryPosition() const { return m_vLocation; };

        /// Returns the current maximum (squared) distance to the point. If no point has been found, returns the initial maximum distance (squared)
        inline float GetQueryRadiusSq() const { return m_fRadiusSq; };

        /// Called during KNN search to add a point to the result set.  Discards the earlier one, if any
        inline void AddPoint( const Point_T* pPoint, float fRadiusSq )
        {
            m_fRadiusSq = fRadiusSq;
            m_pResult = pPoint;
        }

    private:
        const Point_T* m_pResult;
        float m_fRadiusSq;
        Vec3f m_vLocation;
    };



    //=====================================================================================================================
    /// \brief Performs a nearest neighbor search of a point KD tree.
    ///
    ///  In a nearest neighbor search, the tree is traversed in near to far order, and the search radius is assumed to
    ///   shrink after each point is processed.  This type of search is best suited for finding the nearest point to a query position.
    ///
    ///  Note that this function does NOT trivially reject based on the tree bounding box.  
    ///   If this is useful, it must be done by the caller
    ///
    /// \param pPoints  Array of points indexed by the tree.  Must implement the 'Point_C' concept
    /// \param rTree    Tree to be searched. Must implement the PointKDTree_C concept
    /// \param hRoot    Node in the tree to start the search from
    /// \param rQuery   Query object which receives potential points.  Must implement the 'PointQuery_C' concept  
    ///                   The query's AddPoint method will be called for any points which are within its search radius
    ///                     at the time they are traversed.  Note that calls may be made for points which are not 
    ///                     among the K-nearest points.  The query implementation is responsible for filtering these out
    ///
    /// \param rScratch Scratch array used to allocate stack space
    ///
    /// \sa Point_C PointKDTree_C PointQuery_C KNNQuery, NearestQuery, PointKDTree
    //=====================================================================================================================
    template< class Point_T, class PointKDTree_T, class PointQuery_T >
    void KNNSearchKDTree( const Point_T* pPoints, 
                           const PointKDTree_T& rTree, 
                           typename PointKDTree_T::ConstNodeHandle hRoot, 
                           PointQuery_T& rQuery, 
                           ScratchMemory& rScratch )
    {
        // allocate stack space
        typedef std::pair<typename PointKDTree_T::ConstNodeHandle, float> StackType;
        ScratchArray< StackType > StackMem( rScratch, rTree.GetStackDepth() );
        StackType* pStack = StackMem;
        StackType* pStackBottom = StackMem;

        Vec3f vQueryLoc = rQuery.GetQueryPosition();

        while( 1 )
        {
            // Phase 1: descend to leaf in near to far order, fill stack
            float fCurrentRadiusSq = rQuery.GetQueryRadiusSq();
            while( !rTree.IsNodeLeaf( hRoot ) )
            {
                float fSplit = rTree.GetNodeSplitPosition( hRoot );
                uint nAxis = rTree.GetNodeSplitAxis( hRoot );
                uint nFirst = (vQueryLoc[nAxis] <= fSplit) ? 0 : 1; // choose side to visit first
                
                float fPlaneDistSq = (fSplit - vQueryLoc[nAxis]) * (fSplit-vQueryLoc[nAxis]);
                
                // push far side if we're close enough
                if( fPlaneDistSq < fCurrentRadiusSq )
                {
                    pStack->second = fPlaneDistSq;
                    pStack->first = rTree.GetChild(hRoot, nFirst ^ 1 );
                    pStack++;
                }

                // walk to near side
                hRoot = rTree.GetChild( hRoot, nFirst );
            }
            
            // Phase 2: evaluate points in leaf
            typename PointKDTree_T::PointID    nFirstPoint;
            typename PointKDTree_T::PointCount nPointCount;
            rTree.GetNodeObjectRange( hRoot, nFirstPoint, nPointCount );
            
            const Point_T* pFirst = pPoints + nFirstPoint;
            const Point_T* pLast  = pPoints + (nFirstPoint+nPointCount);
            while( pFirst != pLast )
            {
                Vec3f vDir = pFirst->GetPosition() - vQueryLoc;
                float fDistanceSq = Dot3( vDir, vDir );
                if( fDistanceSq < fCurrentRadiusSq )
                {
                    rQuery.AddPoint( pFirst, fDistanceSq );
                    fCurrentRadiusSq = rQuery.GetQueryRadiusSq();
                }
                
                ++pFirst;
            }

            // Phase 3: Pop stack till we find a node we still need to visit, then resume from there
            do
            {
                if( pStack == pStackBottom ) // no nodes left to visit.  Bail out
                    return;

                pStack--;
            } while( pStack->second > fCurrentRadiusSq );

            hRoot = pStack->first;
        } 
    }


    //=====================================================================================================================
    /// \brief Performs a range search of a point KD tree.
    ///
    ///  In a range neighbor search, the tree is traversed in arbitrary order, and the search radius is assumed to
    ///   remain fixed.  This type of search can be used to find all points within a spherical region
    ///
    ///  Note that this function does NOT trivially reject based on the tree bounding box.  
    ///   If this is useful, it must be done by the caller
    ///
    /// \param pPoints  Array of points indexed by the tree.  Must implement the 'Point_C' concept
    /// \param rTree    Tree to be searched. Must implement the PointKDTree_C concept
    /// \param hRoot    Node in the tree to start the search from
    /// \param rQuery   Query object which receives potential points.  Must implement the 'PointQuery_C' concept  
    ///                   The query's AddPoint method will be called once for each point that lies within the search radius
    ///                   The search radius must remain invariant across calls to 'AddPoint'
    ///
    /// \param rScratch Scratch array used to allocate stack space
    ///
    /// \sa Point_C PointKDTree_C PointQuery_C PointKDTree
    //=====================================================================================================================
    template< class Point_T, class PointKDTree_T, class PointQuery_T >
    void RangeSearchKDTree( const Point_T* pPoints, 
                           const PointKDTree_T& rTree, 
                           typename PointKDTree_T::ConstNodeHandle hRoot, 
                           PointQuery_T& rQuery, 
                           ScratchMemory& rScratch )
    {
        // allocate stack space
        ScratchArray< typename PointKDTree_T::ConstNodeHandle > StackMem( rScratch, rTree.GetStackDepth() );
        typename PointKDTree_T::ConstNodeHandle* pStack = StackMem;
        typename PointKDTree_T::ConstNodeHandle* pStackBottom = StackMem;

        Vec3f vQueryLoc = rQuery.GetQueryPosition();
        float fRadiusSq = rQuery.GetQueryRadiusSq();

        while( 1 )
        {
            // Phase 1: descend to leaf in near to far order, fill stack
            while( !rTree.IsNodeLeaf( hRoot ) )
            {
                float fSplit = rTree.GetNodeSplitPosition( hRoot );
                uint nAxis = rTree.GetNodeSplitAxis( hRoot );
                float fPlaneDist = (vQueryLoc[nAxis]-fSplit);
                float fPlaneDistSq = fPlaneDist*fPlaneDist;
                
                if( fPlaneDist <= 0 )
                {
                    // left side
                    if( fPlaneDistSq < fRadiusSq )
                        *(pStack++) = rTree.GetChild(hRoot,1);
                    hRoot = rTree.GetChild( hRoot, 0 );
                }
                else
                {
                    // right side
                    if( fPlaneDistSq < fRadiusSq )
                        *(pStack++) = rTree.GetChild(hRoot,0);
                    hRoot = rTree.GetChild(hRoot,1);
                }
            }
            
            // Phase 2: evaluate points in leaf
            typename PointKDTree_T::PointID    nFirstPoint;
            typename PointKDTree_T::PointCount nPointCount;
            rTree.GetNodeObjectRange( hRoot, nFirstPoint, nPointCount );
            
            const Point_T* pFirst = pPoints + nFirstPoint;
            const Point_T* pLast  = pPoints + (nFirstPoint+nPointCount);
            while( pFirst != pLast )
            {
                Vec3f vDir = pFirst->GetPosition() - vQueryLoc;
                float fDistanceSq = Dot3( vDir, vDir );
                if( fDistanceSq < fRadiusSq )
                    rQuery.AddPoint( pFirst, fDistanceSq );
                
                ++pFirst;
            }

            if( pStack == pStackBottom ) // no nodes left to visit.  Bail out
                return;

            hRoot = *(--pStack);
        } 
    }
}

#endif // _TRTKNNSEARCH_H_
