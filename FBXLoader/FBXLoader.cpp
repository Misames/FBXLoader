#define FBXSDK_SHARED

#include <iostream>
#include <vector>

#include <fbxsdk.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "OpenGLcore.hpp"
#include "GLShader.hpp"
#include "FrameBuffer.hpp"
#include "Vertex.hpp"
#include "Texture.hpp"
#include "Material.hpp"
#include "Camera.hpp"

using namespace std;
using namespace fbxsdk;

vector<Vertex> vertices;
string pathTexture;
uint32_t MatricesUBO, MaterialUBO;
uint32_t ProjectorUBO, ProjectorTexture;
Camera cam(640, 480, glm::vec3(0.0f, 0.0f, 2.0f));
GLuint texture;

// FrameBuffer
Framebuffer offscreenBuffer;
// dimensions du back buffer / Fenetre
int32_t width, height;

// Shader
GLShader shader;

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

    Texture::SetupManager();

    // Shader
    shader.LoadVertexShader("vertex.glsl");
    shader.LoadFragmentShader("fragment.glsl");
    shader.Create();

    GLuint basicProgram = shader.GetProgram();
    glUseProgram(basicProgram);

    const int POSITION = glGetAttribLocation(basicProgram, "a_position");
    const int NORMAL = glGetAttribLocation(basicProgram, "a_normal");
    const int UV = glGetAttribLocation(basicProgram, "a_uv");
    const int TANGENT = glGetAttribLocation(basicProgram, "a_tangent");

    const int MatricesBindingIndex = 0;
    const int MaterialBindingIndex = 1;
    const int ProjectorBindingIndex = 2;

    glGenBuffers(1, &MaterialUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, MaterialUBO);
    // Le materiau va etre mis a jour plusieurs fois par frame -> DYNAMIC
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Material), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, MaterialBindingIndex, MaterialUBO);
    // malheureusement en OpenGL3 il faut faire les appels suivants pour chaque shader program ...
    int materialBlockIndex = glGetUniformBlockIndex(basicProgram, "Materials");
    glUniformBlockBinding(basicProgram, materialBlockIndex, MaterialBindingIndex);

    glGenBuffers(1, &MatricesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, MatricesUBO);
    // Les matrices vont etre mis a jour une fois par frame -> STREAM
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, MatricesBindingIndex, MatricesUBO);
    // malheureusement en OpenGL3 il faut faire les appels suivants pour chaque shader program ...
    int matricesBlockIndex = glGetUniformBlockIndex(basicProgram, "Matrices");
    glUniformBlockBinding(basicProgram, matricesBlockIndex, MatricesBindingIndex);

    glGenBuffers(1, &ProjectorUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, ProjectorUBO);
    // Les matrices vont etre mis a jour une fois par frame -> STREAM
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), nullptr, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, ProjectorBindingIndex, ProjectorUBO);
    int projectorBlockIndex = glGetUniformBlockIndex(basicProgram, "ProjectorMatrices");
    glUniformBlockBinding(basicProgram, projectorBlockIndex, ProjectorBindingIndex);

    ProjectorTexture = Texture::LoadTexture("data/ironman.fbm/ironman.dff.png");
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    constexpr int STRIDE = sizeof(Vertex);
    glBufferData(GL_ARRAY_BUFFER, STRIDE * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(POSITION, 3, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, position));
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, normal));
    glVertexAttribPointer(UV, 2, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, uv));
    glVertexAttribPointer(TANGENT, 2, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(POSITION);
    glEnableVertexAttribArray(NORMAL);
    glEnableVertexAttribArray(UV);
    glEnableVertexAttribArray(TANGENT);

    glEnable(GL_FRAMEBUFFER_SRGB);
    offscreenBuffer.CreateFramebuffer(width, height, true);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
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
    glUseProgram(0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &ProjectorUBO);
    glDeleteBuffers(1, &MaterialUBO);
    glDeleteBuffers(1, &MatricesUBO);
    Texture::PurgeTextures();
    shader.Destroy();
    m_scene->Destroy();
    m_fbxManager->Destroy();
}

