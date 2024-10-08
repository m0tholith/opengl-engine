#include "model.h"

#include "assimp/matrix4x4.h"
#include "material.h"
#include "mesh.h"
#include "node.h"
#include "texture.h"

#include "glad/glad.h"
#include <GL/gl.h>
#include <assimp/postprocess.h>
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Mesh *processMesh(struct aiMesh *mesh, const struct aiScene *scene);
struct Node *processNode(struct aiNode *node, struct Node *parentNode);
void processNodeArray(struct NodeEntry *nodeArray, struct Node *rootNode,
                      int *index, int parentIndex);

Model *modelLoad(const char *_modelPath) {
    char *modelFile = malloc(strlen(_modelPath) + sizeof(MODELS_PATH));
    strcpy(modelFile, MODELS_PATH);
    strcat(modelFile, _modelPath);

    const struct aiScene *scene = aiImportFile(
        modelFile, aiProcess_Triangulate | aiProcess_FlipUVs |
                       aiProcess_GenNormals | aiProcess_SplitLargeMeshes |
                       aiProcess_PopulateArmatureData);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        printf("assimp error: %s", aiGetErrorString());
    }

    Model *model = malloc(sizeof(Model));

    model->WorldFromModel = GLMS_MAT4_IDENTITY;

    model->MeshCount = scene->mNumMeshes;
    model->Meshes = malloc(model->MeshCount * sizeof(struct Mesh *));
    for (int i = 0; i < scene->mNumMeshes; i++) {
        model->Meshes[i] = processMesh(scene->mMeshes[i], scene);
        meshSendData(model->Meshes[i]);
    }

    model->MaterialCount = scene->mNumMaterials;
    model->Materials = malloc(model->MaterialCount * sizeof(Material *));

    model->TextureCount = scene->mNumTextures;
    model->Textures = malloc(model->TextureCount * sizeof(Texture *));
    for (int i = 0; i < model->TextureCount; i++) {
        model->Textures[i] = textureCreate(scene->mTextures[i]->mFilename.data,
                                           TEXTURETYPE_RGB, true);
    }

    struct Node *rootNode = processNode(scene->mRootNode, NULL);
    model->NodeCount = nodeChildCount(rootNode) + 1; // +1 for root node
    model->NodeEntries = malloc(model->NodeCount * sizeof(struct NodeEntry));
    int index = 0;
    processNodeArray(model->NodeEntries, rootNode, &index, -1);

    model->AnimationCount = scene->mNumAnimations;
    model->Animations = malloc(model->AnimationCount * sizeof(Animation *));
    for (int i = 0; i < model->AnimationCount; i++) {
        model->Animations[i] =
            animationCreate(scene, scene->mAnimations[i]->mName.data,
                            model->NodeEntries[0].Node);
    }

    model->OnDelete = &_modelDelete;

    free(modelFile);
    aiReleaseImport(scene);

    return model;
}
void modelSetMaterials(Model *model, int materialCount, ...) {
    va_list materials;
    va_start(materials, materialCount);
    for (int i = 0; i < materialCount; i++) {
        model->Materials[i] = va_arg(materials, Material *);
    }
    va_end(materials);
}
void modelSetDefaultMaterial(Model *model, Material *material) {
    for (int i = 0; i < model->MaterialCount; i++) {
        model->Materials[i] = material;
    }
}
void modelRender(Model *model) {
    for (int i = 0; i < model->NodeCount; i++) {
        struct NodeEntry *nodeEntry = &model->NodeEntries[i];
        mat4s worldFromParent;
        if (i == 0)
            worldFromParent = model->WorldFromModel;
        else
            worldFromParent =
                model->NodeEntries[nodeEntry->ParentIndex].WorldFromLocal;

        nodeRender(worldFromParent, nodeEntry->Node, model->Meshes,
                   model->Materials);
        nodeEntry->WorldFromLocal =
            glms_mat4_mul(worldFromParent, nodeEntry->Node->ParentFromLocal);
    }
}
void modelFree(Model *model) { (model->OnDelete)(model); }
void _modelDelete(void *_model) {
    Model *model = (Model *)_model;
    nodeFree(model->NodeEntries[0].Node);
    free(model->NodeEntries);
    free(model->Materials);
    for (int i = 0; i < model->MeshCount; i++) {
        meshFree(model->Meshes[i]);
    }
    free(model->Meshes);
    for (int i = 0; i < model->TextureCount; i++) {
        textureFree(model->Textures[i]);
    }
    free(model->Textures);
    for (int i = 0; i < model->AnimationCount; i++) {
        animationFree(model->Animations[i]);
    }
    free(model->Animations);
    free(model);
}
void _modelFreeMaterials(void *_model) {
    Model *model = (Model *)_model;
    for (int i = 0; i < model->MaterialCount; i++) {
        materialFree(model->Materials[i]);
    }
}

