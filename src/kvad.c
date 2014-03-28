// Kvad - a C99 quadtree implementation
// http://github.com/appscape/kvad

#include "kvad.h"

#include <stdlib.h>
#include <stdbool.h>

// Activate asserts only if KVAD_ASSERTS=1 is defined
#if KVAD_ASSERTS
#include <assert.h>
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr) /* */
#endif

typedef struct { double x; double y; } kvad_point;
typedef struct { double width; double height; } kvad_size;
typedef struct { kvad_point origin; kvad_size size; } kvad_rect;

// A linked list element storing a point and the reference to associated payload
struct _kvad_data {
    kvad_point point;
    void* payload;
    struct _kvad_data* next;
};

typedef struct _kvad_data kvad_data;

struct _kvad_node {
    unsigned int level; // This node's level in the tree
    kvad_rect rect; // Bounding box
    struct _kvad_node* subnodes[4]; // 0=NW,1=NE,2=SW,3=SE quadrants
    kvad_data* data; // Pointer to the first point data (linked list)
    unsigned int data_count; // Number of points stored in the linked list
};

typedef struct _kvad_node kvad_node;

struct _kvad_tree {
    unsigned int max_levels;
    unsigned int max_points_per_node;
    kvad_node* root_node;
};

static inline bool _kvad_rect_contains_point(kvad_rect rect, kvad_point point) {
    return ((point.x >= rect.origin.x) && (point.x <= rect.origin.x + rect.size.width) &&
            (point.y >= rect.origin.y) && (point.y <= rect.origin.y + rect.size.height));
}

static inline bool _kvad_rect_intersects_rect(kvad_rect r0, kvad_rect r1) {
    double r0_max_x = r0.origin.x + r0.size.width;
    double r1_max_x = r1.origin.x + r1.size.width;
    double r0_max_y = r0.origin.y + r0.size.height;
    double r1_max_y = r1.origin.y + r1.size.height;

    if ((r0_max_x < r1.origin.x) || (r0.origin.x > r1_max_x) ||
        (r0_max_y < r1.origin.y) || (r0.origin.y > r1_max_y)) {
        return false;
    } else {
        return true;
    }
}

static kvad_node* _kvad_node_create(kvad_rect rect, unsigned int level) {
    kvad_node* n = malloc(sizeof(kvad_node));
    if (!n) return NULL;
    n->rect = rect;
    n->level = level;
    n->data = NULL;
    n->data_count = 0;
    for (size_t i=0;i<4;i++) { n->subnodes[i] = NULL; }
    return n;
}

static kvad_node* _kvad_node_subnode_at_point(kvad_node *node, kvad_point point) {
    // Assert all subnodes are initialized
    for (size_t i=0;i<4;i++) { ASSERT(node->subnodes[i]); }
    ASSERT(!node->data); //..and non-leaf node has no data
    ASSERT(node->data_count == 0);

    // Determine which subnode to insert into
    double mid_x = node->rect.origin.x + node->rect.size.width / 2.0;
    double mid_y = node->rect.origin.y + node->rect.size.height / 2.0;

    size_t quadrant = (point.y > mid_y) * 2 + (point.x > mid_x);
    ASSERT(quadrant < 4);
    return node->subnodes[quadrant];
}

static unsigned long _kvad_node_remove_payload(kvad_node* node, kvad_point point, void* payload) {
    if (node->subnodes[0]) {
        return _kvad_node_remove_payload(_kvad_node_subnode_at_point(node, point), point, payload);
    } else {
        unsigned long count = 0;

        kvad_data *data = node->data;
        kvad_data *prev_data = NULL;

        while(data) {
            if (data->payload == payload) {
                // Payload matches, remove this entry
                count++;
                node->data_count--;
                if (prev_data) {
                    prev_data->next = data->next;
                    free(data);
                    data = prev_data->next;
                } else {
                    node->data = data->next;
                    free(data);
                    data = node->data;
                }
            } else {
                prev_data = data;
                data = data->next;
            }
        }

        return count;
    }
}

