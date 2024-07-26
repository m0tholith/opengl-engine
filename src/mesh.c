#include "mesh.h"

#include "glad/glad.h"
#include "rendering.h"
#include <cglm/struct/mat4.h>
#include <stdlib.h>

Mesh *meshLoad(Vertex *vertices, unsigned int *indices, int vertexCount,
               int indexCount) {
    Mesh *mesh = malloc(sizeof(Mesh));
    mesh->Vertices = vertices;
    mesh->VertexCount = vertexCount;
    mesh->Indices = indices;
    mesh->IndexCount = indexCount;
    mesh->Transform = GLMS_MAT4_IDENTITY;

    return mesh;
}
void meshSendData(Mesh *mesh) {
    // generate vertex array object which contains information about all the
    // vertices
    glGenVertexArrays(1, &mesh->VAO);
    glBindVertexArray(mesh->VAO);

    // generate vertex buffer object which is the actual vertex data
    glGenBuffers(1, &mesh->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh->VertexCount * sizeof(Vertex),
                 mesh->Vertices, GL_STATIC_DRAW);

    int attribIdx = 0;
    // position vertex attribute
    glVertexAttribPointer(attribIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Position));
    glEnableVertexAttribArray(attribIdx++);
    // normals
    glVertexAttribPointer(attribIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(attribIdx++);
    // texcoords
    glVertexAttribPointer(attribIdx, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(attribIdx++);
    // vertex colors
    glVertexAttribPointer(attribIdx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Color));
    glEnableVertexAttribArray(attribIdx++);
    // bone IDs
    glVertexAttribIPointer(attribIdx, MAX_BONE_INFLUENCE, GL_INT,
                           sizeof(Vertex), (void *)offsetof(Vertex, BoneIDs));
    glEnableVertexAttribArray(attribIdx++);
    // bone weights
    glVertexAttribPointer(attribIdx, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void *)offsetof(Vertex, Weights));
    glEnableVertexAttribArray(attribIdx++);

    // generate element buffer object which contains information about the order
    // of vertices to draw
    glGenBuffers(1, &mesh->EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 mesh->IndexCount * sizeof(unsigned int), mesh->Indices,
                 GL_STATIC_DRAW);
}
void meshRender(Mesh *mesh, mat4s transformation, unsigned int shader) {
    glUseProgram(shader);

    // assign projectionFromModel matrix
    mat4s projectionFromModel = glms_mat4_mul(
        ProjectionMatrix,
        glms_mat4_mul(ViewMatrix,
                      glms_mat4_mul(transformation, mesh->Transform)));
    // `projectionFromModel` should probably always be defined and used in the
    // vertex shader
    glUniformMatrix4fv(glGetUniformLocation(shader, "projectionFromModel"), 1,
                       GL_FALSE, projectionFromModel.raw[0]);

    // render triangles
    glBindVertexArray(mesh->VAO);
    glDrawElements(GL_TRIANGLES, mesh->IndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
void meshFree(Mesh *mesh) {
    free(mesh->Vertices);
    free(mesh->Indices);
    free(mesh);
}