struct Mesh *processMesh(struct aiMesh *mesh, const struct aiScene *scene) {
    struct Vertex *vertices =
        malloc(mesh->mNumVertices * sizeof(struct Vertex));
    uint32_t *indices = malloc(mesh->mNumFaces * 3 * sizeof(uint32_t));
    for (int i = 0; i < mesh->mNumVertices; i++) {
        struct Vertex v = {0};

        v.Position = (vec3s){
            {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z}};
        v.Normal = (vec3s){
            {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z}};
        if (mesh->mTextureCoords[0])
            v.TexCoords = (vec2s){
                {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}};
        else
            v.TexCoords = GLMS_VEC2_ONE;
        if (mesh->mColors[0])
            v.Color = (vec3s){{
                mesh->mColors[0][i].r,
                mesh->mColors[0][i].g,
                mesh->mColors[0][i].b,
            }};
        else
            v.Color = GLMS_VEC3_ONE;
        vertices[i] = v;
    }
    for (int i = 0; i < mesh->mNumFaces; i++) {
        struct aiFace face = mesh->mFaces[i];
        for (int j = 0; j < 3; j++) {
            indices[3 * i + j] = face.mIndices[j];
        }
    }

    struct Mesh *newMesh =
        meshLoad(vertices, indices, mesh->mNumVertices, mesh->mNumFaces * 3);
    newMesh->MaterialIndex = mesh->mMaterialIndex;
    return newMesh;
}

mat4s aiMatrixToGLMS(struct aiMatrix4x4 aiMat) {
    mat4s mat;
    mat.raw[0][0] = aiMat.a1;
    mat.raw[0][2] = aiMat.c1;
    mat.raw[0][1] = aiMat.b1;
    mat.raw[0][3] = aiMat.d1;
    mat.raw[1][0] = aiMat.a2;
    mat.raw[1][1] = aiMat.b2;
    mat.raw[1][2] = aiMat.c2;
    mat.raw[1][3] = aiMat.d2;
    mat.raw[2][0] = aiMat.a3;
    mat.raw[2][1] = aiMat.b3;
    mat.raw[2][2] = aiMat.c3;
    mat.raw[2][3] = aiMat.d3;
    mat.raw[3][0] = aiMat.a4;
    mat.raw[3][1] = aiMat.b4;
    mat.raw[3][2] = aiMat.c4;
    mat.raw[3][3] = aiMat.d4;
    return mat;
}
struct Node *processNode(struct aiNode *node, struct Node *parentNode) {
    if (parentNode == NULL) {
        printf("Root node is \'%s\'\n", node->mName.data);
    }
    struct Node *newNode = nodeCreate(parentNode, node->mNumChildren);
    newNode->ParentFromLocal = aiMatrixToGLMS(node->mTransformation);
    newNode->MeshCount = node->mNumMeshes;
    newNode->Meshes = malloc(node->mNumMeshes * sizeof(uint32_t));
    memcpy(newNode->Meshes, node->mMeshes, node->mNumMeshes * sizeof(uint32_t));
    newNode->Name = malloc(node->mName.length * sizeof(char) + 1);
    strcpy(newNode->Name, node->mName.data);
    for (int i = 0; i < node->mNumChildren; i++) {
        newNode->Children[i] = processNode(node->mChildren[i], newNode);
    }
    return newNode;
}
void processNodeArray(struct NodeEntry *nodeArray, struct Node *rootNode,
                      int *indexPtr, int parentIndex) {
    int index = (*indexPtr)++;
    nodeArray[index].Node = rootNode;
    nodeArray[index].ParentIndex = parentIndex;

    if (parentIndex == -1)
        nodeArray[index].WorldFromLocal = GLMS_MAT4_IDENTITY;
    else
        nodeArray[index].WorldFromLocal = glms_mat4_mul(
            nodeArray[parentIndex].WorldFromLocal, rootNode->ParentFromLocal);

    for (int i = 0; i < rootNode->ChildCount; i++) {
        processNodeArray(nodeArray, rootNode->Children[i], indexPtr, index);
    }
}

struct Node *searchForNode(char *name, struct Node *rootNode) {
    if (!strcmp(name, rootNode->Name))
        return rootNode;
    for (int i = 0; i < rootNode->ChildCount; i++) {
        struct Node *result = searchForNode(name, rootNode->Children[i]);
        if (result != NULL)
            return result;
    }
    return NULL;
}
