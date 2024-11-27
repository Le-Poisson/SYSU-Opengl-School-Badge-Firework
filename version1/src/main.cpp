#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Windows.h>
#include <gl/GLU.h>
#include <gl/GL.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>

#include "camera.h"
#include "shader.h"
#include "utils.h"
#include "launcher.h"
#include "image-loader.h"

// ��������
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void updateCameraRotation(GLFWwindow* window);
void processInput(GLFWwindow* window);

// ��Ļ�ߴ�
float SCREEN_W = 1920.0f;
float SCREEN_H = 1080.0f;
float FOV = 65.0f; // ��Ұ�Ƕ�
float SENSITIVITY = 0.1f; // ���������

// ���ü����Զ�ü���
const float NEAR_CLIP = 1.0f;
const float FAR_CLIP = 1000.0f;

// ���λ�ú�����Ƕ�
double lastXPos = SCREEN_W / 2, lastYPos = SCREEN_H / 2;
double yaw = 110.0f, pitch = 35.0f, xPos, yPos;
unsigned int Launcher::particlesCount = 0; // ���Ӽ���

Camera* camera; // �������
glm::highp_mat4 projection; // ͶӰ����
std::map<int, bool> heldKeys; // ����״̬ӳ��

//!!!!!!
Texture _normalMapTexture; // ������ͼ
CubemapTexutre _skyboxTexture; // ��պ���ͼ
HeightMap _heightMap; // �߶�ͼ
GLuint _cubeVao = 0; // �������VAO

// ��Դ����
glm::vec3 lightPos = glm::vec3(0.0f, 500.0f, 0.0f); // ��Դλ��
glm::vec3 lightColor = glm::vec3(1.5f, 1.5f, 1.5f); // ��Դ��ɫ
//!!!!!!


