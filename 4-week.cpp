// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//  4-АПТА — Uniform, уақыт және delta time
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
            std::ofstream report("README-week04-error.md");
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
        "Компьютерлік графика — 4 апта",
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
    // Бір үшбұрыш — төбесі жоғары қарап тұр
    // Барлығы 3 vertex: әрқайсысында позиция + түс
    // -----------------------------------------------------------------
    float vertices[] =
    {
        // Позиция (x, y, z)       Түс (r, g, b)
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.45f, -0.25f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.45f, -0.25f, 0.0f,  0.0f, 0.0f, 1.0f
    };
    static_assert(sizeof(vertices) / sizeof(float) == 3 * 6,
                  "Triangle must contain 3 vertices with 6 floats each");
    

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

        // 2 — барлық төбелерге бірдей түс; 5 — сол түсті өзгерту.
        static int previousColorMode = -1;
        int colorMode = sameColor ? (changeColor ? 2 : 1) : 0;
        if (colorMode != previousColorMode)
        {
            for (int i = 0; i < 3; ++i)
            {
                vertices[i * 6 + 3] = sameColor ? (changeColor ? 1.0f : 0.2f) : (i % 3 == 0 ? 1.0f : 0.0f);
                vertices[i * 6 + 4] = sameColor ? (changeColor ? 0.2f : 0.8f) : (i % 3 == 1 ? 1.0f : 0.0f);
                vertices[i * 6 + 5] = sameColor ? (changeColor ? 0.2f : 1.0f) : (i % 3 == 2 ? 1.0f : 0.0f);
            }
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            previousColorMode = colorMode;
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

        }
        else
        {
            // Қалыпты режим
            glDrawArrays(
                GL_TRIANGLES,
                0,
                3
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
