#include <iostream>
#include <cmath>
#include <vector>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

bool inTopDownView = false;
glm::vec3 cameraPos = glm::vec3(0.0f, 20.0f, 0.1f);
glm::vec3 cameraFront = glm::vec3(0.0f, -1.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);

float yaw = -90.0f; 
float pitch = -89.9f;

glm::vec3 topDownPosition = glm::vec3(0.0f, 0.0f, 20.0f); 
glm::vec3 topDownFront = glm::vec3(0.0f, 0.0f, -1.0f);    
glm::vec3 topDownUp = glm::vec3(0.0f, 1.0f, 0.0f);        

glm::vec3 savedCameraPos;
glm::vec3 savedCameraFront;
glm::vec3 savedCameraUp;
float savedYaw;
float savedPitch;

float fov = 45.0f;  
float cameraSpeed = 5.0f; 
bool cursorCaptured = true;
float lastFrame = 0.0f;

const int numSpheres = 6;

void updateCameraDirection(GLFWwindow* window) {
    const float cameraSpeed = 0.05f; 
    if (!inTopDownView) {
        // Adjust yaw with O (right) and U (left) keys
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {

            yaw -= cameraSpeed;
        }
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
            yaw += cameraSpeed;
        }

        // Adjust pitch with I (up) and P (down) keys
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
            pitch += cameraSpeed;
            if (pitch > 89.0f) pitch = 89.0f; 
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            pitch -= cameraSpeed;
            if (pitch < -89.0f) pitch = -89.0f; 
        }

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}
void createSphere(std::vector<float> &vertices, std::vector<unsigned int> &indices, float radius, unsigned int sectorCount, unsigned int stackCount) {
    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float s, t;

    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for (unsigned int i = 0; i <= stackCount; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);


        for (unsigned int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;

            // Vertex position
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Adjusted normals for the poles
            if (i == 0) { // Top pole
                nx = 0;
                ny = 1;
                nz = 0;
            } else if (i == stackCount) { // Bottom pole
                nx = 0;
                ny = -1;
                nz = 0;
            } else { // Rest of the sphere
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
            }

            // Push normal to vertices
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
        }
    }

    unsigned int k1, k2;
    for (unsigned int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

        for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}


// Vertex Shader Source
const char *vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    out vec3 Normal;
    out vec3 FragPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = aNormal;  
        
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)glsl";

// Fragment Shader Source
const char *fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;

    struct Material {
        vec3 specular;
        float shininess;
    };

    struct Light {
        vec3 position;
        vec3 ambient;
        vec3 diffuse;
        vec3 specular;
    };

    uniform Material material;
    uniform Light lights[2];  // Array of lights
    uniform vec3 viewPos;
    uniform vec3 sphereColor;

    in vec3 Normal;
    in vec3 FragPos;

    vec3 CalculateLight(Light light, vec3 normal, vec3 viewDir, vec3 fragPos) {
        vec3 lightDir = normalize(light.position - fragPos);
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = light.diffuse * (diff * sphereColor);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        vec3 specular = light.specular * (spec * material.specular);
        vec3 ambient = light.ambient * sphereColor;
        return (ambient + diffuse + specular);
    }

    void main() {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 result = vec3(0.0);

        for (int i = 0; i < 2; i++) {  // Loop over both lights
            result += CalculateLight(lights[i], norm, viewDir, FragPos);
        }

        FragColor = vec4(result, 1.0);
    }
)glsl";

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

unsigned int compileShader(unsigned int type, const char *source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, NULL);
    glCompileShader(id);
    return id;
}

unsigned int createShaderProgram(const char *vertexShaderSrc, const char *fragmentShaderSrc) {
    unsigned int program = glCreateProgram();
    unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}


