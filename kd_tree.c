#include <stdlib.h>
#include <math.h>
#include "kd_tree.h"
#include <float.h>

// Helper function to get the value of a Vec3 (x, y, or z) depending on the axis (0=x, 1=y, 2=z)
static float get_axis_value(Vec3 v, int axis) {
    return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

// Helper function to compute the center point of a triangle (average of its 3 vertices)
static Vec3 triangle_center(const Triangle* t) {
    return (Vec3) {
        (t->v1.x + t->v2.x + t->v3.x) / 3.0f,
            (t->v1.y + t->v2.y + t->v3.y) / 3.0f,
            (t->v1.z + t->v2.z + t->v3.z) / 3.0f
    };
}

// Insert a triangle into the KD-tree at a given depth (depth is used to determine splitting axis)
KDNode* kd_insert(KDNode* root, Triangle tri, int depth) {
    int axis = depth % 3;  // Cycles through 0 (x), 1 (y), 2 (z)
    Vec3 center = triangle_center(&tri);  // Compute center of triangle
    float value = get_axis_value(center, axis);  // Get coordinate on the chosen axis

    if (!root) {
        // Create a new node if current root is NULL
        KDNode* node = (KDNode*)calloc(1, sizeof(KDNode));
        node->axis = axis;   // Store splitting axis
        node->split = value; // Store splitting value (used to decide left/right in future)
        node->triangles[0] = tri;  // Store triangle in this node
        node->tri_count = 1;
        return node;
    }

    if (root->tri_count < MAX_TRIANGLES) {
        // If this node has space, just store the triangle here
        root->triangles[root->tri_count++] = tri;
        return root;
    }

    // Otherwise, pass triangle to either left or right subtree based on its center's position
    if (value < root->split)
        root->left = kd_insert(root->left, tri, depth + 1);
    else
        root->right = kd_insert(root->right, tri, depth + 1);

    return root;
}

// Helper to compute squared distance between two points (avoids slow sqrt for distance)
static float point_distance_squared(Vec3 a, Vec3 b) {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return dx * dx + dy * dy + dz * dz;
}

// The distance from the query point to the splitting plane is less than the distance to your current farthest nearest neighbor.
// Query the KD-tree for all triangles within a given radius of a point
void kd_query_nearest(const KDNode* root, Vec3 point, int numTriangles, void (*callback)(const Triangle*)) {
    if (!root || numTriangles <= 0) return;

    // Structs to hold results and their distances
    const Triangle* closest[numTriangles];
    float distances[numTriangles];
    int count = 0;

    // Initialize distances with max float
    for (int i = 0; i < numTriangles; ++i) {
        distances[i] = FLT_MAX;
        closest[i] = NULL;
    }

    // Recursive traversal
    void search(const KDNode * node) {
        if (!node) return;

        // Check all triangles in this node
        for (int i = 0; i < node->tri_count; i++) {
            Vec3 center = triangle_center(&node->triangles[i]);
            float distSq = point_distance_squared(center, point);

            // Insert into sorted list if closer
            int insertIndex = -1;
            for (int j = 0; j < numTriangles; ++j) {
                if (distSq < distances[j]) {
                    insertIndex = j;
                    break;
                }
            }

            if (insertIndex != -1) {
                // Shift elements to make room
                for (int j = numTriangles - 1; j > insertIndex; --j) {
                    distances[j] = distances[j - 1];
                    closest[j] = closest[j - 1];
                }
                distances[insertIndex] = distSq;
                closest[insertIndex] = &node->triangles[i];
                if (count < numTriangles) count++;
            }
        }

        // Continue traversal
        float axisVal = get_axis_value(point, node->axis);
        float diff = axisVal - node->split;
        float planeDistSq = diff * diff;
        KDNode* first = axisVal < node->split ? node->left : node->right;
        KDNode* second = axisVal < node->split ? node->right : node->left;

        // Always search near side
        search(first);

        // Only search far side if it could contain a closer triangle
        if (planeDistSq < distances[numTriangles - 1]) {
            search(second);
        }
    }

    // Start recursive search
    search(root);

    // Return closest triangles via callback
    for (int i = 0; i < count; i++) {
        if (closest[i]) callback(closest[i]);
    }
}


// Recursively free all nodes in the KD-tree
void kd_free(KDNode* root) {
    if (!root) return;
    kd_free(root->left);   // Free left subtree
    kd_free(root->right);  // Free right subtree
    free(root);            // Free current node
}
