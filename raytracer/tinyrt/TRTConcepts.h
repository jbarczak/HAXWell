
/*!
 \defgroup TRTConcepts TinyRT Concepts
 \brief A group of classes which define the interfaces used in TinyRT templates.

 TRT, by convention, uses a system of "concepts".  These are similar in spirit to the concept construct that
is defined by C++0x, but since 0x has dropped concepts, TRT approaches concepts using well defined conventions.

All template type names in TRT template definitions will be of the form:  Name_T.  Here, "Name" will be the name of the concept
to which corresponding types in the template instantiations must adhere.  Any type used to instantiate a template MUST adhere 
to the corresponding concept type in order to ensure correct behavior.

Concepts are documented in the TRTConcepts.h file, which defines a set of concept definition classes.  These classes are 
named with the convention "Type_C", where "Type" is again the name of the associated concept.  The _C classes are not used 
anywhere in the TRT codebase, and are not meant to be used by library clients.  These class definitions serve only to document the
requirements for the types used in TRT templates.

The collection of types, typedefs, and methods in a TRT concept class will be referred to as a "concept signature".  Any type 
adhering to a concept must provide methods or types which match those in the signature.


Any member classes in concept signatures should be thought of as "inlined" concept definitions.  They specify that
adhering types must have a member type with the specified name, which adheres to the concept signature of the member class.

Typedefs in concept signatures specify that an adhering class must have a type with the specified name, which adheres to
another concept.  


Where appropriate, inheritance may be used in concept definitions.  When inheritance is present, any types adhering to a
"derived" concept are required to also adhere to all of its "base" concepts.

Note that in order for a templatized code to do anything useful, there must generally be BEHAVIORAL constraints
 assigned to the concept methods, beyond their simple signatures.  There is no way to express these
  constraints syntactically, so they are described in english comments, which should be considered part of the concept signature.

Any types which are intended to adhere to a TRT concept must exhibit the specified behavior for each method in the 
concept signature. 
*/

namespace TinyRT
{
    class AxisAlignedBox;



    /// \ingroup TRTConcepts
    /// \brief Any type which stores information about a ray hit location
    struct HitInfo_C
    {
    };

    /// \ingroup TRTConcepts
    /// \brief  A type representing a ray
    ///
    ///  All rays have an origin, a direction, and an inverse direction.   Rays also possess the notion
    ///   of a 'valid region', which is a range of values of the ray parameter in which intersections 
    ///   should be reported.  Some of TRT's raycasting methods will adjust the valid region as intersections are found
    /// \sa TRTConcepts
    /// \sa Ray, EpsilonRay
    struct Ray_C
    {
    public:
  
        /// Returns the ray origin
        virtual const Vec3f& Origin() const = 0;

        /// Returns the (possibly unnormalized) ray direction
        virtual const Vec3f& Direction() const = 0;

        /// Returns the reciprocal of the ray direction
        virtual const Vec3f& InvDirection() const = 0;

        /// Returns the lower limit of the ray's valid region
        virtual float MinDistance() const = 0;

        /// Returns the upper limit of the ray's valid region
        virtual float MaxDistance() const = 0;

        /// Sets the upper limit of the ray's valid region
        virtual void SetMaxDistance( float t )  = 0;

        /// Determines whether the specified distance is in the ray's valid region
        virtual bool IsDistanceValid( float t ) const = 0;

        /// Determines whether any location on the specified distance interval is in the ray's valid region
        virtual bool IsIntervalValid( float fmin, float fmax ) const = 0;

        /// Vectorized version of 'IsIntervalValid'
        virtual SimdVec4f AreIntervalsValid( const SimdVec4f& rMin, const SimdVec4f& rMax ) const = 0;

        /// Vectorized version of 'AreDistancesValid'
        virtual SimdVec4f AreDistancesValid( const SimdVec4f& rT ) const  = 0;
        
    };

    /// \ingroup TRTConcepts
    /// \brief An interface for a generic tree structure, whose nodes store a variable number of "objects"
    /// \sa TRTConcepts
    struct Tree_C
    {
        /// A handle to a node in the tree
        class NodeHandle {};

        /// A 'const' handle to a node in the tree
        class ConstNodeHandle : public NodeHandle {};
        
        /// Returns a handle to the tree root
        virtual ConstNodeHandle GetRoot() const = 0;
        
        /// \brief Returns a handle to one of a node's children.  
        /// May not be called on an inner node
        virtual ConstNodeHandle GetChild( ConstNodeHandle n, size_t i ) const = 0;
        