void RenderOffscreen()
{
    offscreenBuffer.EnableRender();
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    uint32_t program = shader.GetProgram();
    glUseProgram(program);

    // Cam / projection / vue
    cam.MatrixOffScreen(45.0f, 0.1f, 1000.0f, ProjectorUBO);

    // Texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ProjectorTexture);
    int projectorLocation = glGetUniformLocation(program, "u_ProjectorTexture");
    glUniform1i(projectorLocation, 1);

    // position de la camera
    int32_t camPosLocation = glGetUniformLocation(program, "u_CameraPosition");
    glUniform3fv(camPosLocation, 1, &cam.Position.x);

    glBindBuffer(GL_UNIFORM_BUFFER, MaterialUBO);
    Material &mat = Material();

    void *mappedBuffer = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(Material), GL_MAP_WRITE_BIT);
    Material *ubo = reinterpret_cast<Material *>(mappedBuffer);
    *ubo = mat;
    glUnmapBuffer(GL_UNIFORM_BUFFER);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture);

    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void Display(GLFWwindow *window)
{
    RenderOffscreen();

    glfwGetWindowSize(window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    Framebuffer::RenderToBackBuffer(width, height);

    GLuint basicProgram = shader.GetProgram();
    glUseProgram(basicProgram);

    int32_t samplerLocation = glGetUniformLocation(basicProgram, "u_Texture");
    glUniform1i(samplerLocation, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, offscreenBuffer.colorBuffer);

    //
    // MATRICE DE SCALE
    //
    const float scale[] = {1.f, 0.f, 0.f, 0.f,
                           0.f, 1.f, 0.f, 0.f,
                           0.f, 0.f, 1.f, 0.f,
                           0.f, 0.f, 0.f, 1.f};

    const GLint matScaleLocation = glGetUniformLocation(basicProgram, "u_scale");
    glUniformMatrix4fv(matScaleLocation, 1, false, scale);

    //
    // MATRICE DE ROTATION
    //
    const float rot[] = {1.f, 0.f, 0.f, 0.f,
                         0.f, 1.f, 0.f, 0.f,
                         0.f, 0.f, 1.f, 0.f,
                         0.f, 0.f, 0.f, 1.f};

    const GLint matRotLocation = glGetUniformLocation(basicProgram, "u_rotation");
    glUniformMatrix4fv(matRotLocation, 1, false, rot);

    //
    // MATRICE DE TRANSLATION
    //
    const float translation[] = {1.f, 0.f, 0.f, 0.f,
                                 0.f, 1.f, 0.f, 0.f,
                                 0.f, 0.f, 1.f, 0.f,
                                 0.f, 0.f, -15.f, 1.f};

    const GLint matTranslationLocation = glGetUniformLocation(basicProgram, "u_translation");
    glUniformMatrix4fv(matTranslationLocation, 1, false, translation);

    auto world = finalGlobalTransform.Double44();

    const float worldMat[] = {
        static_cast<float>(world[0][0]), static_cast<float>(world[0][1]), static_cast<float>(world[0][2]), static_cast<float>(world[0][3]),
        static_cast<float>(world[1][0]), static_cast<float>(world[1][1]), static_cast<float>(world[1][2]), static_cast<float>(world[1][3]),
        static_cast<float>(world[2][0]), static_cast<float>(world[2][1]), static_cast<float>(world[2][2]), static_cast<float>(world[2][3]),
        static_cast<float>(world[3][0]), static_cast<float>(world[3][1]), static_cast<float>(world[3][2]), static_cast<float>(world[3][3])};

    const GLint worldMatlocation = glGetUniformLocation(basicProgram, "u_world");
    glUniformMatrix4fv(worldMatlocation, 1, false, worldMat);

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