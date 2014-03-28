// Kvad - a C99 quadtree implementation
// http://github.com/appscape/kvad

/** Opaque struct representing the quadtree. */
typedef struct _kvad_tree kvad_tree;

typedef void (*kvad_callback)(double x, double y, void* payload, void* context);

/**
 Creates the quadtree. Note that you must manually release the tree using 
 kvad_tree_release().
 
 Operations on the quadtree are not thread-safe.

 @param x
        The x coordinate of the bounding box. Can be negative.
 @param y
        The y coordinate of the bounding box. Can be negative.
 @param width
        Width of the bounding box.
 @param height
        Height of the bounding box.
 @param max_levels
        Maximum number of levels. If maximum level is reached, the nodes are not split anymore 
        and points are added to the node at the last level, ignoring max_points_per_node.
 @param max_points_per_node
        Maximum number of points to store in a single node. After this many points are added to
        a single node, it is split into into 4 subnodes/quadrants.
 @return Pointer to the quadtree object if successful, otherwise NULL
 */
kvad_tree* kvad_tree_create(double x, double y, double width, double height, unsigned int max_levels,
                            unsigned int max_points_per_node);

/** Releases the quadtree, freeing up memory. */
void kvad_tree_release(kvad_tree* tree);

/** Inserts a point and an associated payload into the quadtree. You are responsible for
    making sure that payload reference remains valid during the tree lifecycle.
    No check for duplicates is performed.
*/
void kvad_tree_insert(kvad_tree* tree, double x, double y, void* payload);

/** Removes a point with the matching coordinates and payload from the quadtree.
    Point is identified the payload, coordinates are just used to speed up the lookup of the point
    to be removed.
    @return Number of points removed.
 */
unsigned long kvad_tree_remove_payload(kvad_tree* tree, double x, double y, void* payload);

/** Searches for points inside the rectangle. Calls supplied callback for every matched point.
    @return Number of points found.
 */
unsigned long kvad_tree_find(kvad_tree* tree, double x, double y, double width, double height,
                             kvad_callback callback, void *context);

/** Visits all points in the tree.
    @return Total number of points currently stored in the tree.
 */
unsigned long kvad_tree_walk(kvad_tree* tree, kvad_callback callback, void* context);