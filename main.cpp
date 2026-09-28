// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//  2-АПТА — Үшбұрыш (VBO + VAO)
// =====================================================================

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

const int WIDTH = 1280;
const int HEIGHT = 720;

bool whiteBackground = false;
bool changeColor = false;
bool lineLoopMode = false;

// ---------------------------------------------------------------------
// Vertex Shader
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

// ---------------------------------------------------------------------
// Fragment Shader
// ---------------------------------------------------------------------
const char* fragmentSrc = R"(
#version 330 core

out vec4 FragColor;

uniform vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)";

// ---------------------------------------------------------------------
// Терезе өлшемі өзгергенде
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
// Пернетақта
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    // ESC — шығу
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // SPACE — ақ фон
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        whiteBackground = true;
    }
    else
    {
        whiteBackground = false;
    }

    // -------------------------------------------------------------
    // 5 — түс өзгерту
    // -------------------------------------------------------------
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

    // -------------------------------------------------------------
    // 6 — GL_LINE_LOOP
    // -------------------------------------------------------------
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
}

// =====================================================================
// MAIN
// =====================================================================
int main()
{
    // -----------------------------------------------------------------
    // GLFW
    // -----------------------------------------------------------------
    if (!glfwInit())
    {
        std::cerr << "GLFW іске қосылмады\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    // -----------------------------------------------------------------
    // Терезе
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Компьютерлік графика — 2 апта",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "Терезе жасалмады\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, onResize);

    // -----------------------------------------------------------------
    // GLAD
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
        std::cerr << "GLAD жүктелмеді\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: "
              << glGetString(GL_VERSION)
              << "\n";

    std::cout << "GPU: "
              << glGetString(GL_RENDERER)
              << "\n";

    // -----------------------------------------------------------------
    // VSync өшіру
    // -----------------------------------------------------------------
    glfwSwapInterval(0);

    // -----------------------------------------------------------------
    // FPS
    // -----------------------------------------------------------------
    double fpsTimer = glfwGetTime();
    int frameCount = 0;

    // =================================================================
    // 2-АПТА — VBO + VAO
    // =================================================================

    // -----------------------------------------------------------------
    // 2 үшбұрыш
    // Барлығы 6 vertex
    // -----------------------------------------------------------------
    float vertices[] =
    {
        // 1-ші үшбұрыш — жоғары қарап тұр
         0.0f,  0.7f, 0.0f,
        -0.6f, -0.35f, 0.0f,
         0.6f, -0.35f, 0.0f,

        // 2-ші үшбұрыш — төмен қарап тұр
         0.0f, -0.7f, 0.0f,
        -0.6f,  0.35f, 0.0f,
         0.6f,  0.35f, 0.0f
    };
    

    // -----------------------------------------------------------------
    // VAO және VBO
    // -----------------------------------------------------------------
    unsigned int vao;
    unsigned int vbo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    // VAO
    glBindVertexArray(vao);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Деректерді GPU-ға жіберу
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // -----------------------------------------------------------------
    // Vertex атрибуты
    // -----------------------------------------------------------------
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

    // =================================================================
    // Vertex Shader компиляциясы
    // =================================================================

    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(
        vs,
        1,
        &vertexSrc,
        nullptr
    );

    glCompileShader(vs);

    // =================================================================
    // Fragment Shader компиляциясы
    // =================================================================

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(
        fs,
        1,
        &fragmentSrc,
        nullptr
    );

    glCompileShader(fs);

    // =================================================================
    // Shader Program
    // =================================================================

    unsigned int shader = glCreateProgram();

    glAttachShader(shader, vs);
    glAttachShader(shader, fs);

    glLinkProgram(shader);

    glDeleteShader(vs);
    glDeleteShader(fs);

    // -----------------------------------------------------------------
    // Color uniform location
    // -----------------------------------------------------------------
    int colorLocation =
        glGetUniformLocation(shader, "ourColor");

    // =================================================================
    // Негізгі цикл
    // =================================================================

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // -----------------------------------------------------------------
        // Фон
        // -----------------------------------------------------------------

        float t = (float)glfwGetTime();

        float r =
            (std::sin(t * 2.0f) + 1.0f)
            * 0.5f * 0.3f;

        float g =
            (std::sin(t * 1.5f) + 1.0f)
            * 0.5f * 0.3f;

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
            glClearColor(
                r,
                g,
                0.35f,
                1.0f
            );
        }

        glClear(GL_COLOR_BUFFER_BIT);

        // -----------------------------------------------------------------
        // Shader
        // -----------------------------------------------------------------

        glUseProgram(shader);

        // -----------------------------------------------------------------
        // 5 — түс өзгерту
        // -----------------------------------------------------------------

        if (changeColor)
        {
            // Қызыл
            glUniform3f(
                colorLocation,
                1.0f,
                0.2f,
                0.2f
            );
        }
        else
        {
            // Көгілдір
            glUniform3f(
                colorLocation,
                0.2f,
                0.8f,
                1.0f
            );
        }

        // -----------------------------------------------------------------
        // VAO
        // -----------------------------------------------------------------

        glBindVertexArray(vao);

        // -----------------------------------------------------------------
        // 6 — GL_LINE_LOOP
        // -----------------------------------------------------------------

        if (lineLoopMode)
        {
            // Бірінші үшбұрыш
            glDrawArrays(
                GL_LINE_LOOP,
                0,
                3
            );

            // Екінші үшбұрыш
            glDrawArrays(
                GL_LINE_LOOP,
                3,
                3
            );
        }
        else
        {
            // Қалыпты режим
            glDrawArrays(
                GL_TRIANGLES,
                0,
                6
            );
        }

        // -----------------------------------------------------------------
        // FPS
        // -----------------------------------------------------------------

        frameCount++;

        double currentTime = glfwGetTime();

        if (currentTime - fpsTimer >= 1.0)
        {
            std::cout
                << "FPS: "
                << frameCount
                << std::endl;

            frameCount = 0;
            fpsTimer = currentTime;
        }

        // -----------------------------------------------------------------
        // Экран
        // -----------------------------------------------------------------

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // =================================================================
    // Тазалау
    // =================================================================

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);

    glfwTerminate();

    return 0;
}