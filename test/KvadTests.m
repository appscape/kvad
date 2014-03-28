#import <XCTest/XCTest.h>
#import "kvad.h"

@interface KvadTests : XCTestCase
@end

@implementation KvadTests

static void KvadArrayAppendCallback(double x, double y, void *payload, void *context) {
    CFSetAddValue(context, payload);
}

- (void)testBasic {
    void* payload = @"foo";

    kvad_tree *tree = kvad_tree_create(0, 0, 100, 100, 32, 6);
    kvad_tree_insert(tree, 5, 5, payload);
    kvad_tree_insert(tree, 5, 5, payload);
    kvad_tree_insert(tree, 5, 5, payload);
    kvad_tree_insert(tree, 5.5, 5, payload);

    unsigned long count = kvad_tree_walk(tree, NULL, NULL);
    XCTAssertEqual(count, 4);

    count = kvad_tree_find(tree, -5.0, -5.0, 10.0, 10.0, NULL, NULL);
    XCTAssertEqual(count, 3);

    count = kvad_tree_find(tree, -5.0, -5.0, 11.0, 10.0, NULL, NULL);
    XCTAssertEqual(count, 4);

    // Even 5.5 is removed, because coordinates are ignored
    count = kvad_tree_remove_payload(tree, 5, 5, payload);
    XCTAssertEqual(count, 4);

    count = kvad_tree_walk(tree, NULL, NULL);
    XCTAssertEqual(count, 0);
}

- (void)_testInsertFindRemoveWithMaxLevels:(unsigned int)maxLevels pointsPerNode:(unsigned int)ppn {
    kvad_tree* tree = kvad_tree_create(-100, -100, 200, 200, maxLevels, ppn);

    NSMutableArray *payloadStrings = [NSMutableArray array];
    for (int i=0;i<100;i++) {
        NSString *s = [NSString stringWithFormat:@"Point %04d",i];
        [payloadStrings addObject:s];
        kvad_tree_insert(tree, i, i, (__bridge void*)s);
    }

    NSMutableArray* result = [NSMutableArray array];
    unsigned long count = kvad_tree_find(tree, 0, 0, 50, 50, KvadArrayAppendCallback, (__bridge void *)(result));

    XCTAssertEqual(result.count, (NSUInteger)51);
    XCTAssertEqual(result.count, count);
    [result sortUsingSelector:@selector(compare:)];

    int i = 0;
    for (NSString* msg in result) {
        NSString* expected = [NSString stringWithFormat:@"Point %04d",i];
        XCTAssertTrue([msg isEqualToString:expected]);
        i++;
    }

    for (i=0;i<100;i++) {
        kvad_tree_remove_payload(tree, i,i, (__bridge void*)payloadStrings[i]);
    }

    count = kvad_tree_walk(tree, NULL, NULL);
    XCTAssertEqual(count, 0);

    for (i=0;i<100;i++) {
        // Insert all at same point
        kvad_tree_insert(tree, 0, 0, (__bridge void*)payloadStrings[i]);
    }
    [result removeAllObjects];
    count = kvad_tree_find(tree, 0,0,1,1, KvadArrayAppendCallback, (__bridge void*)(result));

    XCTAssertEqual(count, 100);
}

- (void)testBasicLookup {
    [self _testInsertFindRemoveWithMaxLevels:0 pointsPerNode:1];
    [self _testInsertFindRemoveWithMaxLevels:1 pointsPerNode:100];
    [self _testInsertFindRemoveWithMaxLevels:16 pointsPerNode:4];
}

@end