int main()
{
    srand(time(NULL)); // ���������������

    glfwInit(); // ��ʼ��GLFW
    // ����OpenGL�����İ汾������
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, true); // ��������ʱ���
    //glfwWindowHint(GLFW_SAMPLES, 4); // ���ö��ز��������

    // ��������
    GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Fireworks", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "����GLFW����ʧ��" << std::endl;
        glfwTerminate(); // �������ʧ������ֹGLFW
        return -1;
    }

    glfwMakeContextCurrent(window); // �����ڵ���������Ϊ��ǰ
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // ���ô��ڴ�С�仯�Ļص�
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // ���ز�������
    glfwSetKeyCallback(window, key_callback); // ���ü��̻ص�
    glfwSwapInterval(1); // ���ô�ֱͬ��

    // ����OpenGL����ָ��
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "��ʼ��GLADʧ��" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST); // ������Ȳ���
    glDepthFunc(GL_LESS); // ������Ⱥ���



    // ��ʼ�����
    camera = new Camera(glm::vec3(0.0f, 120.0f, -430.0f));
    projection = glm::perspective(glm::radians(FOV), (GLfloat)(SCREEN_W / SCREEN_H), NEAR_CLIP, FAR_CLIP);

    // ��������
    ImageLoader img;
    GLuint textureId = img.loadBMP_custom("version1/textures/circle.bmp");

    // ������ɫ��
    Shader particleShader("version1/shaders/particle.vert", "version1/shaders/particle.frag");

    //!!!!!!
    Shader _terrainShader("version1/shaders/terrain.vert", "version1/shaders/terrain.frag");
    Shader _skyboxShader("version1/shaders/skybox.vert", "version1/shaders/skybox.frag");

    // ���ط�����ͼ
    _normalMapTexture.Load("version1/textures/sky/NormalMap.png");

    // ���ظ߶�ͼ
    _heightMap.Load("version1/textures/sky/HeightMap.png");

    // ������պ���ͼ
    std::vector<std::string> skyboxTextureFilePaths;
    skyboxTextureFilePaths.push_back("version1/textures/sky/right.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/left.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/top.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/bottom.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/front.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/back.png");
    _skyboxTexture.Create(skyboxTextureFilePaths); // ������պ���ͼ

    // ��ʼ�������嶥�㣨������պУ�
    std::vector<glm::vec3> cubeVertices;
    std::vector<unsigned int> cubeIndices;
    float d = 0.5f; // �������С
    // ������п��ܵ�������
    cubeVertices.push_back(glm::vec3(-d, d, d)); // ��
    cubeVertices.push_back(glm::vec3(-d, d, -d));
    cubeVertices.push_back(glm::vec3(d, d, -d));
    cubeVertices.push_back(glm::vec3(d, d, d));
    cubeVertices.push_back(glm::vec3(-d, -d, d)); // ��
    cubeVertices.push_back(glm::vec3(-d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, d));
    cubeVertices.push_back(glm::vec3(-d, d, d)); // ǰ
    cubeVertices.push_back(glm::vec3(-d, -d, d));
    cubeVertices.push_back(glm::vec3(d, -d, d));
    cubeVertices.push_back(glm::vec3(d, d, d));
    cubeVertices.push_back(glm::vec3(-d, d, -d)); // ��
    cubeVertices.push_back(glm::vec3(-d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, d, -d));
    cubeVertices.push_back(glm::vec3(d, d, -d)); // ��
    cubeVertices.push_back(glm::vec3(d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, d));
    cubeVertices.push_back(glm::vec3(d, d, d));
    cubeVertices.push_back(glm::vec3(-d, d, -d)); // ��
    cubeVertices.push_back(glm::vec3(-d, -d, -d));
    cubeVertices.push_back(glm::vec3(-d, -d, d));
    cubeVertices.push_back(glm::vec3(-d, d, d));
    cubeIndices = { 0, 1, 3, 1, 2, 3, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 12, 13, 15, 13, 14, 15, 16, 17, 19, 17, 18, 19, 23, 21, 20, 23, 22, 21 };
    
    GLuint VBO;
    GLuint EBO;
    glGenVertexArrays(1, &_cubeVao);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // ��VAO
    glBindVertexArray(_cubeVao);
    // ��VBO�����ض�������
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(glm::vec3), &cubeVertices[0], GL_STATIC_DRAW);
    // ��EBO��������������
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), &cubeIndices[0], GL_STATIC_DRAW);
    // ���ö�������
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Unbind VAO
    glBindVertexArray(0);
    //!!!!!!
    

    GLuint _particleVAO; // �����������
    glGenVertexArrays(1, &_particleVAO); // ����VAO
    glBindVertexArray(_particleVAO); // ��VAO

    GLuint u_textureId = glGetUniformLocation(particleShader.id, "sampler");

    GLuint cameraRightId = glGetUniformLocation(particleShader.id, "cameraRight");
    GLuint cameraUpId = glGetUniformLocation(particleShader.id, "cameraUp");
    GLuint viewProjId = glGetUniformLocation(particleShader.id, "VP");

    // Ϊ����λ�ú���ɫ�����ڴ�
    GLfloat* particle_position = new GLfloat[maxParticles * 4];
    GLubyte* particle_color = new GLubyte[maxParticles * 4];

    // ���������ı��εĶ��㻺����
    GLuint billboard_vertex_buffer;
    glGenBuffers(1, &billboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // ��������λ�ú���ɫ�Ļ�����
    GLuint particles_position_buffer;
    glGenBuffers(1, &particles_position_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    GLuint particles_color_buffer;
    glGenBuffers(1, &particles_color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

    Launcher launcher; // ���ӷ�����
    int nbFrames = 0; // ֡����
    double lastTime = glfwGetTime(); // FPS�����ʱ��

    glClearColor(0, 0.1f, 0.2f, 0.8f); // ����������ɫ
    // ��ѭ��
    while (!glfwWindowShouldClose(window))
    {
        nbFrames++; // ����֡����
        // ����FPS
        if (glfwGetTime() - lastTime >= 1.0) {
            printf("%i fps\n", nbFrames);
            nbFrames = 0; // ����֡����
            lastTime += 1.0; // �����ϴ�ʱ��
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // ���������
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        Camera::updateDeltaTime(); // ���������ʱ������
        launcher.update(*camera, particle_position, particle_color); // ��������

        processInput(window); // ��������

        // ������ͼͶӰ����
        auto viewMatrix = camera->getWorldToViewMatrix();
        auto vp = projection * viewMatrix;

        //!!!!!!
        //��������Ⱦ����֮ǰ��Ⱦ��պ�
        glm::mat4 view = camera->to_mat4(); // ��ͼ����
        // ��Ⱦ��պ�
        static Transform skyBoxTransform;
        skyBoxTransform.position = camera->getPosition() + glm::vec3(0, -10, 0); // ������պ�λ��
        skyBoxTransform.scale = glm::vec3(100.f); // ������պд�С
        skyBoxTransform.rotation.y -= 0.00015f; // С������ת��ģ���Ʋ��ƶ�

        // ����պ���ɫ�������þ���
        _skyboxShader.use();
        _skyboxShader.SetMat4("projection", projection);
        _skyboxShader.SetMat4("view", view);
        _skyboxShader.SetMat4("model", skyBoxTransform.to_mat4());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _skyboxTexture.ID); // ����պ�����
        glBindVertexArray(_cubeVao); // ��������VAO
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // ������պ�
        glClear(GL_DEPTH_BUFFER_BIT); // �����Ȼ���
        
        // ��Ⱦ�߶�ͼ
        Transform heightMapTransform;
        heightMapTransform.scale = glm::vec3(2.0f); // �����߶�ͼ��С
        _terrainShader.use();
        _terrainShader.SetMat4("projection", projection);
        _terrainShader.SetMat4("view", view);
        _terrainShader.SetMat4("model", heightMapTransform.to_mat4());
        _terrainShader.SetVec3("viewPos", glm::vec3(camera->to_mat4()[3])); // �����ӵ�λ��
        _terrainShader.SetVec3("lightPos", lightPos); // ��Դλ��
        _terrainShader.SetVec3("lightColor", lightColor); // ��Դ��ɫ
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _normalMapTexture.ID); // �󶨷�����ͼ
        glBindVertexArray(_heightMap.vao);
        glDrawElements(GL_TRIANGLE_STRIP, _heightMap.indexCount, GL_UNSIGNED_INT, 0); // ���Ƹ߶�ͼ
        glBindVertexArray(0);
        glClear(GL_DEPTH_BUFFER_BIT); // �����Ȼ���
        //!!!!!!

        glBindVertexArray(_particleVAO);
        // ��������λ�û�����
        glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
        glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Launcher::particlesCount * sizeof(GLfloat) * 4, particle_position);

        // ����������ɫ������
        glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
        glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Launcher::particlesCount * sizeof(GLubyte) * 4, particle_color);

        // ������
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(u_textureId, 0); // ��������ͳһ����

        glEnable(GL_BLEND); // ���û��
        particleShader.use(); // ʹ��������ɫ��

        // ������ɫ��ͳһ����
        glUniform3f(cameraRightId, viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        glUniform3f(cameraUpId, viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
        glUniformMatrix4fv(viewProjId, 1, GL_FALSE, &vp[0][0]);

        // ���ù���ƵĶ�������
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // ����λ������
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // ������ɫ����
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

        // ���ö������Եĳ���
        glVertexAttribDivisor(0, 0); // ÿ�����ӵ��ĸ�����
        glVertexAttribDivisor(1, 1); // ÿ���ı���һ��λ��
        glVertexAttribDivisor(2, 1); // ÿ���ı���һ����ɫ

        //��������
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, Launcher::particlesCount);


        // ���ö�������
        //glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        glfwSwapBuffers(window); // ����������
        glfwPollEvents(); // �����¼�
    }

    // ����
    delete[] particle_position; // �ͷ�����λ������
    delete[] particle_color; // �ͷ�������ɫ����

    // ɾ��OpenGL��Դ
    glDeleteBuffers(1, &particles_color_buffer);
    glDeleteBuffers(1, &particles_position_buffer);
    glDeleteBuffers(1, &billboard_vertex_buffer);
    glDeleteTextures(1, &textureId); // ɾ������
    glDeleteVertexArrays(1, &_particleVAO);

    glfwTerminate(); // ��ֹGLFW
    return 0;
}

