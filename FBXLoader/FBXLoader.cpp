#define FBXSDK_SHARED

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <vector>

#include <fbxsdk.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "Vertex.h"
#include "GLShader.h"

using namespace std;
using namespace fbxsdk;

vector<Vertex> m_vertices;

GLuint m_VAO;
GLuint m_VBO;

// Texture
GLuint m_textureId;
int m_textureW, m_textureH;
string pathText;

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
    FbxIOSettings* ioSettings = FbxIOSettings::Create(m_fbxManager, IOSROOT);
    m_fbxManager->SetIOSettings(ioSettings);

    // Create scene
    m_scene = FbxScene::Create(m_fbxManager, "Ma Scene");
    FbxImporter* importer = FbxImporter::Create(m_fbxManager, "");
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
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
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
    glClear(GL_COLOR_BUFFER_BIT);

    GLuint basicProgram = m_shader.GetProgram();
    glUseProgram(basicProgram);

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
        0.f, 0.f, 0.f, 1.f
    };

    const GLint matScaleLocation = glGetUniformLocation(m_shader.GetProgram(), "u_scale");
    glUniformMatrix4fv(matScaleLocation, 1, false, scale);

    //
    // MATRICE DE ROTATION
    //
    const float rot[] = {
        1.f, 0.f, 0.f, 0.f, // 1ere colonne
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f // 4eme colonne
    };

    const float rotY[] = {
        cosf(time), 0.f, -sinf(time), 0.f, // 1ere colonne
        0.f, 1.f, 0.f, 0.f,
        sinf(time), 0.f, cosf(time), 0.f,
        0.f, 0.f, 0.f, 1.f // 4eme colonne
    };

    const GLint matRotLocation = glGetUniformLocation(m_shader.GetProgram(), "u_rotation");
    glUniformMatrix4fv(matRotLocation, 1, false, rot);

    //
    // MATRICE DE TRANSLATION
    //
    const float translation[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, -15.f, 1.f
    };
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
    const GLint matProjectionLocation = glGetUniformLocation(m_shader.GetProgram(), "u_projection");
    glUniformMatrix4fv(matProjectionLocation, 1, false, projectionPerspective);

    auto world = finalGlobalTransform.Double44();

    const float worldMat[] = {
    static_cast<float>(world[0][0]), static_cast<float>(world[0][1]),static_cast<float>(world[0][2]), static_cast<float>(world[0][3]),
    static_cast<float>(world[1][0]), static_cast<float>(world[1][1]),static_cast<float>(world[1][2]), static_cast<float>(world[1][3]),
    static_cast<float>(world[2][0]), static_cast<float>(world[2][1]),static_cast<float>(world[2][2]), static_cast<float>(world[2][3]),
    static_cast<float>(world[3][0]), static_cast<float>(world[3][1]),static_cast<float>(world[3][2]), static_cast<float>(world[3][3])
    };

    const GLint worldMatlocation = glGetUniformLocation(m_shader.GetProgram(), "u_world");
    glUniformMatrix4fv(worldMatlocation, 1, false, worldMat);

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
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

void LoadTexture()
{
    uint8_t* data = stbi_load(pathText.c_str(), &m_textureW, &m_textureH, nullptr, STBI_rgb_alpha);

    glGenTextures(1, &m_textureId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_textureW, m_textureH, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
}

void GetMaterial(FbxNode *node)
{
    int materialCount = node->GetMaterialCount();
    FbxSurfaceMaterial* material = node->GetMaterial(0);
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
        const FbxFileTexture* texture = property.GetSrcObject<FbxFileTexture>(0);

        if (texture)
        {
            const char* name = texture->GetName();
            const char* filename = texture->GetFileName();
            const char* relativeFilename = texture->GetRelativeFileName();
            pathText = relativeFilename;
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
        // on recupère les donnée du mesh
        Vertex myVertex = Vertex();
        FbxMesh *mesh = node->GetMesh();
        FbxVector4 *positions = mesh->GetControlPoints();
        int nbPlygone = mesh->GetPolygonCount();
        int nbVertex = mesh->GetPolygonVertexCount();
        int *vertex = mesh->GetPolygonVertices();

        for (int i = 0; i < nbPlygone; ++i)
        {
            int size = mesh->GetPolygonSize(i);
            for (int k = 0; k < size; ++k)
            {
                int vertexId = mesh->GetPolygonVertex(i, k);
                FbxVector4 currentControlPoint = mesh->GetControlPointAt(vertexId);
                myVertex.position.x = currentControlPoint[0];
                myVertex.position.y = currentControlPoint[1];
                myVertex.position.z = currentControlPoint[2];

                FbxVector4 normal;
                mesh->GetPolygonVertexNormal(i, vertexId, normal);
                myVertex.normal.x = normal[0];
                myVertex.normal.y = normal[1];
                myVertex.normal.z = normal[2];

                FbxStringList nameListUV;
                mesh->GetUVSetNames(nameListUV);
                int totalUVChannels = nameListUV.GetCount();
                for (size_t j = 0; j < totalUVChannels; ++j)
                {
                    const char *nameUV = nameListUV.GetStringAt(j);
                    FbxVector2 uv;
                    bool isUnMapped;
                    bool hasUV = mesh->GetPolygonVertexUV(i, vertexId, nameUV, uv, isUnMapped);
                    myVertex.texcoords.x = uv[0];
                    myVertex.texcoords.y = uv[1];
                }
                m_vertices.push_back(myVertex);

                FbxLayerElementTangent* meshTangents = mesh->GetElementTangent(0);
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
                    tangent = meshTangents->GetDirectArray().GetAt(vertexId);
                else if (tangentRefMode == FbxLayerElement::eIndexToDirect) 
                {
                    int indirectIndex = meshTangents->GetIndexArray().GetAt(vertexId);
                    tangent = meshTangents->GetDirectArray().GetAt(indirectIndex);
                }

                myVertex.tangent.x = tangent[0];
                myVertex.tangent.y = tangent[1];
                myVertex.tangent.z = tangent[2];
                myVertex.tangent.w = tangent[3];
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
    GLFWwindow* window;

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
    FbxNode* root_node = m_scene->GetRootNode();
    FbxNode* model = root_node->GetChild(0);
    ProcessNode(model, root_node);
    GetWorldMat(model);
    GetMaterial(model);
    LoadTexture();

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    constexpr int STRIDE = sizeof(Vertex);
    glBufferData(GL_ARRAY_BUFFER, STRIDE * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    const int POSITION = glGetAttribLocation(m_shader.GetProgram(), "a_position");
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, false, STRIDE, (void*)offsetof(Vertex, position));

    const int NORMAL = glGetAttribLocation(m_shader.GetProgram(), "a_normal");
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, false, STRIDE, (void*)offsetof(Vertex, normal));

    const int UV = glGetAttribLocation(m_shader.GetProgram(), "a_texcoords");
    glEnableVertexAttribArray(UV);
    glVertexAttribPointer(UV, 2, GL_FLOAT, false, STRIDE, (void*)offsetof(Vertex, texcoords));

    const int TANGENT = glGetAttribLocation(m_shader.GetProgram(), "a_tangent");
    glEnableVertexAttribArray(TANGENT);
    glVertexAttribPointer(TANGENT, 2, GL_FLOAT, false, STRIDE, (void*)offsetof(Vertex, tangent));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    while (!glfwWindowShouldClose(window))
    {
        Display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Shutdown();
    glfwTerminate();
    return 0;
}