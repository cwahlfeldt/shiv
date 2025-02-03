/**
 * @file Math.h
 * @brief Math helpers for Shiv
 */

#ifndef SHIV_MATH_H
#define SHIV_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Basic type definitions */
typedef struct Vector2 {
    float x, y;
} shiv_vec2_t;

typedef struct Vector3 {
    float x, y, z;
} shiv_vec3_t;

typedef struct Vector4 {
    float x, y, z, w;
} shiv_vec4_t;

typedef struct Matrix4x4 {
    float m[16];
} shiv_mat4_t;

#ifdef __cplusplus
}
#endif

#endif /* SHIV_MATH_H */
