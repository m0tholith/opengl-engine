#include "camera.h"

#include "rendering.h"
#include "window.h"

#define X_AXIS ((vec3s){{1.0f, 0.0f, 0.0f}})
#define Y_AXIS ((vec3s){{0.0f, 1.0f, 0.0f}})
#define Z_AXIS ((vec3s){{0.0f, 0.0f, 1.0f}})

Camera cameraCreate(vec3s position, versors quaternion) {
    Camera camera = (Camera){
        .Position = position,
        .Quaternion = quaternion,
    };
    cameraCalculateViewMatrix(&camera);
    return camera;
}
void cameraSetProjectionMatrixPersp(Camera *camera, float fov, float nearPlane,
                                    float farPlane) {
    ProjectionFromViewMatrix = glms_perspective(
        glm_rad(fov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, nearPlane,
        farPlane);
}
void cameraSetProjectionMatrixOrtho(Camera *camera, float x, float y,
                                    float nearPlane, float farPlane) {
#define INITIAL_SCALE 40.0f
    ProjectionFromViewMatrix = glms_ortho(-x, x, y, -y, nearPlane, farPlane);
}
void cameraCalculateViewMatrix(Camera *camera) {
    camera->ViewFromWorldMatrix =
        glms_quat_look(camera->Position, camera->Quaternion);
    ViewFromWorldMatrix = camera->ViewFromWorldMatrix;
}
void cameraSetPosition(Camera *camera, vec3s position) {
    camera->Position = position;
}
void cameraLookAt(Camera *camera, vec3s target) {
    camera->Quaternion = glms_quat_forp(camera->Position, target, Y_AXIS);
}
// creates a new quaternion by multiplying per-axis quaternions together in a
// specific order (YXZ)
void cameraSetEulerAngles(Camera *camera, vec3s eulerAngles) {
    versors quatX = glms_quatv(eulerAngles.x, X_AXIS);
    versors quatY = glms_quatv(eulerAngles.y, Y_AXIS);
    versors quatZ = glms_quatv(eulerAngles.z, Z_AXIS);
    camera->Quaternion = glms_quat_mul(glms_quat_mul(quatY, quatX), quatZ);
}
void cameraSetQuaternion(Camera *camera, versors quaternion) {
    camera->Quaternion = quaternion;
}
