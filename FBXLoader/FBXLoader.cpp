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

std::vector<Vertex> m_vertices;

GLuint m_VAO;
GLuint m_VBO;
GLuint m_texId;

// Shader
GLShader m_shader;

// Scene FBX
FbxManager *m_fbxManager;
FbxScene *m_scene;

void Initialize()
{
    GLenum error = glewInit();
    if (error != GLEW_OK)
        std::cout << "erreur d'initialisation de GLEW!" << std::endl;

    // Logs
    std::cout << "Version : " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Vendor : " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer : " << glGetString(GL_RENDERER) << std::endl;

    // Shader
    m_shader.LoadVertexShader("vertex.glsl");
    m_shader.LoadFragmentShader("fragment.glsl");
    m_shader.Create();

    glGenVertexArrays(1, &m_VAO);
    glBindVertexArray(m_VAO);

    glGenBuffers(1, &m_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    constexpr int STRIDE = sizeof(Vertex);
    glBufferData(GL_ARRAY_BUFFER, STRIDE * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    const int POSITION = glGetAttribLocation(m_shader.GetProgram(), "a_position");
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, position));

    const int NORMAL = glGetAttribLocation(m_shader.GetProgram(), "a_normal");
    glEnableVertexAttribArray(NORMAL);
    glVertexAttribPointer(NORMAL, 3, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, normal));

    const int UV = glGetAttribLocation(m_shader.GetProgram(), "a_texcoords");
    glEnableVertexAttribArray(UV);
    glVertexAttribPointer(UV, 2, GL_FLOAT, false, STRIDE, (void *)offsetof(Vertex, texcoords));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
    glUniformMatrix4fv(matRotLocation, 1, false, rotY);

    //
    // MATRICE DE TRANSLATION
    //
    const float translation[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, -10.f, 1.f
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

    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());
}

static void ErrorCallback(int error, const char *description)
{
    std::cout << "Error GFLW " << error << " : " << description << std::endl;
}

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void LoadFBX()
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

// Parcours du Scene Graph
static void ProcessNode(FbxNode *node, FbxNode *parent)
{
    FbxNodeAttribute *att = node->GetNodeAttribute();
    FbxNodeAttribute::EType type = att->GetAttributeType();

    switch (type)
    {
    case FbxNodeAttribute::eMesh:
        // etape 1. calcul de la matrice geometrique
        FbxVector4 translation = node->GetGeometricTranslation(FbxNode::eSourcePivot);
        FbxVector4 rotation = node->GetGeometricRotation(FbxNode::eSourcePivot);
        FbxVector4 scaling = node->GetGeometricScaling(FbxNode::eSourcePivot);
        FbxAMatrix geometryTransform;
        geometryTransform.SetTRS(translation, rotation, scaling);

        // etape 2. on recupere la matrice global (world) du mesh
        FbxAMatrix globalTransform = node->EvaluateGlobalTransform();

        // etape 3. on concatene les deux matrices, ce qui donne la matrice world finale
        auto finalGlobalTransform = globalTransform * geometryTransform;

        // etape 4. on recupère les donnée du mesh
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
            }
        }

        int materialCount = node->GetMaterialCount();
        FbxSurfaceMaterial *material = node->GetMaterial(0);
        const FbxProperty property = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
        const FbxProperty factDf = material->FindProperty(FbxSurfaceMaterial::sDiffuseFactor);

        if (property.IsValid())
        {
            FbxDouble3 color = property.Get<FbxDouble3>();
            if (factDf.IsValid())
            {
                // le facteur s’applique generalement sur la propriete (ici RGB)
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

    LoadFBX();
    FbxNode *root_node = m_scene->GetRootNode();
    FbxNode *model = root_node->GetChild(0);
    ProcessNode(model, root_node);

    Initialize();

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