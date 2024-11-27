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

// 函数声明
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void updateCameraRotation(GLFWwindow* window);
void processInput(GLFWwindow* window);

// 屏幕尺寸
float SCREEN_W = 1920.0f;
float SCREEN_H = 1080.0f;
float FOV = 65.0f; // 视野角度
float SENSITIVITY = 0.1f; // 鼠标灵敏度

// 近裁剪面和远裁剪面
const float NEAR_CLIP = 1.0f;
const float FAR_CLIP = 1000.0f;

// 鼠标位置和相机角度
double lastXPos = SCREEN_W / 2, lastYPos = SCREEN_H / 2;
double yaw = 110.0f, pitch = 35.0f, xPos, yPos;
unsigned int Launcher::particlesCount = 0; // 粒子计数

Camera* camera; // 相机对象
glm::highp_mat4 projection; // 投影矩阵
std::map<int, bool> heldKeys; // 按键状态映射

//!!!!!!
Texture _normalMapTexture; // 法线贴图
CubemapTexutre _skyboxTexture; // 天空盒贴图
HeightMap _heightMap; // 高度图
GLuint _cubeVao = 0; // 立方体的VAO

// 光源属性
glm::vec3 lightPos = glm::vec3(0.0f, 500.0f, 0.0f); // 光源位置
glm::vec3 lightColor = glm::vec3(1.5f, 1.5f, 1.5f); // 光源颜色
//!!!!!!


