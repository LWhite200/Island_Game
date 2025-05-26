#ifndef KD_TREE_H
#define KD_TREE_H

#define MAX_TRIANGLES 3

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    Vec3 v1, v2, v3;
} Triangle;

typedef struct KDNode {
    Triangle triangles[MAX_TRIANGLES];
    int tri_count;
    
    /*
        Axis determines which dimension of the 3d space is used to split the space at 'this' node
        so the x,y, or z. This is paired with the split (the coordinate in said dimension) so we 
        can further split the space into smakker regions to eliminate large parts of the search space.
    */
    int axis; // 0 = x, 1 = y, 2 = z
    float split;

    struct KDNode* left;
    struct KDNode* right;
} KDNode;

KDNode* kd_insert(KDNode* root, Triangle tri, int depth);
void kd_query_nearest(const KDNode* root, Vec3 point, float radius, void (*callback)(const Triangle*));

void kd_free(KDNode* root);

#endif
