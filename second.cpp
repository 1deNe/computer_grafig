#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;

bool whiteBackground = false;
bool changeColor = false;
bool lineLoopMode = false;
bool wireframeMode = false;

// =====================================================
// Терезе өлшемі өзгерген кездегі Viewport
// =====================================================

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 0-ге бөлуге жол бермеу
    if (width <= 0 || height <= 0)
        return;

    // Бастапқы экранның пропорциясы
    const float targetAspect = 16.0f / 9.0f;

    float windowAspect = static_cast<float>(width) /
                         static_cast<float>(height);

    int viewportWidth;
    int viewportHeight;
    int viewportX;
    int viewportY;

    if (windowAspect > targetAspect)
    {
        // Терезе тым кең
        // Биіктік толық қалады
        viewportHeight = height;
        viewportWidth =
            static_cast<int>(height * targetAspect);

        viewportX = (width - viewportWidth) / 2;
        viewportY = 0;
    }
    else
    {
        // Терезе тым биік
        // Ені толық қалады
        viewportWidth = width;
        viewportHeight =
            static_cast<int>(width / targetAspect);

        viewportX = 0;
        viewportY = (height - viewportHeight) / 2;
    }

    glViewport(
        viewportX,
        viewportY,
        viewportWidth,
        viewportHeight
    );
}

// =====================================================
// Vertex Shader
// =====================================================

const char* vertexSrc = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

// =====================================================
// Fragment Shader
// =====================================================

const char* fragmentSrc = R"(
#version 330 core

out vec4 FragColor;

uniform vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)";

// =====================================================
// Пернетақта
// =====================================================

void processInput(GLFWwindow* window)
{
    // ESC - шығу
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // SPACE - ақ фон
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        whiteBackground = true;
    }
    else
    {
        whiteBackground = false;
    }

    // =================================================
    // 5 - түс ауыстыру
    // =================================================

    static bool key5WasPressed = false;

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
    {
        if (!key5WasPressed)
        {
            changeColor = !changeColor;
            key5WasPressed = true;
        }
    }
    else
    {
        key5WasPressed = false;
    }

    // =================================================
    // 6 - GL_LINE_LOOP
    // =================================================

    static bool key6WasPressed = false;

    if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
    {
        if (!key6WasPressed)
        {
            lineLoopMode = !lineLoopMode;
            key6WasPressed = true;
        }
    }
    else
    {
        key6WasPressed = false;
    }

    // =================================================
    // 7 - Wireframe
    // =================================================

    static bool key7WasPressed = false;

    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
    {
        if (!key7WasPressed)
        {
            wireframeMode = !wireframeMode;
            key7WasPressed = true;
        }
    }
    else
    {
        key7WasPressed = false;
    }
}

// =====================================================
// MAIN
// =====================================================

