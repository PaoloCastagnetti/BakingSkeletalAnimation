#include <iostream>
#include <fstream>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void writeMeshToObj(const aiMesh* mesh, std::ofstream& outputFile);
aiMatrix4x4 interpolateTransformation(float animationTime, const aiNodeAnim* nodeAnim);
void applyPoseToMesh(const aiMesh* mesh, const aiAnimation* animation, float animationTime);

int main() {
    // Inizializza l'importer di Assimp
    Assimp::Importer importer;

    // Specifica le opzioni di importazione, in questo caso, vogliamo caricare i dati relativi alle ossa
    unsigned int importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_LimitBoneWeights;
    const aiScene* scene = importer.ReadFile("Mesh/AnimatedSkeletalMesh.fbx", importFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "Errore durante il caricamento della mesh skinnata: " << importer.GetErrorString() << std::endl;
        return -1;
    }

    // Seleziona la prima animazione dalla scena
    aiAnimation* animation = scene->mAnimations[0];

    // Seleziona il frame desiderato (es. frame 10)
    float desiredTime = 10.0f;

    // Applica la posa a tutte le mesh nella scena
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        applyPoseToMesh(scene->mMeshes[i], animation, desiredTime);
    }

    // Scrivi la mesh risultante in formato OBJ
    std::ofstream outputFile("Mesh/OutputMesh.obj");
    if (!outputFile.is_open()) {
        std::cout << "Impossibile aprire il file OutputMesh.obj per la scrittura." << std::endl;
        return -1;
    }

    // Scrivi tutte le mesh in formato OBJ
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        writeMeshToObj(scene->mMeshes[i], outputFile);
    }

    // Chiudi il file
    outputFile.close();

    return 0;
}

void writeMeshToObj(const aiMesh* mesh, std::ofstream& outputFile) {
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D vertex = mesh->mVertices[i];
        outputFile << "v " << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        if (face.mNumIndices != 3) {
            std::cout << "Il formato OBJ supporta solo triangoli. La mesh contiene facce con " << face.mNumIndices << " vertici." << std::endl;
            return;
        }
        outputFile << "f ";
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            outputFile << face.mIndices[j] + 1 << " "; // Gli indici OBJ partono da 1
        }
        outputFile << std::endl;
    }
}

