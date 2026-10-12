// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//  5-АПТА — Индекс буфері (EBO), примитив түрлері
// =====================================================================

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>

const int WIDTH = 1280;
const int HEIGHT = 720;

bool whiteBackground = false;
bool changeColor = false;
bool lineLoopMode = false;
bool invertColor = false;
bool sameColor = false;
bool checkerboard = false;
float speed = 1.5f; // радиан/секунд
float angle = 0.0f;
bool frameDependent = false; // F — dt жоқ эксперимент, бастапқыда өшірулі
int geometryMode = 0; // 0: төртбұрыш, 1: бір үшбұрыш, 2: бесбұрыш, 3: екі объект
bool wireframe = false;

// ---------------------------------------------------------------------
// Vertex Shader
// ---------------------------------------------------------------------
const char* vertexSrc = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform vec2 uOffset;
uniform float uScale;

void main()
{
    gl_Position = vec4(aPos.xy * uScale + uOffset, aPos.z, 1.0);
    vColor = aColor;
}
)";

// ---------------------------------------------------------------------
// Fragment Shader
// ---------------------------------------------------------------------
const char* fragmentSrc = R"(
#version 330 core

out vec4 FragColor;

in vec3 vColor;
uniform bool invertColor;
uniform bool checkerboard;

void main()
{
    vec3 color = invertColor ? 1.0 - vColor : vColor;
    if (checkerboard)
    {
        vec2 cell = floor(gl_FragCoord.xy / 32.0);
        float shade = step(1.0, mod(cell.x + cell.y, 2.0));
        color *= mix(0.35, 1.0, shade);
    }
    FragColor = vec4(color, 1.0);
}
)";

