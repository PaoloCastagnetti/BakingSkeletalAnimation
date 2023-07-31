#include <iostream>
#include <fstream>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void writeMeshToObj(const aiMesh* mesh, std::ofstream& outputFile);
aiQuaternion interpolateRotation(float animationTime, const aiNodeAnim* nodeAnim);
void applyPoseToMeshesInScene(const aiScene* scene, const aiAnimation* animation, float animationTime);

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

    // Dopo aver caricato l'animazione
    std::cout << "Numero di canali animati: " << animation->mNumChannels << std::endl;
    for (unsigned int i = 0; i < animation->mNumChannels; i++) {
        aiNodeAnim* channel = animation->mChannels[i];
        std::cout << "Canale " << i << " - Nome nodo: " << channel->mNodeName.C_Str() << std::endl;
    }

    // Seleziona il frame desiderato (es. frame 10)
    float desiredTime = 25.0f;

    // Applica la posa a tutte le mesh nella scena
    applyPoseToMeshesInScene(scene, animation, desiredTime);

    // Scrivi la mesh risultante in formato OBJ
    std::ofstream outputFile("Mesh/OutputMesh.obj");
    if (!outputFile.is_open()) {
        std::cout << "Impossibile aprire il file OutputMesh.obj per la scrittura." << std::endl;
        return -1;
    }

    // Scrivi la mesh del primo frame in formato OBJ
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

aiQuaternion interpolateRotation(float animationTime, const aiNodeAnim* nodeAnim) {
    aiQuaternion rotation;
    if (nodeAnim->mNumRotationKeys == 1) {
        rotation = nodeAnim->mRotationKeys[0].mValue;
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
        aiQuaternion::Interpolate(rotation, startRotationQ, endRotationQ, factor);
        rotation.Normalize();
    }
    return rotation;
}

void applyPoseToNode(aiNode* node, const aiMatrix4x4& parentTransform, const aiAnimation* animation, float animationTime) {
    aiMatrix4x4 nodeTransform = node->mTransformation;

    // Cerca il canale di animazione per il nodo corrente
    aiNodeAnim* nodeAnim = nullptr;
    for (unsigned int i = 0; i < animation->mNumChannels; i++) {
        aiNodeAnim* channel = animation->mChannels[i];
        if (channel->mNodeName == node->mName) {
            nodeAnim = channel;
            break;
        }
    }

    if (nodeAnim) {
        // Applica la posa al nodo utilizzando l'animazione interpolata per la rotazione
        aiQuaternion rotationQ = interpolateRotation(animationTime, nodeAnim);
        aiMatrix4x4 rotationMatrix = aiMatrix4x4(rotationQ.GetMatrix());
        nodeTransform = rotationMatrix;
    }

    // Calcola la trasformazione globale del nodo combinando la trasformazione del nodo corrente con quella del nodo padre
    aiMatrix4x4 globalTransform = parentTransform * nodeTransform;

    // Applica la trasformazione globale al nodo corrente
    node->mTransformation = globalTransform;

    // Applica ricorsivamente la posa a tutti i nodi figli
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        applyPoseToNode(node->mChildren[i], globalTransform, animation, animationTime);
    }
}

void applyPoseToMeshesInScene(const aiScene* scene, const aiAnimation* animation, float animationTime) {
    // Applica la posa a partire dalla radice dell'albero della scena
    applyPoseToNode(scene->mRootNode, aiMatrix4x4(), animation, animationTime);
}