        /// \brief Returns the number of children of a node (generally a constant, but not necessarily).  
        /// May not be called on a leaf
        virtual size_t GetChildCount( ConstNodeHandle n ) const = 0;
        
        /// Returns true if the specified node is a leaf
        virtual bool IsNodeLeaf( ConstNodeHandle n ) const = 0;
        
        /// \brief Determines the number of object stored in the given node.  
        /// May not be called on an inner node
        virtual int GetNodeObjectCount( ConstNodeHandle n ) const = 0;

    };

    /// \ingroup TRTConcepts
    /// \brief A container of objects which may be raytraced
    /// \sa TRTConcepts
    /// \sa BasicMesh, StridedMesh
    struct ObjectSet_C
    {
        typedef unsigned int obj_id;     ///< Type used as an object identifier. Must be an integral type
      
        /// \brief Performs an intersection test between this object and a ray
        virtual void RayIntersect( Ray_C& rRay, HitInfo_C& rHitInfo, obj_id nObject ) const;

        /// \brief Performs an intersection test between a range of objects and a ray
        virtual void RayIntersect( Ray_C& rRay, HitInfo_C& rHitInfo, obj_id nObject, obj_id nCount ) const;

        /// \brief Re-arranages the IDs of objects in the object set
        /// This operation is needed when building object-order data structures such as AABB trees, which 
        ///   rely on the objects being stored in a particular order.  TinyRT::RemapArray may be used as a quick means of implementation
        ///
        /// \param pObjectRemap  An array containing the index of the object that belongs in position i
        virtual void RemapObjects( obj_id* pObjectRemap );

        /// Computes the AABB of the entire object set
        virtual void GetAABB( AxisAlignedBox& rBox );

        /// Computes the AABB of a particular object in the object set
        virtual void GetObjectAABB( obj_id nObject, AxisAlignedBox& pBoxOut );
        
        /// Returns the number of objects in the set
        virtual obj_id GetObjectCount() const;


    };

    /// \ingroup TRTConcepts
    /// \brief A set of methods for clipping objects against planes
    ///
    ///  This class is used during 'perfect-splits' KD tree construction 
    /// \sa TRTConcepts
    /// \sa BoxClipper, TriangleClipper
    struct Clipper_C
    {
        /// \brief Clips an object to an axis-aligned plane, and computes the bounding boxes of the two halves
        ///
        /// Given an object and a subset of its AABB, this method clips the object by an axis-aligned plane, computes the 
        ///  AABBs of the two halves, and then returns their intersection with the input AABB
        /// \param pObjects      Object set whose objects are being clipped
        /// \param nObject       Id of the object being clipped
        /// \param rOldBB        The axis-aligned box of the portion of the object being clipped.  This box must intersect the clipping plane
        /// \param fPosition     Location of axis aligned split plane
        /// \param nAxis         Index of the axis to which the plane is aligned
        /// \param rLeftSideOut  Receives the AABB of the back (lower) portion 
        /// \param rRightSideOut Receives the AABB of the front (upper) portion
        static void ClipObjectToAxisAlignedPlane( const ObjectSet_C* pObjects, 
                                                 ObjectSet_C::obj_id nObject,
                                                 const AxisAlignedBox& rOldBB, 
                                                 float fPosition, 
                                                 uint nAxis,
                                                 AxisAlignedBox& rLeftSideOut, 
                                                 AxisAlignedBox& rRightSideOut ) {};
    };


    /// \ingroup TRTConcepts
    /// \brief Interface for an indexed triangle mesh
    /// \sa TRTConcepts
    struct Mesh_C 
    {
        typedef float Position_T[3];    ///< 3-component vector type representing a vertex position
        typedef unsigned int face_id;   ///< Integral type corresponding to a face id
        typedef unsigned int Index_T;   ///< Integral type corresponding to a vertex index

        /// Accessor for the tree vertex positions of a triangle
        virtual void GetTriangleVertexPositions( face_id nFace, Position_T pVerticesOut[3] ) = 0;
    };


    /// \ingroup TRTConcepts
    /// \brief Interface for a KD tree
    ///
    ///  In a KD tree, each leaf node contains a list of objects, and each inner node contains
    ///   an axis aligned splitting plane which subdivides space
    ///
    /// \sa TRTConcepts
    /// \sa RaycastKDTree
    /// \sa ObjectKDTree_C
    /// \sa PointKDTree_C
    struct KDTree_C : public Tree_C
    {
    public:

