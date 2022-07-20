#pragma once

#include "Common.hpp"
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/rotate_vector.hpp>
#include <gtx/vector_angle.hpp>

#include "GLShader.hpp"

class Camera
{
public:
    // Stores the main vectors of the camera
    glm::vec3 Position;
    glm::vec3 Orientation = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);

    // Prevents the camera from jumping around when first clicking left click
    bool firstClick = true;

    // Stores the width and height of the window
    int width;
    int height;

    // Adjust the speed of the camera and it's sensitivity when looking around
    float speed = 1.5f;
    float sensitivity = 100.0f;

    // Camera constructor to set up initial values
    Camera(int, int, glm::vec3);

    // Updates and exports the camera matrix to the Vertex Shader
    void Matrix(float, float, float, GLShader &, const char *);

    void MatrixOffScreen(float, float, float, uint32_t);

    // Handles camera inputs
    void Inputs(GLFWwindow *);
};