int main()
{
    // =================================================
    // Қабат санын енгізу
    // =================================================

    int n;

    std::cout << "Үшбұрыш қабаттарының санын енгізіңіз: ";
    std::cin >> n;

    if (n < 1)
    {
        std::cout << "Қате! n кемінде 1 болуы керек."
                  << std::endl;

        return -1;
    }

    std::cout << "Қабат саны: "
              << n
              << std::endl;

    // =================================================
    // GLFW
    // =================================================

    if (!glfwInit())
    {
        std::cout << "GLFW іске қосылмады!"
                  << std::endl;

        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    GLFWwindow* window =
        glfwCreateWindow(
            WIDTH,
            HEIGHT,
            "Triangle Layers",
            nullptr,
            nullptr
        );

    if (!window)
    {
        std::cout << "Терезе ашылмады!"
                  << std::endl;

        glfwTerminate();

        return -1;
    }

    glfwMakeContextCurrent(window);

    // =================================================
    // GLAD
    // =================================================

    if (!gladLoadGL(
            (GLADloadfunc)glfwGetProcAddress))
    {
        std::cout << "GLAD іске қосылмады!"
                  << std::endl;

        glfwTerminate();

        return -1;
    }

    // =================================================
    // Терезе өлшемі өзгерген кезде
    // framebuffer_size_callback шақырылады
    // =================================================

    glfwSetFramebufferSizeCallback(
        window,
        framebuffer_size_callback
    );

    // Бастапқы viewport
    int framebufferWidth;
    int framebufferHeight;

    glfwGetFramebufferSize(
        window,
        &framebufferWidth,
        &framebufferHeight
    );

    framebuffer_size_callback(
        window,
        framebufferWidth,
        framebufferHeight
    );

    // =================================================
    // VSync өшіру
    // =================================================

    glfwSwapInterval(0);

    // =================================================
    // ҮЛКЕН ҮШБҰРЫШ ӨЛШЕМІ
    // =================================================

    float topX = 0.0f;
    float topY = 0.85f;

    float leftX = -0.85f;
    float rightX = 0.85f;

    float bottomY = -0.75f;

    // =================================================
    // Кішкентай үшбұрыш өлшемі
    // =================================================

    float width =
        (rightX - leftX) / n;

    float height =
        (topY - bottomY) / n;

    // =================================================
    // VERTICES
    // =================================================

    std::vector<float> vertices;

    // =================================================
    // ҚАБАТТАР
    // =================================================

    for (int layer = 0; layer < n; layer++)
    {
        // Төменде n
        // жоғарыда n-1
        // ...
        // ең үстінде 1

        int count = n - layer;

        // Қабаттың Y координатасы
        float y =
            bottomY + (layer + 1) * height;

        // Қабатты ортасына орналастыру
        float startX =
            -((count * width) / 2.0f);

        // =================================================
        // Осы қабаттағы үшбұрыштар
        // =================================================

        for (int i = 0; i < count; i++)
        {
            float x =
                startX + i * width;

            // =================================================
            // Жоғарғы нүкте
            // =================================================

            vertices.push_back(
                x + width / 2.0f
            );

            vertices.push_back(y);
            vertices.push_back(0.0f);

            // =================================================
            // Сол жақ төменгі нүкте
            // =================================================

            vertices.push_back(x);
            vertices.push_back(
                y - height
            );
            vertices.push_back(0.0f);

            // =================================================
            // Оң жақ төменгі нүкте
            // =================================================

            vertices.push_back(
                x + width
            );

            vertices.push_back(
                y - height
            );

            vertices.push_back(0.0f);
        }
    }

    // =================================================
    // VAO + VBO
    // =================================================

    unsigned int vao;
    unsigned int vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(
        GL_ARRAY_BUFFER,
        vbo
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    // =================================================
    // Position
    // =================================================

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // =================================================
    // SHADER КОМПИЛЯЦИЯСЫ
    // =================================================

    unsigned int vs =
        glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(
        vs,
        1,
        &vertexSrc,
        nullptr
    );

    glCompileShader(vs);

    unsigned int fs =
        glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(
        fs,
        1,
        &fragmentSrc,
        nullptr
    );

    glCompileShader(fs);

    // =================================================
    // Shader Program
    // =================================================

    unsigned int shader =
        glCreateProgram();

    glAttachShader(shader, vs);
    glAttachShader(shader, fs);

    glLinkProgram(shader);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // =================================================
    // Color uniform
    // =================================================

    int colorLocation =
        glGetUniformLocation(
            shader,
            "ourColor"
        );

    // =================================================
    // FPS
    // =================================================

    double lastTime = glfwGetTime();

    int frameCount = 0;

    // =================================================
    // MAIN LOOP
    // =================================================

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // =================================================
        // ФОН
        // =================================================

        if (whiteBackground)
        {
            glClearColor(
                1.0f,
                1.0f,
                1.0f,
                1.0f
            );
        }
        else
        {
            float time =
                glfwGetTime();

            float r =
                (sin(time) + 1.0f) / 2.0f;

            float g =
                (sin(time + 2.0f) + 1.0f) / 2.0f;

            float b =
                (sin(time + 4.0f) + 1.0f) / 2.0f;

            glClearColor(
                r,
                g,
                b,
                1.0f
            );
        }

        glClear(GL_COLOR_BUFFER_BIT);

        // =================================================
        // WIREFRAME
        // =================================================

        if (wireframeMode)
        {
            glPolygonMode(
                GL_FRONT_AND_BACK,
                GL_LINE
            );
        }
        else
        {
            glPolygonMode(
                GL_FRONT_AND_BACK,
                GL_FILL
            );
        }

        // =================================================
        // SHADER
        // =================================================

        glUseProgram(shader);

        // =================================================
        // ТҮС
        // =================================================

        if (changeColor)
        {
            glUniform3f(
                colorLocation,
                1.0f,
                0.2f,
                0.2f
            );
        }
        else
        {
            glUniform3f(
                colorLocation,
                0.2f,
                0.8f,
                1.0f
            );
        }

        // =================================================
        // СУРЕТ САЛУ
        // =================================================

        glBindVertexArray(vao);

        int vertexCount =
            vertices.size() / 3;

        if (lineLoopMode)
        {
            // Әр үшбұрыш жеке контур

            for (
                int i = 0;
                i < vertexCount;
                i += 3
            )
            {
                glDrawArrays(
                    GL_LINE_LOOP,
                    i,
                    3
                );
            }
        }
        else
        {
            // Толық үшбұрыштар

            glDrawArrays(
                GL_TRIANGLES,
                0,
                vertexCount
            );
        }

        glBindVertexArray(0);

        // =================================================
        // FPS
        // =================================================

        frameCount++;

        double currentTime =
            glfwGetTime();

        if (currentTime - lastTime >= 1.0)
        {
            std::cout
                << "FPS: "
                << frameCount
                << std::endl;

            frameCount = 0;
            lastTime = currentTime;
        }

        // =================================================
        // UPDATE
        // =================================================

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // =================================================
    // CLEANUP
    // =================================================

    glDeleteVertexArrays(
        1,
        &vao
    );

    glDeleteBuffers(
        1,
        &vbo
    );

    glDeleteProgram(shader);

    glfwTerminate();

    return 0;
}