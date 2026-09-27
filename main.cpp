// =====================================================================
//  КОМПЬЮТЕРЛІК ГРАФИКА — бір файлдық жоба
//  1-АПТА — терезе ашу және негізгі басқару
// =====================================================================

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>

// ---------------------------------------------------------------------
//  Баптаулар
// ---------------------------------------------------------------------
const int WIDTH  = 1280;
const int HEIGHT = 720;

// ---------------------------------------------------------------------
//  3. Пробел басылғанын сақтау үшін жаһандық айнымалы
// ---------------------------------------------------------------------
bool whiteBackground = false;

// ---------------------------------------------------------------------
//  Шейдерді файлдан жүктеу класы
// ---------------------------------------------------------------------
class Shader {
public:

    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {

        std::string vertexCode;
        std::string fragmentCode;

        std::ifstream vertexFile(vertexPath);
        std::ifstream fragmentFile(fragmentPath);

        if (!vertexFile.is_open()) {
            std::cerr << "Vertex shader файлы ашылмады: "
                      << vertexPath << "\n";
        }

        if (!fragmentFile.is_open()) {
            std::cerr << "Fragment shader файлы ашылмады: "
                      << fragmentPath << "\n";
        }

        std::stringstream vertexStream;
        std::stringstream fragmentStream;

        vertexStream << vertexFile.rdbuf();
        fragmentStream << fragmentFile.rdbuf();

        vertexCode = vertexStream.str();
        fragmentCode = fragmentStream.str();

        const char* vertexSource = vertexCode.c_str();
        const char* fragmentSource = fragmentCode.c_str();

        // Vertex shader
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);

        glShaderSource(
            vertexShader,
            1,
            &vertexSource,
            nullptr
        );

        glCompileShader(vertexShader);

        // Fragment shader
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

        glShaderSource(
            fragmentShader,
            1,
            &fragmentSource,
            nullptr
        );

        glCompileShader(fragmentShader);

        // Shader program
        ID = glCreateProgram();

        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);

        glLinkProgram(ID);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void use() {
        glUseProgram(ID);
    }
};

// ---------------------------------------------------------------------
//  Терезе өлшемі өзгергенде шақырылады
// ---------------------------------------------------------------------
void onResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// ---------------------------------------------------------------------
//  Пернетақтаны тексеру
// ---------------------------------------------------------------------
void processInput(GLFWwindow* window) {

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        whiteBackground = true;
    }
    else {
        whiteBackground = false;
    }
}

// =====================================================================
//  MAIN
// =====================================================================
int main() {

    // -----------------------------------------------------------------
    //  1. GLFW-ны іске қосу
    // -----------------------------------------------------------------
    if (!glfwInit()) {
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
    //  2. Терезе жасау
    // -----------------------------------------------------------------
    GLFWwindow* window = glfwCreateWindow(
        WIDTH,
        HEIGHT,
        "Компьютерлік графика",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Терезе жасалмады. Видеокарта OpenGL 3.3-ті "
                     "қолдамауы мүмкін.\n";

        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, onResize);

    // -----------------------------------------------------------------
    //  4. VSync өшіру
    // -----------------------------------------------------------------
    glfwSwapInterval(0);

    // -----------------------------------------------------------------
    //  3. GLAD
    // -----------------------------------------------------------------
    if (gladLoadGL(glfwGetProcAddress) == 0) {

        std::cerr << "GLAD жүктелмеді\n";

        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU:    " << glGetString(GL_RENDERER) << "\n";

    // -----------------------------------------------------------------
    //  FPS есептеу үшін айнымалылар
    // -----------------------------------------------------------------
    double fpsTimer = glfwGetTime();
    int frameCount = 0;

    // === 2-АПТА: үшбұрыштың деректері мен буферлері ===

    // Позиция + түс
    float vertices[] = {

        // x      y       r    g    b
         0.0f,   0.5f,   1.0f, 0.0f, 0.0f,   // Жоғарғы - қызыл
        -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,   // Сол жақ - жасыл
         0.5f,  -0.5f,   0.0f, 0.0f, 1.0f    // Оң жақ - көк
    };

    unsigned int VBO, VAO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // -----------------------------------------------------------------
    //  Позиция атрибуты
    // -----------------------------------------------------------------

    glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    // -----------------------------------------------------------------
    //  Түс атрибуты
    // -----------------------------------------------------------------

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        (void*)(2 * sizeof(float))
    );

    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // === 3-АПТА: шейдерді файлдан жүктеу ===

    Shader shader(
        "vertex.glsl",
        "fragment.glsl"
    );

    // -----------------------------------------------------------------
    //  4. Негізгі цикл
    // -----------------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {

        // Пернетақтаны тексеру
        processInput(window);

        // -------------------------------------------------------------
        //  Фон түсін есептеу
        // -------------------------------------------------------------
        float t = (float)glfwGetTime();

        float r =
            (std::sin(t * 2.0f) + 1.0f)
            * 0.5f * 0.3f;

        float g =
            (std::sin(t * 1.5f) + 1.0f)
            * 0.5f * 0.3f;

        // -------------------------------------------------------------
        //  Пробел басылса — ақ фон
        // -------------------------------------------------------------
        if (whiteBackground) {
            glClearColor(
                1.0f,
                1.0f,
                1.0f,
                1.0f
            );
        }
        else {
            glClearColor(
                r,
                g,
                0.35f,
                1.0f
            );
        }

        // Экранды тазалау
        glClear(GL_COLOR_BUFFER_BIT);

        // -------------------------------------------------------------
        //  Үшбұрышты салу
        // -------------------------------------------------------------

        shader.use();

        glBindVertexArray(VAO);

        glDrawArrays(
            GL_TRIANGLES,
            0,
            3
        );

        // -------------------------------------------------------------
        //  FPS есептеу
        // -------------------------------------------------------------
        frameCount++;

        double currentTime = glfwGetTime();

        if (currentTime - fpsTimer >= 1.0) {

            std::cout
                << "FPS: "
                << frameCount
                << std::endl;

            frameCount = 0;
            fpsTimer = currentTime;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // -----------------------------------------------------------------
    //  5. Тазалау
    // -----------------------------------------------------------------

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glDeleteProgram(shader.ID);

    glfwTerminate();

    return 0;
}