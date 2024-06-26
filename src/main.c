#include "model.h"
#include "window.h"

#include "camera.h"
#include "cglm/struct.h"
#include "input.h"
#include "model_presets.h"
#include "rendering.h"

#define MOVE_SPEED 10.0f

GLFWwindow *window;

mat4s ViewMatrix, ProjectionMatrix;
vec3s movementInput;
vec2s mousePosition;
vec2s mouseDelta;
vec2s mouseSensitivity;

int main(void) {
    window = windowCreate();
    inputInit(window);

    Camera camera =
        cameraCreate((vec3s){{0.0f, 1.0f, -1.0f}}, GLMS_QUAT_IDENTITY);
    cameraLookAt(&camera, GLMS_VEC3_ZERO);

    Model *model1 = modelPresetTextured(
        "models/SM_Deccer_Cubes.glb", "shaders/vertex_shader.vert",
        "shaders/fragment_shader.frag", "textures/crate.jpg");
    Model *model2 = modelPresetTextured(
        "models/SM_Deccer_Cubes.glb", "shaders/vertex_shader.vert",
        "shaders/fragment_shader.frag", "textures/crate.jpg");
    model2->Transform = glms_translate(
        glms_rotate(model2->Transform, glm_rad(33), (vec3s){{0.6f, 0.8f, 0}}),
        (vec3s){{4, 8, 10}});

    ProjectionMatrix = glms_perspective(
        glm_rad(60.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f,
        100.0f);

    float lastTime = 0, currentTime = 0, deltaTime = 0;
    vec3s eulerAngles = GLMS_VEC3_ZERO;
    vec3s positionDelta = GLMS_VEC3_ZERO;
    GLenum err;
    while (!glfwWindowShouldClose(window)) {
        currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;

        inputMouseUpdate(window);
        eulerAngles = glms_vec3_add(
            eulerAngles,
            (vec3s){{-mouseDelta.y * mouseSensitivity.x * deltaTime,
                     -mouseDelta.x * mouseSensitivity.y * deltaTime, 0}});
        cameraSetEulerAngles(&camera, eulerAngles);

        positionDelta = (vec3s){{movementInput.x, 0, -movementInput.y}};
        positionDelta = glms_quat_rotatev(camera.Quaternion, positionDelta);
        positionDelta =
            glms_vec3_add(positionDelta, (vec3s){{0, -movementInput.z, 0}});
        camera.Position = glms_vec3_add(
            camera.Position,
            glms_vec3_scale(positionDelta, MOVE_SPEED * deltaTime));
        cameraCalculateViewMatrix(&camera);
        ViewMatrix = camera.ViewMatrix;

        lastTime = currentTime;

        // HINT: drawing goes here
        // glClearColor(0.12f, 0.12f, 0.18f, 1.0f);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        modelRender(model1);
        modelRender(model2);

        windowDraw(window);

        while ((err = glGetError()) != GL_NO_ERROR) {
            fprintf(stderr, "gl error 0x%04X\n", err);
        }
    }

    modelFree(model1);
    modelFree(model2);

    windowClose();
    return 0;
}