        /// Returns the bounding box of the tree
        virtual const AxisAlignedBox& GetBoundingBox() const = 0;

        /// Returns the left (less-than) child of a node in this tree
        virtual ConstNodeHandle GetLeftChild( ConstNodeHandle p ) const  = 0;
        
        /// Returns the right (greater-than) child of a node in this tree
        virtual ConstNodeHandle GetRightChild( ConstNodeHandle p ) const = 0;

        /// Returns the maximum depth of any node in the tree.  This is used to allocate stack space during traversal
        virtual size_t GetStackDepth() const = 0;    
    
        /// Returns the split axis (0,1,2 for a particular node)
        virtual uint GetNodeSplitAxis( ConstNodeHandle p ) const = 0;

        /// Returns the split plane location for a particular node
        virtual float GetNodeSplitPosition( ConstNodeHandle p ) const = 0;
        
        /// Tests whether or not a particular node is a leaf
        virtual bool IsNodeLeaf( ConstNodeHandle p ) const = 0;

        /// Subdivides a particular node, creating two empty leaf children for it
        virtual std::pair<NodeHandle,NodeHandle> MakeInnerNode( NodeHandle hNode, float fSplitPlane, uint nSplitAxis ) = 0;

    };

    /// \ingroup TRTConcepts
    /// \brief Interface for a KD tree over objects with spatial extent
    ///
    ///  In a object KD tree, each leaf node contains a list of object references, and objects may span multiple leaves
    ///
    /// \sa TRTConcepts
    /// \sa RaycastKDTree
    struct ObjectKDTree_C : public KDTree_C
    {
        typedef ObjectSet_C ObjectSet;
        typedef unsigned int obj_id;     ///< An object identifier.  Must be an integral type
        typedef const obj_id* LeafIterator; ///< Type that acts as an iterator over objects.  Must support standard iterator semantics

        /// Returns a pair of iterators over the objects in a leaf node
        virtual void GetNodeObjectList( ConstNodeHandle p, LeafIterator& start, LeafIterator& end ) const= 0;

        /// Clears existing nodes in the tree and creates a new, single-node tree with the specified bounding box, returns the root node
        virtual NodeHandle Initialize( const AxisAlignedBox& rRootAABB ) = 0;

        /// Turns the specified node into a leaf containing the given set of object references.  Returns an object ref array which the caller must fill
        virtual obj_id* MakeLeafNode( NodeHandle n, obj_id nObjCount )  = 0;
    
    };

    /// \ingroup TRTConcepts
    /// \brief Interface for a KD tree over points
    ///
    ///  In a point KD tree, each leaf node contains a range of contiguous objects, and each object resides in a single leaf.
    ///   Points in the left child of a node are <= to its split plane
    ///
    /// \sa TRTConcepts
    /// \sa KNNSearchKDTree
    /// \sa PointKDTree
    /// \sa Point_C
    struct PointKDTree_C : public KDTree_C
    {
        typedef unsigned int PointID;       ///< Type for a point index.  Must be integral
        typedef unsigned int PointCount;    ///< Type for the point count in a leaf.  Must be integral

        /// Returns the point range for a leaf node
        /// \param p        Handle to the node
        /// \param start    Index of the first point in the node
        /// \param count    Number of points in the node
        virtual void GetNodeObjectRange( ConstNodeHandle p, PointID& start, PointCount& count ) const = 0;
        
        /// Clears existing nodes in the tree and creates a new, single-node tree with the specified bounding box
        /// \param rRootAABB    AABB of the point set that will go into this tree
        /// \param nMaxNodes    Expected number of nodes that will be created for this tree (not the maximum)
        /// \return Handle to the root node
        virtual NodeHandle Initialize( const AxisAlignedBox& rRootAABB, uint nMaxNodes ) = 0;

   
        /// Turns the specified node into a leaf containing the specified point range
        /// \param n            Node to turn into a leaf
        /// \param nFirstObj    Index of the first point in the leaf
        /// \param nPointCount  Number of points in the leaf
        virtual void MakeLeafNode ( NodeHandle n, PointID nFirstObj, PointCount nObjCount ) = 0;
    };

    /// \ingroup TRTConcepts
    /// \brief An interface for a data item in a PointKDTree
    ///
    /// \sa TRTConcepts
    /// \sa KNNSearchKDTree
    /// \sa PointKDTree
    /// \sa PointKDTree_C
    /// \sa PointQuery_C
    struct Point_C 
    {
        /// Returns the position of the point
        virtual Vec3f GetPosition() const { return Vec3f(0,0,0); };

    };