int main()
{
    srand(time(NULL)); // 随机数生成器种子

    glfwInit(); // 初始化GLFW
    // 设置OpenGL上下文版本和配置
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, true); // 窗口启动时最大化
    //glfwWindowHint(GLFW_SAMPLES, 4); // 启用多重采样抗锯齿

    // 创建窗口
    GLFWwindow* window = glfwCreateWindow(SCREEN_W, SCREEN_H, "Fireworks", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "创建GLFW窗口失败" << std::endl;
        glfwTerminate(); // 如果创建失败则终止GLFW
        return -1;
    }

    glfwMakeContextCurrent(window); // 将窗口的上下文设为当前
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); // 设置窗口大小变化的回调
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // 隐藏并捕获光标
    glfwSetKeyCallback(window, key_callback); // 设置键盘回调
    glfwSwapInterval(1); // 启用垂直同步

    // 加载OpenGL函数指针
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "初始化GLAD失败" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST); // 启用深度测试
    glDepthFunc(GL_LESS); // 设置深度函数



    // 初始化相机
    camera = new Camera(glm::vec3(0.0f, 120.0f, -430.0f));
    projection = glm::perspective(glm::radians(FOV), (GLfloat)(SCREEN_W / SCREEN_H), NEAR_CLIP, FAR_CLIP);

    // 加载纹理
    ImageLoader img;
    GLuint textureId = img.loadBMP_custom("version1/textures/circle.bmp");

    // 加载着色器
    Shader particleShader("version1/shaders/particle.vert", "version1/shaders/particle.frag");

    //!!!!!!
    Shader _terrainShader("version1/shaders/terrain.vert", "version1/shaders/terrain.frag");
    Shader _skyboxShader("version1/shaders/skybox.vert", "version1/shaders/skybox.frag");

    // 加载法线贴图
    _normalMapTexture.Load("version1/textures/sky/NormalMap.png");

    // 加载高度图
    _heightMap.Load("version1/textures/sky/HeightMap.png");

    // 加载天空盒贴图
    std::vector<std::string> skyboxTextureFilePaths;
    skyboxTextureFilePaths.push_back("version1/textures/sky/right.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/left.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/top.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/bottom.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/front.png");
    skyboxTextureFilePaths.push_back("version1/textures/sky/back.png");
    _skyboxTexture.Create(skyboxTextureFilePaths); // 创建天空盒贴图

    // 初始化立方体顶点（用于天空盒）
    std::vector<glm::vec3> cubeVertices;
    std::vector<unsigned int> cubeIndices;
    float d = 0.5f; // 立方体大小
    // 添加所有可能的三角形
    cubeVertices.push_back(glm::vec3(-d, d, d)); // 上
    cubeVertices.push_back(glm::vec3(-d, d, -d));
    cubeVertices.push_back(glm::vec3(d, d, -d));
    cubeVertices.push_back(glm::vec3(d, d, d));
    cubeVertices.push_back(glm::vec3(-d, -d, d)); // 下
    cubeVertices.push_back(glm::vec3(-d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, d));
    cubeVertices.push_back(glm::vec3(-d, d, d)); // 前
    cubeVertices.push_back(glm::vec3(-d, -d, d));
    cubeVertices.push_back(glm::vec3(d, -d, d));
    cubeVertices.push_back(glm::vec3(d, d, d));
    cubeVertices.push_back(glm::vec3(-d, d, -d)); // 后
    cubeVertices.push_back(glm::vec3(-d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, d, -d));
    cubeVertices.push_back(glm::vec3(d, d, -d)); // 右
    cubeVertices.push_back(glm::vec3(d, -d, -d));
    cubeVertices.push_back(glm::vec3(d, -d, d));
    cubeVertices.push_back(glm::vec3(d, d, d));
    cubeVertices.push_back(glm::vec3(-d, d, -d)); // 左
    cubeVertices.push_back(glm::vec3(-d, -d, -d));
    cubeVertices.push_back(glm::vec3(-d, -d, d));
    cubeVertices.push_back(glm::vec3(-d, d, d));
    cubeIndices = { 0, 1, 3, 1, 2, 3, 7, 5, 4, 7, 6, 5, 11, 9, 8, 11, 10, 9, 12, 13, 15, 13, 14, 15, 16, 17, 19, 17, 18, 19, 23, 21, 20, 23, 22, 21 };
    
    GLuint VBO;
    GLuint EBO;
    glGenVertexArrays(1, &_cubeVao);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // 绑定VAO
    glBindVertexArray(_cubeVao);
    // 绑定VBO并加载顶点数据
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(glm::vec3), &cubeVertices[0], GL_STATIC_DRAW);
    // 绑定EBO并加载索引数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), &cubeIndices[0], GL_STATIC_DRAW);
    // 启用顶点属性
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Unbind VAO
    glBindVertexArray(0);
    //!!!!!!
    

    GLuint _particleVAO; // 顶点数组对象
    glGenVertexArrays(1, &_particleVAO); // 生成VAO
    glBindVertexArray(_particleVAO); // 绑定VAO

    GLuint u_textureId = glGetUniformLocation(particleShader.id, "sampler");

    GLuint cameraRightId = glGetUniformLocation(particleShader.id, "cameraRight");
    GLuint cameraUpId = glGetUniformLocation(particleShader.id, "cameraUp");
    GLuint viewProjId = glGetUniformLocation(particleShader.id, "VP");

    // 为粒子位置和颜色分配内存
    GLfloat* particle_position = new GLfloat[maxParticles * 4];
    GLubyte* particle_color = new GLubyte[maxParticles * 4];

    // 创建用于四边形的顶点缓冲区
    GLuint billboard_vertex_buffer;
    glGenBuffers(1, &billboard_vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // 创建粒子位置和颜色的缓冲区
    GLuint particles_position_buffer;
    glGenBuffers(1, &particles_position_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

    GLuint particles_color_buffer;
    glGenBuffers(1, &particles_color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
    glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);

    Launcher launcher; // 粒子发射器
    int nbFrames = 0; // 帧计数
    double lastTime = glfwGetTime(); // FPS计算的时间

    glClearColor(0, 0.1f, 0.2f, 0.8f); // 设置清屏颜色
    // 主循环
    while (!glfwWindowShouldClose(window))
    {
        nbFrames++; // 增加帧计数
        // 计算FPS
        if (glfwGetTime() - lastTime >= 1.0) {
            printf("%i fps\n", nbFrames);
            nbFrames = 0; // 重置帧计数
            lastTime += 1.0; // 更新上次时间
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 清除缓冲区
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);

        Camera::updateDeltaTime(); // 更新相机的时间增量
        launcher.update(*camera, particle_position, particle_color); // 更新粒子

        processInput(window); // 处理输入

        // 计算视图投影矩阵
        auto viewMatrix = camera->getWorldToViewMatrix();
        auto vp = projection * viewMatrix;

        //!!!!!!
        //必须在渲染粒子之前渲染天空盒
        glm::mat4 view = camera->to_mat4(); // 视图矩阵
        // 渲染天空盒
        static Transform skyBoxTransform;
        skyBoxTransform.position = camera->getPosition() + glm::vec3(0, -10, 0); // 调整天空盒位置
        skyBoxTransform.scale = glm::vec3(100.f); // 调整天空盒大小
        skyBoxTransform.rotation.y -= 0.00015f; // 小幅度旋转以模拟云层移动

        // 绑定天空盒着色器并设置矩阵
        _skyboxShader.use();
        _skyboxShader.SetMat4("projection", projection);
        _skyboxShader.SetMat4("view", view);
        _skyboxShader.SetMat4("model", skyBoxTransform.to_mat4());
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _skyboxTexture.ID); // 绑定天空盒纹理
        glBindVertexArray(_cubeVao); // 绑定立方体VAO
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // 绘制天空盒
        glClear(GL_DEPTH_BUFFER_BIT); // 清除深度缓冲
        
        // 渲染高度图
        Transform heightMapTransform;
        heightMapTransform.scale = glm::vec3(2.0f); // 调整高度图大小
        _terrainShader.use();
        _terrainShader.SetMat4("projection", projection);
        _terrainShader.SetMat4("view", view);
        _terrainShader.SetMat4("model", heightMapTransform.to_mat4());
        _terrainShader.SetVec3("viewPos", glm::vec3(camera->to_mat4()[3])); // 设置视点位置
        _terrainShader.SetVec3("lightPos", lightPos); // 光源位置
        _terrainShader.SetVec3("lightColor", lightColor); // 光源颜色
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _normalMapTexture.ID); // 绑定法线贴图
        glBindVertexArray(_heightMap.vao);
        glDrawElements(GL_TRIANGLE_STRIP, _heightMap.indexCount, GL_UNSIGNED_INT, 0); // 绘制高度图
        glBindVertexArray(0);
        glClear(GL_DEPTH_BUFFER_BIT); // 清除深度缓冲
        //!!!!!!

        glBindVertexArray(_particleVAO);
        // 更新粒子位置缓冲区
        glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
        glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Launcher::particlesCount * sizeof(GLfloat) * 4, particle_position);

        // 更新粒子颜色缓冲区
        glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
        glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLubyte), NULL, GL_STREAM_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, Launcher::particlesCount * sizeof(GLubyte) * 4, particle_color);

        // 绑定纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(u_textureId, 0); // 设置纹理统一变量

        glEnable(GL_BLEND); // 启用混合
        particleShader.use(); // 使用粒子着色器

        // 设置着色器统一变量
        glUniform3f(cameraRightId, viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        glUniform3f(cameraUpId, viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
        glUniformMatrix4fv(viewProjId, 1, GL_FALSE, &vp[0][0]);

        // 设置广告牌的顶点属性
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, billboard_vertex_buffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // 粒子位置属性
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, particles_position_buffer);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // 粒子颜色属性
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, particles_color_buffer);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)0);

        // 设置顶点属性的除数
        glVertexAttribDivisor(0, 0); // 每个粒子的四个顶点
        glVertexAttribDivisor(1, 1); // 每个四边形一个位置
        glVertexAttribDivisor(2, 1); // 每个四边形一个颜色

        //绘制粒子
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, Launcher::particlesCount);


        // 禁用顶点属性
        //glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        glfwSwapBuffers(window); // 交换缓冲区
        glfwPollEvents(); // 处理事件
    }

    // 清理
    delete[] particle_position; // 释放粒子位置数组
    delete[] particle_color; // 释放粒子颜色数组

    // 删除OpenGL资源
    glDeleteBuffers(1, &particles_color_buffer);
    glDeleteBuffers(1, &particles_position_buffer);
    glDeleteBuffers(1, &billboard_vertex_buffer);
    glDeleteTextures(1, &textureId); // 删除纹理
    glDeleteVertexArrays(1, &_particleVAO);

    glfwTerminate(); // 终止GLFW
    return 0;
}

// 通过鼠标控制摄像头视角
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

// 如果用户一直按着某个键，则无需改变该键的状态，否则更新键盘输入状态
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_REPEAT)
        return;
    heldKeys[key] = action == GLFW_PRESS;
}

// 通过键盘改变摄像头位置
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

// 帧缓冲区大小变化的回调
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    SCREEN_W = (float)width; // 更新屏幕宽度
    SCREEN_H = (float)height; // 更新屏幕高度

    // 更新投影矩阵
    projection = glm::perspective(glm::radians(FOV), (GLfloat)(SCREEN_W / SCREEN_H), NEAR_CLIP, FAR_CLIP);

    glViewport(0, 0, width, height);// 更新视口
}
