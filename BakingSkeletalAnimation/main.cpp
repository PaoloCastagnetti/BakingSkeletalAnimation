#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

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

    // La struttura di scena ha un solo nodo radice che contiene tutte le informazioni
    aiNode* rootNode = scene->mRootNode;

    // Seleziona la prima mesh trovata nella scena
    aiMesh* mesh = scene->mMeshes[0];

    // Leggi le informazioni della mesh skinnata
    // I dati sui pesi delle ossa e sugli indici delle ossa associati a ciascun vertice sono contenuti nella mesh
    aiVector3D* positions = mesh->mVertices; // Array delle posizioni dei vertici
    aiVector3D* normals = mesh->mNormals; // Array delle normali dei vertici
    aiVector3D* tangents = mesh->mTangents; // Array delle tangenti dei vertici
    aiVector3D* bitangents = mesh->mBitangents; // Array delle bitangenti dei vertici

    // Leggi i dati dello skinning della mesh
    // I dati relativi alle ossa sono contenuti nella scena e devono essere associati ai vertici corrispondenti
    aiBone** bones = mesh->mBones; // Array delle ossa associate alla mesh
    unsigned int numBones = mesh->mNumBones; // Numero di ossa associate alla mesh

    // Itera sulle ossa associate alla mesh e ottieni i dati sui pesi e sugli indici delle ossa per ciascun vertice
    for (unsigned int i = 0; i < numBones; i++) {
        aiBone* bone = bones[i];
        std::string boneName = bone->mName.C_Str();

        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            aiVertexWeight vertexWeight = bone->mWeights[j];
            unsigned int vertexIndex = vertexWeight.mVertexId;
            float weight = vertexWeight.mWeight;
            std::cout << "vertexIndex: " << vertexIndex << std::endl;
            std::cout << "weight: " << weight << std::endl;

            // Usa vertexIndex e weight per associare i dati dello skinning al vertice corrispondente
            // Ad esempio, puoi mantenere un array di indici di ossa e pesi per ciascun vertice
        }
    }

    // Fai altre operazioni con i dati della mesh, ad esempio, calcola le pose o esegui il rendering della mesh

    return 0;
}