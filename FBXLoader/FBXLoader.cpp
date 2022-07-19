#define FBXSDK_SHARED
#define STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include <vector>

#include <fbxsdk.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "GLShader.h"
#include "Camera.hpp"
#include "Vertex.h"

using namespace std;
using namespace fbxsdk;

vector<Vertex> vertices;
string pathTexture;
GLuint VAO, VBO;
Camera cam(640, 480, glm::vec3(0.0f, 0.0f, 2.0f));
unsigned int texture;

// Shader
GLShader m_shader;

// Scene FBX
FbxManager *m_fbxManager;
FbxScene *m_scene;
FbxMatrix finalGlobalTransform;

void Initialize()
{
    GLenum error = glewInit();
    if (error != GLEW_OK)
        cout << "erreur d'initialisation de GLEW!" << endl;

    // Logs
    cout << "Version : " << glGetString(GL_VERSION) << endl;
    cout << "Vendor : " << glGetString(GL_VENDOR) << endl;
    cout << "Renderer : " << glGetString(GL_RENDERER) << endl;

    // Shader
    m_shader.LoadVertexShader("vertex.glsl");
    m_shader.LoadFragmentShader("fragment.glsl");
    m_shader.Create();
}

void InitSceneFBX()
{
    // Init SDK
    m_fbxManager = FbxManager::Create();
    FbxIOSettings *ioSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);
    m_fbxManager->SetIOSettings(ioSettings);

    // Create scene
    m_scene = FbxScene::Create(m_fbxManager, "Ma Scene");
    FbxImporter *importer = FbxImporter::Create(m_fbxManager, "");
    bool status = importer->Initialize("data/ironman.fbx", -1, m_fbxManager->GetIOSettings());
    status = importer->Import(m_scene);
    importer->Destroy();

    // On compare le repère de la scene avec le repere souhaite
    FbxAxisSystem SceneAxisSystem = m_scene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
    if (SceneAxisSystem != OurAxisSystem)
        OurAxisSystem.ConvertScene(m_scene);

    FbxSystemUnit SceneSystemUnit = m_scene->GetGlobalSettings().GetSystemUnit();

    // L'unite standard du Fbx est le centimetre, que l'on peut tester ainsi
    if (SceneSystemUnit != FbxSystemUnit::cm)
    {
        printf("[warning] SystemUnity vaut %f cm!\n", SceneSystemUnit.GetScaleFactor());
        FbxSystemUnit::cm.ConvertScene(m_scene);
    }
}

void Shutdown()
{
    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    m_shader.Destroy();
    m_scene->Destroy();
    m_fbxManager->Destroy();
}