// Компиляция және линковка қателерін тексеру.
unsigned int makeShader(const char* vertex, const char* fragment)
{
    unsigned int shaders[2] = {0, 0};
    const char* sources[2] = {vertex, fragment};
    const GLenum types[2] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    for (int i = 0; i < 2; ++i)
    {
        shaders[i] = glCreateShader(types[i]);
        glShaderSource(shaders[i], 1, &sources[i], nullptr);
        glCompileShader(shaders[i]);
        int ok = 0;
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            int length = 0;
            glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &length);
            std::vector<char> log(length > 0 ? length : 1, '\0');
            glGetShaderInfoLog(shaders[i], static_cast<GLsizei>(log.size()), nullptr, log.data());
            std::cerr << "Shader compilation error:\n" << log.data() << '\n';
            std::ofstream report("README-week05-error.md");
            report << "# Нақты драйвер қатесі\n\n```text\n" << log.data() << "\n```\n";
            glDeleteShader(shaders[0]);
            if (i == 1) glDeleteShader(shaders[1]);
            return 0;
        }
    }
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, shaders[0]);
    glAttachShader(prog, shaders[1]);
    glLinkProgram(prog);
    glDeleteShader(shaders[0]);
    glDeleteShader(shaders[1]);
    int ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        int length = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length > 0 ? length : 1, '\0');
        glGetProgramInfoLog(prog, static_cast<GLsizei>(log.size()), nullptr, log.data());
        std::cerr << "Program link error:\n" << log.data() << '\n';
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

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
void processInput(GLFWwindow* window, float dt)
{
    // 5-апта тапсырмалары: 0 — негізгі, 8 — бір үшбұрыш,
    // 9 — бесбұрыш, T — екі төртбұрыш.
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) geometryMode = 0;
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) geometryMode = 1;
    if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) geometryMode = 2;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) geometryMode = 3;
    static bool tabWasPressed = false;
    bool tabPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabPressed && !tabWasPressed) wireframe = !wireframe;
    tabWasPressed = tabPressed;

    // Жылдамдықтың өзгеруі де FPS-ке тәуелсіз болуы керек.
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) speed += 2.0f * dt;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) speed -= 2.0f * dt;
    speed = std::clamp(speed, 0.1f, 10.0f);

    // 1-тапсырманы салыстыру: F қайта басылса, dt қайтарылады.
    static bool keyFWasPressed = false;
    bool keyFPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
    if (keyFPressed && !keyFWasPressed)
    {
        frameDependent = !frameDependent;
        std::cout << (frameDependent ? "Experiment: 0.02 radians/frame\n"
                                    : "Normal: speed * dt, radians/second\n");
    }
    keyFWasPressed = keyFPressed;

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        speed = 1.5f;
        angle = 0.0f;
        frameDependent = false;
        invertColor = false;
        sameColor = false;
        changeColor = false;
        checkerboard = false;
        lineLoopMode = false;
        geometryMode = 0;
        wireframe = false;
    }

    // 3-апта: 1 — кері түс, 2 — бірдей түс, 4 — өрнек.
    static bool wasPressed[3] = {};
    const int keys[3] = {GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_4};
    bool* modes[3] = {&invertColor, &sameColor, &checkerboard};
    for (int i = 0; i < 3; ++i)
    {
        bool pressed = glfwGetKey(window, keys[i]) == GLFW_PRESS;
        if (pressed && !wasPressed[i]) *modes[i] = !*modes[i];
        wasPressed[i] = pressed;
    }

    // 7 — әдейі қате; негізгі шейдер өзгермейді.
    static bool key7WasPressed = false;
    bool key7Pressed = glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS;
    if (key7Pressed && !key7WasPressed)
    {
        const char* broken = "#version 330 core\nout vec4 FragColor;\nvoid main() {\nvec5 bad;\nFragColor = vec4(1.0);\n}\n";
        unsigned int test = makeShader(vertexSrc, broken);
        if (test) glDeleteProgram(test);
    }
    key7WasPressed = key7Pressed;

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
        "Компьютерлік графика — 5 апта",
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
    // Төртбұрыш: 4 vertex, әрқайсысында позиция + түс
    // -----------------------------------------------------------------
    float vertices[] =
    {
        // Позиция (x, y, z)       Түс (r, g, b)
         0.3f,  0.3f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.3f, -0.3f, 0.0f,  0.0f, 1.0f, 0.0f,
        -0.3f, -0.3f, 0.0f,  0.0f, 0.0f, 1.0f,
        -0.3f,  0.3f, 0.0f,  1.0f, 1.0f, 0.0f
    };
    static_assert(sizeof(vertices) / sizeof(float) == 4 * 6,
                  "Rectangle must contain 4 vertices with 6 floats each");
    unsigned int indices[] = {0, 1, 3, 1, 2, 3};
    unsigned int triangleIndices[] = {0, 1, 2};
    unsigned int pentagonIndices[] = {0, 1, 2, 0, 2, 3, 0, 3, 4};
    float pentagonVertices[5 * 6];
    const float colors[5][3] = {
        {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}
    };
    for (int i = 0; i < 5; ++i)
    {
        float vertexAngle = (-90.0f + 72.0f * i) * 3.14159265359f / 180.0f;
        pentagonVertices[i * 6] = std::cos(vertexAngle) * 0.3f;
        pentagonVertices[i * 6 + 1] = std::sin(vertexAngle) * 0.3f;
        pentagonVertices[i * 6 + 2] = 0.0f;
        for (int c = 0; c < 3; ++c) pentagonVertices[i * 6 + 3 + c] = colors[i][c];
    }
    

    // -----------------------------------------------------------------
    // VAO және VBO
    // -----------------------------------------------------------------
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    // VAO
    glBindVertexArray(vao);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Деректерді GPU-ға жіберу
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_DYNAMIC_DRAW
    );

    // EBO байлауы осы VAO ішінде сақталады.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    // -----------------------------------------------------------------
    // Vertex атрибуты
    // -----------------------------------------------------------------
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    unsigned int shader = makeShader(vertexSrc, fragmentSrc);
    if (!shader)
    {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    int invertLocation = glGetUniformLocation(shader, "invertColor");
    int checkerLocation = glGetUniformLocation(shader, "checkerboard");
    int locOffset = glGetUniformLocation(shader, "uOffset");
    int locScale = glGetUniformLocation(shader, "uScale");
    float lastFrame = (float)glfwGetTime(); // нөлден басталмайды

    // =================================================================
    // Негізгі цикл
    // =================================================================

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - lastFrame;
        lastFrame = now;
        processInput(window, dt);
        if (frameDependent)
            angle += 0.02f; // тек F эксперименті
        else
            angle += speed * dt;
        angle = std::fmod(angle, 6.28318530718f);

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

        glUniform2f(locOffset, std::cos(angle) * 0.4f, std::sin(angle) * 0.4f);
        // sin: [-1, 1] -> scale: [0.5, 1.0]
        float scale = 0.75f + 0.25f * std::sin(now * 2.0f);
        glUniform1f(locScale, scale);

        glUniform1i(invertLocation, invertColor);
        glUniform1i(checkerLocation, checkerboard);

        // Геометрия режимі ауысқанда ғана VBO/EBO жаңартылады.
        glBindVertexArray(vao);
        static int previousGeometryMode = -1;
        bool geometryChanged = geometryMode != previousGeometryMode;
        bool pentagon = geometryMode == 2;
        float* activeVertices = pentagon ? pentagonVertices : vertices;
        int vertexCount = pentagon ? 5 : 4;
        GLsizei indexCount = pentagon ? 9 : (geometryMode == 1 ? 3 : 6);
        if (geometryChanged)
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertexCount * 6 * sizeof(float), activeVertices, GL_DYNAMIC_DRAW);
            const unsigned int* activeIndices = pentagon ? pentagonIndices :
                (geometryMode == 1 ? triangleIndices : indices);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), activeIndices, GL_DYNAMIC_DRAW);
            previousGeometryMode = geometryMode;
        }

        // 2 — барлық төбелерге бірдей түс; 5 — сол түсті өзгерту.
        static int previousColorMode = -1;
        int colorMode = sameColor ? (changeColor ? 2 : 1) : 0;
        if (colorMode != previousColorMode || geometryChanged)
        {
            for (int i = 0; i < vertexCount; ++i)
            {
                activeVertices[i * 6 + 3] = sameColor ? (changeColor ? 1.0f : 0.2f) : colors[i][0];
                activeVertices[i * 6 + 4] = sameColor ? (changeColor ? 0.2f : 0.8f) : colors[i][1];
                activeVertices[i * 6 + 5] = sameColor ? (changeColor ? 0.2f : 1.0f) : colors[i][2];
            }
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * 6 * sizeof(float), activeVertices);
            previousColorMode = colorMode;
        }

        // -----------------------------------------------------------------
        // VAO
        // -----------------------------------------------------------------

        glBindVertexArray(vao);

        // -----------------------------------------------------------------
        // 6 — GL_LINE_LOOP
        // -----------------------------------------------------------------

        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        GLenum primitive = lineLoopMode ? GL_LINE_LOOP : GL_TRIANGLES;
        if (geometryMode == 3)
        {
            // Бір VAO, бір геометрия, екі түрлі uniform.
            glUniform2f(locOffset, -0.4f, 0.0f);
            glDrawElements(primitive, indexCount, GL_UNSIGNED_INT, nullptr);
            glUniform2f(locOffset, 0.4f, 0.0f);
            glDrawElements(primitive, indexCount, GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            glDrawElements(primitive, indexCount, GL_UNSIGNED_INT, nullptr);
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
    glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader);

    glfwTerminate();

    return 0;
}