    /// \ingroup TRTConcepts
    /// \brief An interface for an object which process points retrieved from a search
    ///
    ///  A point query is a search for points within a specified radius of a given query location
    ///   TRT's search algorithms will call the 'AddPoint' method on a query object as points are found
    ///   This allows the search criterion to be abstracted away from the mechanics of the search
    ///
    /// \sa TRTConcepts
    /// \sa KNNSearchKDTree
    /// \sa Point_C
    /// \sa KNNQuery
    /// \sa NearestQuery
    struct PointQuery_C
    {
        /// Returns the current maximum radius, squared.  Depending on the search algorithm, this may change during the course of a search
        virtual float GetQueryRadiusSq() const = 0;

        /// Returns the query point.  This is assumed to stay constant over the duration of the search
        virtual Vec3f GetQueryPosition() const = 0;
    
        /// Called by the search algorithm to indicate that a particular point is within the search distance
        /// \param pPoint       The point to add
        /// \param fDistanceSq  Squared distance from the query to the point
        virtual void AddPoint( const Point_C* pPoint, float fDistanceSq ) = 0;

    };


    /// \ingroup TRTConcepts
    /// \brief An interface for a generic bounding volume hierarchy
    ///
    ///  A BVH is a binary tree where each node contains a bounding volume.  The bounding volumes of parent nodes
    ///  fully enclose those of their children.  Leaf nodes in the BVH store a range of object indices.  This concept
    ///  class defines the basic interface used in TinyRT for raycasting against AABB trees
    /// \sa TRTConcepts
    /// \sa RaycastBVH
    struct BVH_C : public Tree_C
    {
        typedef int obj_id;             ///< Type used as an object identifier. Must be an integral type
        typedef ObjectSet_C ObjectSet;

        class NodeHandle{};        ///< This should be a type that acts as a reference to a node

        /// Tests whether a ray strikes a bounding volume, returning the hit distance
        /// \param p    Handle to a node whose bounding volume is tested
        /// \param rRay Ray which is to be tested
        /// \param rfTOut Receives the hit distance to the ray hit location (unmodified in the event of a miss)
        /// \return True if the ray strikes the node's bounding volume
        virtual bool RayNodeTest( ConstNodeHandle p, const Ray_C& rRay, float& rfTOut ) const = 0;

        /// Tests whether a ray strikes a bounding volume
        /// \param p    Handle to a node whose bounding volume is tested
        /// \param rRay Ray which is to be tested
        /// \return True if the ray strikes the node's bounding volume
        virtual bool RayNodeTest( ConstNodeHandle p, const Ray_C& rRay ) const = 0;

        /// Retrieves the range of objects in a leaf node
        /// \param p        Node whose objects are desired
        /// \param rFirst   Receives the first object ID in the object range
        /// \param rLast    Receives the last object ID in the object range
        virtual void GetNodeObjectRange( ConstNodeHandle p, obj_id& rFirst, obj_id& rLast ) const = 0;

        /// Returns the best axis to use for approximate ordered traversal of a node's two children
        virtual uint32 GetNodeSplitAxis( ConstNodeHandle pNode ) const = 0;

        /// Tests whether a node is a leaf
        virtual bool IsNodeLeaf( ConstNodeHandle pNode ) const = 0;
        
        /// Returns a node's left child
        virtual ConstNodeHandle GetLeftChild( ConstNodeHandle pNode ) const  = 0;
        
        /// Returns a node's right child
        virtual ConstNodeHandle GetRightChild( ConstNodeHandle pNode ) const = 0;
        
        /// Returns the maximum depth of any leaf node
        virtual size_t GetStackDepth() const                     = 0;

    };

    /// \ingroup TRTConcepts
    /// \brief Interface for a BVH whose BVs are axis aligned bounding boxes
    ///
    ///  This concept defines the interface for constructing AABB trees using TRT's AABB tree builders
    /// \sa TRTConcepts
    /// \sa RaycastBVH
    struct AABBTree_C : public BVH_C
    {

        /// Creates and returns the root node of the tree, discarding any existing nodes
        /// This is called by tree builders at the start of tree construction
        /// \param rRootBox     The root bounding box of the tree to be created
        /// \param nMaxNodes    The maximum number of nodes that will be created in the tree
        /// \return The root node of the new tree.  NULL on failure
        virtual NodeHandle Initialize( const AxisAlignedBox& rRootBox, uint32 nMaxNodes ) = 0;