void Display(GLFWwindow *window)
{
    int widthWindow, heightWindow;
    glfwGetWindowSize(window, &widthWindow, &heightWindow);
    glViewport(0, 0, widthWindow, heightWindow);
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    // on peut egalement modifier la fonction de test
    // glDepthFunc(GL_LEQUAL); // par ex: en inferieur ou egal
    // active la suppression des faces arrieres
    glEnable(GL_CULL_FACE);

    GLuint basicProgram = m_shader.GetProgram();
    glUseProgram(basicProgram);

    const int sampler = glGetUniformLocation(basicProgram, "u_sampler");
    glUniform1i(sampler, 0);

    // Actuel Time
    const int timeLocation = glGetUniformLocation(m_shader.GetProgram(), "u_time");
    float time = static_cast<float>(glfwGetTime());
    glUniform1f(timeLocation, time);

    //
    // MATRICE DE SCALE
    //
    const float scale[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f};

    const GLint matScaleLocation = glGetUniformLocation(m_shader.GetProgram(), "u_scale");
    glUniformMatrix4fv(matScaleLocation, 1, false, scale);

    //
    // MATRICE DE ROTATION
    //
    const float rot[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f};

    const GLint matRotLocation = glGetUniformLocation(m_shader.GetProgram(), "u_rotation");
    glUniformMatrix4fv(matRotLocation, 1, false, rot);

    //
    // MATRICE DE TRANSLATION
    //
    const float translation[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, -15.f, 1.f};
    const GLint matTranslationLocation = glGetUniformLocation(m_shader.GetProgram(), "u_translation");
    glUniformMatrix4fv(matTranslationLocation, 1, false, translation);

    //
    // MATRICE DE PROJECTION
    //
    const float aspectRatio = float(widthWindow) / float(heightWindow);
    constexpr float nearZ = 0.01f;
    constexpr float farZ = 1000.f;
    constexpr float fov = 45.f;
    constexpr float fov_rad = fov * 3.141592654f / 180.f;
    const float f = 1.f / tanf(fov_rad / 2.f);

    const float projectionPerspective[] = {
        f / aspectRatio, 0.f, 0.f, 0.f, // 1ere colonne
        0.f, f, 0.f, 0.f,
        0.f, 0.f, -(farZ + nearZ) / (farZ - nearZ), -1.f,
        0.f, 0.f, -(2.f * farZ * nearZ) / (farZ - nearZ), 0.f // 4eme colonne
    };

    auto world = finalGlobalTransform.Double44();

    const float worldMat[] = {
        static_cast<float>(world[0][0]), static_cast<float>(world[0][1]), static_cast<float>(world[0][2]), static_cast<float>(world[0][3]),
        static_cast<float>(world[1][0]), static_cast<float>(world[1][1]), static_cast<float>(world[1][2]), static_cast<float>(world[1][3]),
        static_cast<float>(world[2][0]), static_cast<float>(world[2][1]), static_cast<float>(world[2][2]), static_cast<float>(world[2][3]),
        static_cast<float>(world[3][0]), static_cast<float>(world[3][1]), static_cast<float>(world[3][2]), static_cast<float>(world[3][3])};

    const GLint worldMatlocation = glGetUniformLocation(m_shader.GetProgram(), "u_world");
    glUniformMatrix4fv(worldMatlocation, 1, false, worldMat);

    cam.Matrix(45.0f, 0.1f, 1000.0f, m_shader, "u_projection");

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

static void ErrorCallback(int error, const char *description)
{
    cout << "Error GFLW " << error << " : " << description << endl;
}

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void GetMaterial(FbxNode *node)
{
    int materialCount = node->GetMaterialCount();
    FbxSurfaceMaterial *material = node->GetMaterial(0);
    const FbxProperty property = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
    const FbxProperty factDf = material->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);

    if (property.IsValid())
    {
        FbxDouble3 color = property.Get<FbxDouble3>();
        if (factDf.IsValid())
        {
            double factor = factDf.Get<FbxDouble>();
            color[0] *= factor;
            color[1] *= factor;
            color[2] *= factor;
        }

        const int textureCount = property.GetSrcObjectCount<FbxFileTexture>();
        const FbxFileTexture *texture = property.GetSrcObject<FbxFileTexture>(0);

        if (texture)
        {
            const char *name = texture->GetName();
            const char *filename = texture->GetFileName();
            const char *relativeFilename = texture->GetRelativeFileName();
        }
    }
}

void GetWorldMat(FbxNode *node)
{
    FbxVector4 translation = node->GetGeometricTranslation(FbxNode::eSourcePivot);
    FbxVector4 rotation = node->GetGeometricRotation(FbxNode::eSourcePivot);
    FbxVector4 scaling = node->GetGeometricScaling(FbxNode::eSourcePivot);
    FbxAMatrix geometryTransform;
    geometryTransform.SetTRS(translation, rotation, scaling);
    FbxAMatrix globalTransform = node->EvaluateGlobalTransform();
    finalGlobalTransform = globalTransform * geometryTransform;
}

