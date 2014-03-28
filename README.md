# Kvad

Kvad is a [quadtree](http://en.wikipedia.org/wiki/Quadtree) data structure implementation in C99,
extracted from the [Superpin clustering library](http://getsuperpin.com).

## Usage

    void walk(double x, double y, void* payload, void* context) {
        printf("Found %s at %f,%f", payload, x, y);
    }

    kvad_tree* tree = kvad_tree_create(0, 0, 1000, 1000, 16 /* max number of levels */, 16 /* max points per level */);
    kvad_tree_insert(tree, 10, 10, "Place 1");
    kvad_tree_insert(tree, 10, 10, "Place 2");
    kvad_tree_insert(tree, 10, 10, "Place 3");
    kvad_tree_find(tree, 5, 5, 10, 10, walk, NULL); // Find all points in (5,5)-(15,15) rectangle
    kvad_tree_release(tree);

## Assertions

To enable internal assertions and checks on supplied arguments, define `KVAD_ASSERTS=1`.

If defined, the assertion macro Kvad uses internally will expand to a stdlib `assert()` call that
aborts the program if an inconstitent state is encountered. Use this in your debug builds.