// ͨ������������ͷ�ӽ�
void updateCameraRotation(GLFWwindow* window)
{
    glfwGetCursorPos(window, &xPos, &yPos);

    yaw += (xPos - lastXPos) * SENSITIVITY;
    pitch += (lastYPos - yPos) * SENSITIVITY;
    pitch = clamp(pitch, 89.0f, -89.0f);

    lastXPos = xPos;
    lastYPos = yPos;

    camera->rotate((float)yaw, (float)pitch);
}

// ����û�һֱ����ĳ������������ı�ü���״̬��������¼�������״̬
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;
    heldKeys[key] = action == GLFW_PRESS;
}

// ͨ�����̸ı�����ͷλ��
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera controls
    updateCameraRotation(window);

    if (heldKeys[GLFW_KEY_SPACE])
        camera->moveUp(2.0f);
    if (heldKeys[GLFW_KEY_LEFT_SHIFT])
        camera->moveUp(-2.0f);
    if (heldKeys[GLFW_KEY_W])
        camera->moveForward(2.0f);
    if (heldKeys[GLFW_KEY_S])
        camera->moveForward(-2.0f);
    if (heldKeys[GLFW_KEY_D])
        camera->moveLeft(1.5f);
    if (heldKeys[GLFW_KEY_A])
        camera->moveLeft(-1.5f);
}

// ֡��������С�仯�Ļص�
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    SCREEN_W = (float)width; // ������Ļ���
    SCREEN_H = (float)height; // ������Ļ�߶�

    // ����ͶӰ����
    projection = glm::perspective(glm::radians(FOV), (GLfloat)(SCREEN_W / SCREEN_H), NEAR_CLIP, FAR_CLIP);

    glViewport(0, 0, width, height);// �����ӿ�
}