        /// Modifes the AABB of the specified node
        virtual void SetNodeAABB( NodeHandle, const AxisAlignedBox& rBox ) = 0;

        /// Converts the specified node into a leaf node, and stores a range of objects in it
        /// This is called by tree builders during tree constuction
        /// \param p        Handle to the node to be made a leaf
        /// \param firstObj Index of the first object in this leaf in the OUTPUT object ordering.
        ///                    Tree builders will call ObjectSet_C::RemapObjects in order to sort the objects into a particular
        ///                      order defined by the tree leaves.   This is the index of the first object in the resulting order.
        /// \param nObjectCount Number of objects to go into this leaf
        virtual void MakeLeafNode( NodeHandle p, obj_id firstObj, obj_id nObjectCount );
        
        /// Turns the specified node into an inner node, and allocates a pair of child nodes for it
        /// Returns an std::pair containing the children.  
        /// This is called by tree builders during tree construction
        virtual std::pair<NodeHandle,NodeHandle> MakeInnerNode( NodeHandle p, uint32 nSplitAxis );


    };


    /// \ingroup TRTConcepts
    /// \brief A spatial data structure consisting of a regular grid of cells
    ///
    ///  Each cell in a uniform grid stores a list of object IDs
    /// \sa TRTConcepts
    /// \sa RaycastUniformGrid
    struct UniformGrid_C
    {
        typedef unsigned int obj_id;     ///< Type used as an object identifier. Must be an integral type
        typedef obj_id* CellIterator; ///< Type that acts as an iterator over objects.  Must support standard iterator semantics
            
        typedef uint32 UnsignedCellIndex;   ///< Type used as a cell index (unsigned)
        typedef int32 SignedCellIndex;      ///< Signed type with the same bit width as 'UnsignedCellIndex

        /// Returns the bounding box of the grid
        virtual const AxisAlignedBox& GetBoundingBox() const =0;

        /// Returns the number of grid cells along each axis
        virtual Vec3<UnsignedCellIndex>& GetCellCounts() const = 0;

        /// Returns the number of objects in a cell list.  
        /// \param pCell    The cell list.  May NOT be NULL
        virtual size_t GetCellObjectCount( const Vec3<UnsignedCellIndex>& rCell ) const = 0;

        /// Retrieves a list of objects stored in a particular cell
        virtual void GetCellObjectList( const Vec3<UnsignedCellIndex>& rCell, CellIterator& hCellStart, CellIterator& hCellEnd ) const =0; 
        
    };

    /// \ingroup TRTConcepts
    /// \brief Interface for a mailboxing algorithm.
    ///
    /// A mailbox is a cache which records previous object intersection tests, in order to prevent
    ///   repeated intersection tests against the same object.  The mailbox is acts as a sort of cache, which records
    ///   intersection tests which were performed previously.  During raytracing, calls to 'CheckMailbox' will be made
    ///   using object IDs from the data structure
    /// \sa TRTConcepts
    struct Mailbox_C
    {
        /// Constructs a mailbox which tracks intersections for objects in a particular object set
        inline Mailbox_C( const ObjectSet_C* pObjects );
        
        /// Determines whether an intersection test was performed against an object since the instantiation of the mailbox.
        /// If a previous test has occurred, true is returned.  If not, false is returned, and the given object id is stored in the cache.  
        /// Note that this method need not return a strictly correct result.  This will not be the case when so-called "inverse mailboxing" schemes are in use
        inline bool CheckMailbox( const int& rRef ) const { return false; };
    };


    /// \ingroup TRTConcepts
    /// \brief Interface for a per-object cost function
    /// \sa TRTConcepts
    struct CostFunction_C
    {
        /// Computes the cost of the object with the specified ID
        virtual float operator()( uint obj_id ) const = 0;
    };

    /// \ingroup TRTConcepts
    /// \brief Interface for a policy class which controls splitting decisions when constructing tree data structures
    /// The split policy class provides a method for controlling split/leaf decisions in AABB and KD tree builders, 
    ///  based on the object count. and recursion depth.  
    ///
    ///  Potential uses include:  
    ///     - Setting cost to infinity to enforce a maximum object count per leaf
    ///     - Setting cost to 0 to enforce a maximum recursion depth
    ///     - Biasing the cost to encourage particular counts (e.g.  encouraging multiples of 4 for SSE, etc)
    ///
    ///
    /// \sa TRTConcepts
    struct SplitPolicy_C
    {

        
        /// \param nRecursionDepth  The current depth in the tree (0 at the root)
        /// \param nObjectCount     The number of objects being considered in a leaf/split decision
        /// \param fLeafCost        The cost of creating a leaf, according to the cost metric being used
        /// \return A modified cost, based on the object count and leaf cost.  If the implementation does not wish to
        ///          change the cost, it should simply return fLeafCost.  
        virtual float AdjustLeafCost( uint nRecursionDepth, uint nObjectCount, float fLeafCost ) const = 0;

    };