int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(800, 600, "Solar System", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(shaderProgram);
    glm::vec3 materialAmbient = glm::vec3(1.0f, 0.5f, 0.31f);
    glm::vec3 materialDiffuse = glm::vec3(1.0f, 0.5f, 0.31f);
    glm::vec3 materialSpecular = glm::vec3(0.5f, 0.5f, 0.5f);
    float materialShininess = 32.0f;



    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 lightPosition = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 lightAmbient = lightColor * glm::vec3(0.2f);
    glm::vec3 lightDiffuse = lightColor * glm::vec3(0.5f);
    glm::vec3 lightSpecular = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::vec3 lightColor2 = glm::vec3(1.0f, 0.0f, 0.0f); 
    glm::vec3 lightPosition2 = glm::vec3(-2.0f, -2.0f, 2.0f);
    glm::vec3 lightAmbient2 = lightColor2 * glm::vec3(0.1f); 
    glm::vec3 lightDiffuse2 = lightColor2 * glm::vec3(0.7f); 
    glm::vec3 lightSpecular2 = glm::vec3(1.0f, 1.0f, 1.0f); 



    unsigned int sphereColorLoc = glGetUniformLocation(shaderProgram, "sphereColor");

    unsigned int materialAmbientLoc = glGetUniformLocation(shaderProgram, "material.ambient");
    unsigned int materialDiffuseLoc = glGetUniformLocation(shaderProgram, "material.diffuse");
    unsigned int materialSpecularLoc = glGetUniformLocation(shaderProgram, "material.specular");
    unsigned int materialShininessLoc = glGetUniformLocation(shaderProgram, "material.shininess");

    unsigned int lightPosLoc = glGetUniformLocation(shaderProgram, "light.position");
    unsigned int lightAmbientLoc = glGetUniformLocation(shaderProgram, "light.ambient");
    unsigned int lightDiffuseLoc = glGetUniformLocation(shaderProgram, "light.diffuse");
    unsigned int lightSpecularLoc = glGetUniformLocation(shaderProgram, "light.specular");
    unsigned int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");

    unsigned int lightPosLoc2 = glGetUniformLocation(shaderProgram, "light2.position");
    unsigned int lightAmbientLoc2 = glGetUniformLocation(shaderProgram, "light2.ambient");
    unsigned int lightDiffuseLoc2 = glGetUniformLocation(shaderProgram, "light2.diffuse");
    unsigned int lightSpecularLoc2 = glGetUniformLocation(shaderProgram, "light2.specular");


    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    glm::mat4 projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");


    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    createSphere(vertices, indices, 1.0f, 36, 18); /

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    glm::vec3 sphereColors[numSpheres] = {
        glm::vec3(1.0f, 1.0f, 0.0f),  
        glm::vec3(0.0f, 0.0f, 1.0f),  
        glm::vec3(1.0f, 0.0f, 0.0f), 
        glm::vec3(0.5f, 0.0f, 0.5f),  
        glm::vec3(0.6f, 0.3f, 0.2f),  
        glm::vec3(0.0f, 1.0f, 0.0f)   
    };

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        updateCameraDirection(window);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            cursorCaptured = !cursorCaptured;
            glfwSetInputMode(window, GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }
        static bool tKeyPressed = 0;
        glUniform3fv(materialAmbientLoc, 1, glm::value_ptr(materialAmbient));
        glUniform3fv(materialDiffuseLoc, 1, glm::value_ptr(materialDiffuse));
        glUniform3fv(materialSpecularLoc, 1, glm::value_ptr(materialSpecular));
        glUniform1f(materialShininessLoc, materialShininess);


        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[0].position"), 1, glm::value_ptr(lightPosition));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[0].ambient"), 1, glm::value_ptr(lightAmbient));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[0].diffuse"), 1, glm::value_ptr(lightDiffuse));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[0].specular"), 1, glm::value_ptr(lightSpecular));

        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[1].position"), 1, glm::value_ptr(lightPosition2));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[1].ambient"), 1, glm::value_ptr(lightAmbient2));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[1].diffuse"), 1, glm::value_ptr(lightDiffuse2));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lights[1].specular"), 1, glm::value_ptr(lightSpecular2));
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
            tKeyPressed = true;
            inTopDownView = !inTopDownView;

            if (inTopDownView) {
                // Save current camera state
                savedCameraPos = cameraPos;
                savedCameraFront = cameraFront;
                savedCameraUp = cameraUp;
                savedYaw = yaw;
                savedPitch = pitch;

                // Set top-down view
                cameraPos = topDownPosition;
                cameraFront = topDownFront;
                cameraUp = topDownUp;
            } else {
                // Restore saved camera state
                cameraPos = savedCameraPos;
                cameraFront = savedCameraFront;
                cameraUp = savedCameraUp;
                yaw = savedYaw;
                pitch = savedPitch;

                // Update camera front vector based on saved yaw and pitch
                glm::vec3 front;
                front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.y = sin(glm::radians(pitch));
                front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                cameraFront = glm::normalize(front);
            }
        } else if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
            tKeyPressed = false;
        }
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Movement speed per frame
        float moveStep = cameraSpeed * deltaTime;

        // Forward movement (W key)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cameraPos += moveStep * cameraFront;
        }
        
        // Backward movement (S key)
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cameraPos -= moveStep * cameraFront;
        }
        
        // Left movement (A key)
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * moveStep;
        }
        
        // Right movement (D key)
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * moveStep;
        }


        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);


        glUseProgram(shaderProgram);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 projection = glm::perspective(glm::radians(fov), 800.0f / 600.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

            if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        fov -= 1.0f;
        if (fov < 1.0f) fov = 1.0f; 
    }

    // Check for zoom out (X key)
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        fov += 1.0f;
        if (fov > 45.0f) fov = 45.0f; 
    }


        glBindVertexArray(VAO);

        for (int i = 0; i < numSpheres; ++i) {
            glm::mat4 model = glm::mat4(1.0f);

            if (i == 0) {
                model = glm::scale(model, glm::vec3(1.5f)); 
            } else {
                // Orbiting spheres
                float angle = (float)glfwGetTime() * (i * 0.2f);
                float distance = 2.5f * i; 
                model = glm::translate(model, glm::vec3(distance * cos(angle), distance * sin(angle), 0.0f));
                model = glm::scale(model, glm::vec3(0.5f)); 
            }


            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));


            glUniform3fv(sphereColorLoc, 1, glm::value_ptr(sphereColors[i]));

            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}