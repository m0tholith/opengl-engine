#include "model.h"

#include "armature.h"
#include "assimp/postprocess.h"
#include "material.h"
#include "mesh.h"
#include "node.h"
#include "texture.h"
#include <glad/glad.h>

#include <GL/gl.h>
#include <cglm/struct/mat4.h>
#include <cglm/struct/vec2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Mesh *processMesh(struct aiMesh *mesh, const struct aiScene *scene);
Node *processNode(struct aiNode *node, Node *parentNode);
Armature *processSkeleton(struct aiScene *scene, Mesh **meshes, Node *rootNode);

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

    model->Transform = GLMS_MAT4_IDENTITY;

    model->MeshCount = scene->mNumMeshes;
    model->Meshes = malloc(model->MeshCount * sizeof(Mesh *));
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        model->Meshes[i] = processMesh(scene->mMeshes[i], scene);
    }

    model->MaterialCount = scene->mNumMaterials;
    model->Materials = malloc(model->MaterialCount * sizeof(Material *));

    model->TextureCount = scene->mNumTextures;
    model->Textures = malloc(model->TextureCount * sizeof(unsigned int));
    for (int i = 0; i < model->TextureCount; i++) {
        model->Textures[i] = textureCreate(scene->mTextures[i]->mFilename.data,
                                           TEXTURETYPE_RGB, true);
    }

    model->RootNode = processNode(scene->mRootNode, NULL);
    model->RootNode->Transform = GLMS_MAT4_IDENTITY;

    model->Skeleton = processSkeleton(scene, model->Meshes, model->RootNode);
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        meshSendData(model->Meshes[i]);
    }

    model->AnimationCount = scene->mNumAnimations;
    model->Animations = malloc(model->AnimationCount * sizeof(Animation *));
    for (int i = 0; i < model->AnimationCount; i++) {
        model->Animations[i] = animationCreate(
            scene, scene->mAnimations[i]->mName.data, model->RootNode);
    }

    model->OnDelete = &_modelDelete;

    free(modelFile);
    aiReleaseImport(scene);

    return model;
}
void modelSetMaterials(Model *model, ...) {
    va_list materials;
    va_start(materials, model->MaterialCount);
    for (int i = 0; i < model->MaterialCount; i++) {
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
    armatureSetBoneMatrices(model->Skeleton);
    for (int i = 0; i < model->MaterialCount; i++) {
        glUniformMatrix4fv(
            glGetUniformLocation(model->Materials[i]->Shader, "boneFromRoot"),
            MAX_BONES, GL_FALSE, (GLfloat *)model->Skeleton->BoneMatrices);
    }
    nodeRender(model->Transform, model->RootNode, model->Meshes,
               model->Materials);
}
void modelFree(Model *model) { (model->OnDelete)(model); }
void _modelDelete(void *_model) {
    Model *model = (Model *)_model;
    nodeFree(model->RootNode);
    free(model->Materials);
    for (int i = 0; i < model->MeshCount; i++) {
        meshFree(model->Meshes[i]);
    }
    free(model->Meshes);
    free(model->Textures);
    for (int i = 0; i < model->AnimationCount; i++) {
        animationFree(model->Animations[i]);
    }
    free(model->Animations);
    armatureFree(model->Skeleton);
    free(model);
}
void _modelFreeMaterials(void *_model) {
    Model *model = (Model *)_model;
    for (int i = 0; i < model->MaterialCount - 1; i++) {
        materialFree(model->Materials[i]);
    }
}

Mesh *processMesh(struct aiMesh *mesh, const struct aiScene *scene) {
    Vertex *vertices = malloc(mesh->mNumVertices * sizeof(Vertex));
    unsigned int *indices = malloc(mesh->mNumFaces * 3 * sizeof(unsigned int));
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v = {0};

        v.Position = (vec3s){
            {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z}};
        v.Normal = (vec3s){
            {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z}};
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
        for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
            v.BoneIDs[j] = -1;
            v.Weights[j] = 0;
        }
        vertices[i] = v;
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        struct aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < 3; j++) {
            indices[3 * i + j] = face.mIndices[j];
        }
    }

    Mesh *newMesh =
        meshLoad(vertices, indices, mesh->mNumVertices, mesh->mNumFaces * 3);
    newMesh->MaterialIndex = mesh->mMaterialIndex;
    return newMesh;
}

mat4s aiMatrixToGLMS(struct aiMatrix4x4 aiMat) {
    aiTransposeMatrix4(&aiMat);
    return *(mat4s *)&aiMat;
}
Node *processNode(struct aiNode *node, Node *parentNode) {
    Node *newNode = nodeCreate(parentNode, node->mNumChildren);
    newNode->Transform = aiMatrixToGLMS(node->mTransformation);
    newNode->MeshCount = node->mNumMeshes;
    newNode->Meshes = malloc(node->mNumMeshes * sizeof(unsigned int));
    memcpy(newNode->Meshes, node->mMeshes,
           node->mNumMeshes * sizeof(unsigned int));
    newNode->Name = malloc(node->mName.length * sizeof(char) + 1);
    strcpy(newNode->Name, node->mName.data);
    for (int i = 0; i < node->mNumChildren; i++) {
        newNode->Children[i] = processNode(node->mChildren[i], newNode);
    }
    return newNode;
}

Node *searchForNode(char *name, Node *rootNode) {
    if (!strcmp(name, rootNode->Name))
        return rootNode;
    for (int i = 0; i < rootNode->ChildCount; i++) {
        Node *result = searchForNode(name, rootNode->Children[i]);
        if (result != NULL)
            return result;
    }
    return NULL;
}
Armature *processSkeleton(struct aiScene *scene, Mesh **meshes,
                          Node *rootNode) {
    Armature *skeleton = armatureCreate();
    int boneCount = 0;
    for (int i = 0; i < scene->mNumMeshes; i++) {
        struct aiMesh *assimpMesh = scene->mMeshes[i];
        Mesh *mesh = meshes[i];
        for (int j = 0; j < assimpMesh->mNumBones; j++) {
            struct aiBone *assimpBone = assimpMesh->mBones[j];
            skeleton->Bones[boneCount] =
                searchForNode(assimpBone->mNode->mName.data, rootNode);
            if (skeleton->Bones[boneCount] == NULL) {
                boneCount++;
                continue;
            }
            for (int k = 0; k < assimpBone->mNumWeights; k++) {
                Vertex *vertex =
                    &mesh->Vertices[assimpBone->mWeights[k].mVertexId];
                int boneIndex = 0;
                while (boneIndex < MAX_BONE_INFLUENCE &&
                       vertex->BoneIDs[boneIndex] != -1)
                    boneIndex++;
                if (boneIndex == MAX_BONE_INFLUENCE)
                    continue;
                vertex->BoneIDs[boneIndex] = boneCount;
                vertex->Weights[boneIndex] = assimpBone->mWeights[k].mWeight;
            }
            boneCount++;
        }
    }
    printf("boneCount = %d\n", boneCount);
    armatureSetBoneMatrices(skeleton);
    return skeleton;
}