    /// \ingroup TRTConcepts
    /// \brief Interface for a non-binary BVH (a 'multi-BVH')
    /// This concept is used for raycasting against all non-binary BVHs
    /// \sa TRTConcepts
    /// \sa RaycastMultiBVH
    struct MBVH_C : public Tree_C
    {
        typedef unsigned int obj_id;  ///< Type used as an object identifier. Must be an integral type
        class ConstNodeHandle {};     ///< \brief A 'const' handle to a node
        
        static const int BRANCH_FACTOR = 4; ///< The number of children for each node

        /// Returns a handle to the root node
        virtual ConstNodeHandle GetRoot() = 0;

        /// Returns the maximum depth of any leaf node
        virtual size_t GetStackDepth() const                     = 0;

        /// Tests whether or not the specified node is a leaf
        virtual bool IsNodeLeaf( ConstNodeHandle n ) const = 0;

        /// Returns the range of object stored in a leaf node.  May only be called on a leaf
        virtual void GetNodeObjectRange( ConstNodeHandle n, obj_id& rFirst, obj_id& rLast ) const = 0;

        /// \brief Performs a ray intersection test against the children of a node, pushing any hit nodes onto the given stack
        /// \param nNode    The node to be tested
        /// \param rRay     The ray
        /// \param pStack   The traversal stack.  Each time a hit is found, it is placed on the stack, and the stack is incremented
        /// \param nDirSigns    Elements 0-2 contain the sign bits of the ray directions (0 if positive, 1 if negative), 
        ///                      element 3 contains a mask formed as follows:  signs[2]<<2 | signs[1]<<1 | signs[0]. 
        ///                     This encodes the octant index of the ray.  This information may be used to speed up ray/node intersection tests
        /// \return The new top of the stack, after the visited children are pushed.  If no children are hit, pStack is returned
        template< class Ray_T >
        ConstNodeHandle* RayIntersectChildren( ConstNodeHandle nNode, const Ray_T& rRay, ConstNodeHandle* pStack, const int nDirSigns[4] ) const { return 0; };

    };


    /// \ingroup TRTConcepts
    /// \brief Interface for a Quaternary AABB tree (QBVH)
    /// This concept is used for constructing quad AABB trees.  
    /// \sa TRTConcepts
    /// \sa SahAABBTreeBuilder::BuildQuadAABBTree()
    /// \sa QuadAABBTree
    struct QuadAABBTree_C : public MBVH_C
    {
        typedef unsigned int obj_id;  ///< Type used as an object identifier. Must be an integral type


        /// \brief Creates and returns the root node of the tree, discarding any existing nodes
        /// This is called by tree builders at the start of tree construction.
        /// \param rRootBox     The root bounding box of the tree to be created
        /// \return The root node of the new tree
        virtual NodeHandle Initialize( const AxisAlignedBox& rRootBox )  = 0;

        /// \brief Subdivides a child node into four children, and returns a reference to the subdivided child
        /// \param nNode    The node whose child should be subdivided.  Must be a non-leaf
        /// \param nChild   Index of the child to be subdivided.  The child must be a leaf
        virtual NodeHandle SubdivideChild( NodeHandle nNode, uint32 nChild ) = 0;

        /// Sets the split axes that are used for ordered traversal of a particular node
        virtual void SetSplitAxes( NodeHandle nNode, uint32 nA0, uint32 nA1, uint32 nA2 )= 0;
    
        /// Turns a child of a QBVH node into a leaf
        virtual void CreateLeafChild( NodeHandle nNode, uint32 nChildIdx, obj_id nFirstObject, obj_id nObjects )= 0; 

        /// Turns a child of a QBVH node into an empty leaf
        virtual void CreateEmptyLeafChild( NodeHandle pNode, uint32 nChildIdx )= 0;
   
        /// Sets the stored AABB for one of a node's children
        virtual void SetChildAABB( NodeHandle nNode, uint32 nChildIdx, const AxisAlignedBox& rBox )= 0;
   
    };


};