aiMatrix4x4 interpolateTransformation(float animationTime, const aiNodeAnim* nodeAnim) {
    aiMatrix4x4 transformation;

    // Interpolazione della traslazione
    if (nodeAnim->mNumPositionKeys == 1) {
        transformation = aiMatrix4x4(); // Inizializza come matrice identità
        transformation.a4 = nodeAnim->mPositionKeys[0].mValue.x;
        transformation.b4 = nodeAnim->mPositionKeys[0].mValue.y;
        transformation.c4 = nodeAnim->mPositionKeys[0].mValue.z;
    }
    else {
        unsigned int frameIndex = 0;
        for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys - 1; i++) {
            if (animationTime < nodeAnim->mPositionKeys[i + 1].mTime) {
                frameIndex = i;
                break;
            }
        }
        unsigned int nextFrameIndex = (frameIndex + 1) % nodeAnim->mNumPositionKeys;
        float deltaTime = (float)(nodeAnim->mPositionKeys[nextFrameIndex].mTime - nodeAnim->mPositionKeys[frameIndex].mTime);
        float factor = (animationTime - (float)nodeAnim->mPositionKeys[frameIndex].mTime) / deltaTime;
        const aiVector3D& startPosition = nodeAnim->mPositionKeys[frameIndex].mValue;
        const aiVector3D& endPosition = nodeAnim->mPositionKeys[nextFrameIndex].mValue;
        aiVector3D interpolatedPosition = startPosition + factor * (endPosition - startPosition);

        transformation = aiMatrix4x4(); // Inizializza come matrice identità
        transformation.a4 = interpolatedPosition.x;
        transformation.b4 = interpolatedPosition.y;
        transformation.c4 = interpolatedPosition.z;
    }

    // Interpolazione della rotazione
    if (nodeAnim->mNumRotationKeys == 1) {
        aiQuaternion rotationQ = nodeAnim->mRotationKeys[0].mValue;
        aiMatrix4x4 rotationMatrix = aiMatrix4x4(rotationQ.GetMatrix());
        transformation *= rotationMatrix;
    }
    else {
        unsigned int frameIndex = 0;
        for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys - 1; i++) {
            if (animationTime < nodeAnim->mRotationKeys[i + 1].mTime) {
                frameIndex = i;
                break;
            }
        }
        unsigned int nextFrameIndex = (frameIndex + 1) % nodeAnim->mNumRotationKeys;
        float deltaTime = (float)(nodeAnim->mRotationKeys[nextFrameIndex].mTime - nodeAnim->mRotationKeys[frameIndex].mTime);
        float factor = (animationTime - (float)nodeAnim->mRotationKeys[frameIndex].mTime) / deltaTime;
        const aiQuaternion& startRotationQ = nodeAnim->mRotationKeys[frameIndex].mValue;
        const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[nextFrameIndex].mValue;
        aiQuaternion interpolatedRotationQ;
        aiQuaternion::Interpolate(interpolatedRotationQ, startRotationQ, endRotationQ, factor);
        interpolatedRotationQ.Normalize();
        aiMatrix4x4 rotationMatrix = aiMatrix4x4(interpolatedRotationQ.GetMatrix());
        transformation *= rotationMatrix;
    }

    // Interpolazione dello scaling
    if (nodeAnim->mNumScalingKeys == 1) {
        aiVector3D scale = nodeAnim->mScalingKeys[0].mValue;
        aiMatrix4x4 scalingMatrix;
        scalingMatrix.a1 = scale.x; scalingMatrix.a2 = 0.0f; scalingMatrix.a3 = 0.0f; scalingMatrix.a4 = 0.0f;
        scalingMatrix.b1 = 0.0f; scalingMatrix.b2 = scale.y; scalingMatrix.b3 = 0.0f; scalingMatrix.b4 = 0.0f;
        scalingMatrix.c1 = 0.0f; scalingMatrix.c2 = 0.0f; scalingMatrix.c3 = scale.z; scalingMatrix.c4 = 0.0f;
        scalingMatrix.d1 = 0.0f; scalingMatrix.d2 = 0.0f; scalingMatrix.d3 = 0.0f; scalingMatrix.d4 = 1.0f;
        transformation *= scalingMatrix;
    }
    else {
        unsigned int frameIndex = 0;
        for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys - 1; i++) {
            if (animationTime < nodeAnim->mScalingKeys[i + 1].mTime) {
                frameIndex = i;
                break;
            }
        }
        unsigned int nextFrameIndex = (frameIndex + 1) % nodeAnim->mNumScalingKeys;
        float deltaTime = (float)(nodeAnim->mScalingKeys[nextFrameIndex].mTime - nodeAnim->mScalingKeys[frameIndex].mTime);
        float factor = (animationTime - (float)nodeAnim->mScalingKeys[frameIndex].mTime) / deltaTime;
        const aiVector3D& startScaling = nodeAnim->mScalingKeys[frameIndex].mValue;
        const aiVector3D& endScaling = nodeAnim->mScalingKeys[nextFrameIndex].mValue;
        aiVector3D interpolatedScaling = startScaling + factor * (endScaling - startScaling);
        aiMatrix4x4 scalingMatrix;
        scalingMatrix.a1 = interpolatedScaling.x; scalingMatrix.a2 = 0.0f; scalingMatrix.a3 = 0.0f; scalingMatrix.a4 = 0.0f;
        scalingMatrix.b1 = 0.0f; scalingMatrix.b2 = interpolatedScaling.y; scalingMatrix.b3 = 0.0f; scalingMatrix.b4 = 0.0f;
        scalingMatrix.c1 = 0.0f; scalingMatrix.c2 = 0.0f; scalingMatrix.c3 = interpolatedScaling.z; scalingMatrix.c4 = 0.0f;
        scalingMatrix.d1 = 0.0f; scalingMatrix.d2 = 0.0f; scalingMatrix.d3 = 0.0f; scalingMatrix.d4 = 1.0f;
        transformation *= scalingMatrix;
    }

    return transformation;
}

void applyPoseToMesh(const aiMesh* mesh, const aiAnimation* animation, float animationTime) {
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D vertex = mesh->mVertices[i];
        aiVector3D normal = mesh->mNormals ? mesh->mNormals[i] : aiVector3D(0.0f, 0.0f, 0.0f);

        // Applica la posa al vertice e alla normale utilizzando le trasformazioni degli ossi associati al vertice
        aiVector3D transformedVertex(0.0f, 0.0f, 0.0f);
        aiVector3D transformedNormal(0.0f, 0.0f, 0.0f);

        // Trasformazione totale dell'ossatura per il vertice corrente
        aiMatrix4x4 totalTransformation;

        // Cicla tra tutti gli ossi associati al vertice
        for (unsigned int j = 0; j < mesh->mNumBones; j++) {
            aiBone* bone = mesh->mBones[j];
            aiMatrix4x4 boneMatrix = bone->mOffsetMatrix;

            // Trova il canale di animazione corrispondente al nome dell'osso
            aiNodeAnim* nodeAnim = nullptr;
            for (unsigned int k = 0; k < animation->mNumChannels; k++) {
                aiNodeAnim* channel = animation->mChannels[k];
                if (channel->mNodeName == bone->mName) {
                    nodeAnim = channel;
                    break;
                }
            }

            if (nodeAnim) {
                // Calcola la trasformazione finale dell'osso interpolando le trasformazioni di rotazione, scaling e translation
                aiMatrix4x4 boneTransformation = interpolateTransformation(animationTime, nodeAnim);

                // Moltiplica la trasformazione dell'osso con la matrice di offset
                boneTransformation *= boneMatrix;

                // Aggiungi la trasformazione dell'osso alla trasformazione totale usando l'operatore di moltiplicazione
                totalTransformation = boneTransformation * totalTransformation;
            }
        }

        // Applica la trasformazione totale al vertice e alla normale
        transformedVertex = totalTransformation * vertex;
        transformedNormal = totalTransformation * normal;

        // Assegna il vertice trasformato alla mesh
        mesh->mVertices[i] = transformedVertex;
        if (mesh->mNormals) {
            mesh->mNormals[i] = transformedNormal.Normalize();
        }
    }
}