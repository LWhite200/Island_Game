#include <stdlib.h>
#include <math.h>
#include "kd_tree.h"

static float get_axis_value(Vec3 v, int axis) {
    return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

static Vec3 triangle_center(const Triangle* t) {
    return (Vec3) {
        (t->v1.x + t->v2.x + t->v3.x) / 3.0f,
            (t->v1.y + t->v2.y + t->v3.y) / 3.0f,
            (t->v1.z + t->v2.z + t->v3.z) / 3.0f
    };
}

KDNode* kd_insert(KDNode* root, Triangle tri, int depth) {
    int axis = depth % 3;
    Vec3 center = triangle_center(&tri);
    float value = get_axis_value(center, axis);

    if (!root) {
        KDNode* node = (KDNode*)calloc(1, sizeof(KDNode));
        node->axis = axis;
        node->split = value;
        node->triangles[0] = tri;
        node->tri_count = 1;
        return node;
    }

    if (root->tri_count < MAX_TRIANGLES) {
        root->triangles[root->tri_count++] = tri;
        return root;
    }

    if (value < root->split)
        root->left = kd_insert(root->left, tri, depth + 1);
    else
        root->right = kd_insert(root->right, tri, depth + 1);

    return root;
}

static float point_distance_squared(Vec3 a, Vec3 b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

void kd_query_nearest(const KDNode* root, Vec3 point, float radius, void (*callback)(const Triangle*)) {
    if (!root) return;

    float r2 = radius * radius;

    // Check this node's triangles
    for (int i = 0; i < root->tri_count; i++) {
        Vec3 center = triangle_center(&root->triangles[i]);
        if (point_distance_squared(center, point) <= r2) {
            callback(&root->triangles[i]);
        }
    }

    float axis_val = get_axis_value(point, root->axis);

    if (axis_val < root->split) {
        kd_query_nearest(root->left, point, radius, callback);
        if (fabsf(axis_val - root->split) < radius)
            kd_query_nearest(root->right, point, radius, callback);
    }
    else {
        kd_query_nearest(root->right, point, radius, callback);
        if (fabsf(axis_val - root->split) < radius)
            kd_query_nearest(root->left, point, radius, callback);
    }
}

void kd_free(KDNode* root) {
    if (!root) return;
    kd_free(root->left);
    kd_free(root->right);
    free(root);
}