// Parcours du Scene Graph
static void ProcessNode(FbxNode *node, FbxNode *parent)
{
    FbxNodeAttribute *att = node->GetNodeAttribute();
    FbxNodeAttribute::EType type = att->GetAttributeType();

    switch (type)
    {
    case FbxNodeAttribute::eMesh:
        Vertex myVertex = Vertex();
        FbxMesh *mesh = node->GetMesh();
        FbxVector4 *positions = mesh->GetControlPoints();
        int nbPlygone = mesh->GetPolygonCount();

        for (int i = 0; i < nbPlygone; i++)
        {
            int size = mesh->GetPolygonSize(i);
            for (int k = 0; k < size; k++)
            {
                int vId = mesh->GetPolygonVertex(i, k);

                // Positon
                FbxVector4 position = mesh->GetControlPointAt(vId);
                myVertex.position = glm::vec3(positions[vId][0], positions[vId][1], positions[vId][2]);

                // Normal
                FbxVector4 normal;
                mesh->GetPolygonVertexNormal(i, k, normal);
                myVertex.normal = glm::vec3(normal[0], normal[1], normal[2]);

                // UV
                FbxStringList nameListUV;
                mesh->GetUVSetNames(nameListUV);
                int totalUVChannels = nameListUV.GetCount();
                for (size_t j = 0; j < totalUVChannels; j++)
                {
                    FbxVector2 uv;
                    const char *nameUV = nameListUV.GetStringAt(j);
                    bool isUnMapped;
                    bool hasUV = mesh->GetPolygonVertexUV(i, vId, nameUV, uv, isUnMapped);
                    myVertex.uv = glm::vec2(uv[0], uv[1]);
                }

                // Tangent
                FbxLayerElementTangent *meshTangents = mesh->GetElementTangent(0);
                if (meshTangents == nullptr)
                {
                    // sinon on genere des tangentes (pour le tangent space normal mapping)
                    bool test = mesh->GenerateTangentsDataForAllUVSets(true);
                    meshTangents = mesh->GetElementTangent(0);
                }

                FbxLayerElement::EMappingMode tangentMode = meshTangents->GetMappingMode();
                FbxLayerElement::EReferenceMode tangentRefMode = meshTangents->GetReferenceMode();
                FbxVector4 tangent;

                if (tangentRefMode == FbxLayerElement::eDirect)
                    tangent = meshTangents->GetDirectArray().GetAt(vId);
                else if (tangentRefMode == FbxLayerElement::eIndexToDirect)
                {
                    int indirectIndex = meshTangents->GetIndexArray().GetAt(vId);
                    tangent = meshTangents->GetDirectArray().GetAt(indirectIndex);
                }

                myVertex.tangent = glm::vec4(tangent[0], tangent[1], tangent[2], tangent[3]);

                vertices.emplace_back(myVertex);
            }
        }
        break;
    }

    int childCount = node->GetChildCount();
    for (int i = 0; i < childCount; ++i)
    {
        FbxNode *child = node->GetChild(i);
        ProcessNode(child, node);
    }
}

int main()
{
    GLFWwindow *window;

    glfwSetErrorCallback(ErrorCallback);

    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(640, 480, "FBX Loader", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, KeyCallback);

    Initialize();
    InitSceneFBX();
    FbxNode *root_node = m_scene->GetRootNode();
    FbxNode *model = root_node->GetChild(0);
    ProcessNode(model, root_node);
    GetWorldMat(model);
    GetMaterial(model);

    // TEXTURE
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // charge et génère la texture
    int width, height, nrChannels;
    auto data = stbi_load("data/ironman.fbm/ironman.dff.png", &width, &height, &nrChannels, 0);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    stbi_image_free(data);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    constexpr int STRIDE = sizeof(Vertex);
    glBufferData(GL_ARRAY_BUFFER, STRIDE * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    const int POSITION = glGetAttribLocation(m_shader.GetProgram(), "a_position");
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, position));

    const int NORMAL = glGetAttribLocation(m_shader.GetProgram(), "a_normal");
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, normal));

    const int UV = glGetAttribLocation(m_shader.GetProgram(), "a_uv");
    glEnableVertexAttribArray(UV);
    glVertexAttribPointer(UV, 2, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, uv));

    const int TANGENT = glGetAttribLocation(m_shader.GetProgram(), "a_tangent");
    glEnableVertexAttribArray(TANGENT);
    glVertexAttribPointer(TANGENT, 2, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, tangent));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    while (!glfwWindowShouldClose(window))
    {
        cam.Inputs(window);
        Display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    Shutdown();
    glfwTerminate();
    return 0;
}