static void _kvad_node_insert(kvad_tree* tree, kvad_node* node, kvad_point point, void* payload) {
    ASSERT(tree);
    ASSERT(node);
    ASSERT(_kvad_rect_contains_point(node->rect, point));
    if (!_kvad_rect_contains_point(node->rect, point)) return;

    if (node->subnodes[0]) {
        // Non-leaf node
        _kvad_node_insert(tree, _kvad_node_subnode_at_point(node, point), point, payload);
    } else {
        // Leaf node, insert here by creating a new element at the head of linked list
        kvad_data* new_data = malloc(sizeof(kvad_data));
        new_data->point = point;
        new_data->payload = payload;
        new_data->next = NULL;
        if (node->data) {
            ASSERT(node->data_count > 0);
            new_data->next = node->data;
        }
        node->data = new_data;
        node->data_count++;

        if (node->level < tree->max_levels && node->data_count > tree->max_points_per_node) {
            // Maximum number of points reached AND below max level: split this node
            kvad_size subnode_rect_size = {node->rect.size.width / 2.0, node->rect.size.height / 2.0};

            for (size_t i=0;i<4;i++) {
                kvad_rect r = {{
                    node->rect.origin.x + (i%2==1 ? subnode_rect_size.width : 0),
                    node->rect.origin.y + (i>1 ? subnode_rect_size.height : 0)
                },subnode_rect_size};
                node->subnodes[i] = _kvad_node_create(r, node->level+1);
            }

            // ..and move data from it into subnodes
            kvad_data* data = node->data;
            node->data = NULL;
            node->data_count = 0;

            while (data) {
                _kvad_node_insert(tree, node, data->point, data->payload);
                kvad_data* next = data->next;
                free(data);
                data = next;
            }
        }
    }
}

static unsigned long _kvad_node_find(kvad_node* node, kvad_rect rect, kvad_callback callback, void* context) {
    unsigned long count = 0;
    if (node->subnodes[0]) {
        // Non-leaf node
        for (size_t i=0;i<4;i++) {
            if (_kvad_rect_intersects_rect(node->subnodes[i]->rect, rect)) {
                count += _kvad_node_find(node->subnodes[i], rect, callback, context);
            }
        }
    } else {
        // Leaf node
        kvad_data* data = node->data;
        while (data) {
            if (_kvad_rect_contains_point(rect, data->point)) {
                count++;
                if (callback) callback(data->point.x, data->point.y, data->payload, context);
            }
            data = data->next;
        }
    }
    return count;
}

static unsigned long _kvad_node_walk(kvad_node* node, kvad_callback callback, void* context) {
    unsigned long count = 0;
    if (node->subnodes[0]) {
        for (size_t i=0;i<4;i++) {
            count += _kvad_node_walk(node->subnodes[i], callback, context);
        }
    } else {
        kvad_data* data = node->data;
        while(data) {
            count++;
            if (callback) {
                callback(data->point.x, data->point.y, data->payload, context);
            }
            data = data->next;
        }
        ASSERT(count == node->data_count);
    }
    return count;
}

static void _kvad_node_release(kvad_node* node) {
    if (node->subnodes[0]) {
        // Non-leaf node: first free subnodes..
        for (size_t i=0;i<4;i++) {
            _kvad_node_release(node->subnodes[i]);
            free(node->subnodes[i]);
        }
    } else {
        // Leaf node: free data..
        kvad_data *data = node->data;
        while (data) {
            kvad_data *curr = data;
            data = data->next;
            free(curr);
        }
    }
    //..then free this node
    free(node);
}

// Public functions

kvad_tree* kvad_tree_create(double x, double y, double width, double height, unsigned int max_levels,
                            unsigned int max_points_per_node) {
    ASSERT(width > 0);
    ASSERT(height > 0);
    ASSERT(max_points_per_node > 0);
    // Sane behaviour in case asserts are disabled
    if (max_points_per_node == 0) max_points_per_node = 1;
    kvad_tree* t = malloc(sizeof(kvad_tree));
    if (!t) return NULL;
    t->max_levels = max_levels;
    t->max_points_per_node = max_points_per_node;
    kvad_rect world_rect = {{x, y}, {width, height}};
    t->root_node = _kvad_node_create(world_rect, 0);
    if (!t->root_node) return NULL;
    return t;
}

void kvad_tree_release(kvad_tree* tree) {
    _kvad_node_release(tree->root_node);
    free(tree);
}

void kvad_tree_insert(kvad_tree* tree, double x, double y, void* payload) {
    kvad_point p = {x, y};
    _kvad_node_insert(tree, tree->root_node, p, payload);
}

unsigned long kvad_tree_remove_payload(kvad_tree* tree, double x, double y, void* payload) {
    kvad_point p = {x,y};
    return _kvad_node_remove_payload(tree->root_node, p, payload);
}

unsigned long kvad_tree_find(kvad_tree* tree, double x, double y, double width, double height,
                             kvad_callback callback, void* context) {
    kvad_rect r = {{x,y},{width, height}};
    return _kvad_node_find(tree->root_node, r, callback, context);
}

unsigned long kvad_tree_walk(kvad_tree* tree, kvad_callback callback, void* context) {
    return _kvad_node_walk(tree->root_node, callback, context);
}