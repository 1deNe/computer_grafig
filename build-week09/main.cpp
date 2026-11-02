// 9-апта: камера, LookAt, WASD және delta time.
// Бұрынғы апталардағы GLFW + GLAD, GLM, VAO/VBO/EBO құрылымы сақталды.
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

const int WIDTH = 1280, HEIGHT = 720;
glm::vec3 camPos(0.0f, 0.0f, 3.0f);
glm::vec3 camFront(0.0f, 0.0f, -1.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);
bool compareLookAt = false;
bool wireframe = false;
int cubeCount = 5;

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vColor = aColor;
}
)";
const char* fragmentSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

// Әр беттің өз түсі бар: тереңдік пен камераның қозғалысы анық көрінеді.
const float vertices[] = {
    // Артқы бет
    -.5f,-.5f,-.5f, .25f,.40f,.85f,  .5f,-.5f,-.5f, .25f,.40f,.85f,
     .5f, .5f,-.5f, .25f,.40f,.85f, -.5f, .5f,-.5f, .25f,.40f,.85f,
    // Алдыңғы бет
    -.5f,-.5f, .5f, .95f,.55f,.20f,  .5f,-.5f, .5f, .95f,.55f,.20f,
     .5f, .5f, .5f, .95f,.55f,.20f, -.5f, .5f, .5f, .95f,.55f,.20f,
    // Сол жақ
    -.5f,-.5f,-.5f, .20f,.65f,.65f, -.5f,-.5f, .5f, .20f,.65f,.65f,
    -.5f, .5f, .5f, .20f,.65f,.65f, -.5f, .5f,-.5f, .20f,.65f,.65f,
    // Оң жақ
     .5f,-.5f,-.5f, .75f,.30f,.45f,  .5f,-.5f, .5f, .75f,.30f,.45f,
     .5f, .5f, .5f, .75f,.30f,.45f,  .5f, .5f,-.5f, .75f,.30f,.45f,
    // Төменгі бет
    -.5f,-.5f,-.5f, .40f,.30f,.65f,  .5f,-.5f,-.5f, .40f,.30f,.65f,
     .5f,-.5f, .5f, .40f,.30f,.65f, -.5f,-.5f, .5f, .40f,.30f,.65f,
    // Жоғарғы бет
    -.5f, .5f,-.5f, .45f,.80f,.45f,  .5f, .5f,-.5f, .45f,.80f,.45f,
     .5f, .5f, .5f, .45f,.80f,.45f, -.5f, .5f, .5f, .45f,.80f,.45f
};
const unsigned int indices[] = {
     0, 1, 2,  2, 3, 0,    4, 5, 6,  6, 7, 4,
     8, 9,10, 10,11, 8,   12,13,14, 14,15,12,
    16,17,18, 18,19,16,   20,21,22, 22,23,20
};
// Бірінші бес куб: z = 0 ... -4. Кеңейтілген сахна: z = 0 ... -15.
const glm::vec3 cubePositions[] = {
    {0.0f,0.0f,0.0f}, {-1.7f,0.0f,-1.0f}, {1.7f,0.0f,-2.0f},
    {-1.7f,0.0f,-3.0f}, {1.7f,0.0f,-4.0f}, {-2.0f,0.0f,-6.0f},
    {1.8f,0.0f,-8.0f}, {-1.8f,0.0f,-10.0f}, {2.0f,0.0f,-12.0f},
    {0.0f,0.0f,-15.0f}
};

unsigned int makeShader(const char* vertex, const char* fragment) {
    unsigned int shaders[2] = {};
    const char* sources[] = {vertex, fragment};
    const GLenum types[] = {GL_VERTEX_SHADER, GL_FRAGMENT_SHADER};
    for (int i = 0; i < 2; ++i) {
        shaders[i] = glCreateShader(types[i]);
        glShaderSource(shaders[i], 1, &sources[i], nullptr);
        glCompileShader(shaders[i]);
        int ok = 0;
        glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[4096] = {};
            glGetShaderInfoLog(shaders[i], sizeof(log), nullptr, log);
            std::cerr << "Shader compilation error:\n" << log << '\n';
            for (auto id : shaders) if (id) glDeleteShader(id);
            return 0;
        }
    }
    unsigned int program = glCreateProgram();
    for (auto id : shaders) glAttachShader(program, id);
    glLinkProgram(program);
    for (auto id : shaders) glDeleteShader(id);
    int ok = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096] = {};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Program link error:\n" << log << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void resetCamera() {
    camPos = glm::vec3(0.0f, 0.0f, 3.0f);
    camFront = glm::vec3(0.0f, 0.0f, -1.0f);
    camUp = glm::vec3(0.0f, 1.0f, 0.0f);
    compareLookAt = false;
}

// Пернетақтадан бөлек функция: dt және бағыттарды жеке тексеруге болады.
void moveCamera(float dt, bool w, bool s, bool a, bool d,
                bool up, bool down, bool sprint) {
    float speed = 2.5f * dt;
    if (sprint) speed *= 2.0f;
    const glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
    if (w) camPos += speed * camFront;
    if (s) camPos -= speed * camFront;
    if (a) camPos -= speed * right;
    if (d) camPos += speed * right;
    // Әлемнің Y осін таңдадық: жоғары әрқашан әлемнің жоғарысы болады.
    if (up) camPos += speed * glm::vec3(0.0f, 1.0f, 0.0f);
    if (down) camPos -= speed * glm::vec3(0.0f, 1.0f, 0.0f);
    const float distance = glm::length(camPos);
    if (distance > 20.0f) camPos *= 20.0f / distance;
}

void processInput(GLFWwindow* window, float dt) {
    const auto held = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
    if (held(GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window, GLFW_TRUE);
    moveCamera(dt, held(GLFW_KEY_W), held(GLFW_KEY_S), held(GLFW_KEY_A), held(GLFW_KEY_D),
               held(GLFW_KEY_SPACE), held(GLFW_KEY_LEFT_SHIFT), held(GLFW_KEY_LEFT_CONTROL));
}

// Бір рет басылатын әрекеттер: P консольді әр кадрда толтырмайды.
void onKey(GLFWwindow*, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_R) resetCamera();
    if (key == GLFW_KEY_P)
        std::cout << std::fixed << std::setprecision(3) << "camPos = ("
                  << camPos.x << ", " << camPos.y << ", " << camPos.z << ")\n";
    if (key == GLFW_KEY_O) {
        compareLookAt = !compareLookAt;
        std::cout << "LookAt target: " << (compareLookAt ? "origin (comparison)" : "camPos + camFront") << '\n';
    }
    if (key == GLFW_KEY_5) cubeCount = 5;
    if (key == GLFW_KEY_0) cubeCount = 10;
    if (key == GLFW_KEY_TAB) wireframe = !wireframe;
}

glm::mat4 cameraView() {
    if (compareLookAt) {
        // Тек 1-тапсырмадағы салыстыру үшін. R негізгі дұрыс режимді қайтарады.
        // eye == target немесе up-пен параллель бағыт LookAt-ты бұзады.
        glm::vec3 direction = -camPos;
        if (glm::length(direction) < 0.0001f) direction = camFront;
        glm::vec3 up = camUp;
        if (glm::length(glm::cross(direction, up)) < 0.0001f)
            up = glm::vec3(0.0f, 0.0f, 1.0f);
        return glm::lookAt(camPos, camPos + direction, up);
    }
    return glm::lookAt(camPos, camPos + camFront, camUp);
}

void onResize(GLFWwindow*, int width, int height) { glViewport(0, 0, width, height); }

// PPM скриншот: графиканы тексеру үшін --capture preview.ppm.
void captureFrame(const std::string& path, int width, int height) {
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot save screenshot: " + path);
    file << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        file.write(reinterpret_cast<const char*>(pixels.data() + static_cast<size_t>(y) * width * 3), width * 3);
    if (!file) throw std::runtime_error("Screenshot write failed: " + path);
}

int main(int argc, char** argv) {
    int frameLimit = 0;
    std::string capturePath;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--frames" && i + 1 < argc) {
                frameLimit = std::stoi(argv[++i]);
                if (frameLimit <= 0) throw std::runtime_error("--frames must be positive");
            } else if (arg == "--capture" && i + 1 < argc) capturePath = argv[++i];
            else if (arg == "--ten-cubes") cubeCount = 10;
            else throw std::runtime_error("Usage: week09 [--frames N] [--capture image.ppm] [--ten-cubes]");
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
    glfwSetErrorCallback([](int code, const char* text) { std::cerr << "GLFW " << code << ": " << text << '\n'; });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Week 09 | Camera / LookAt", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::cerr << "GLAD initialization failed\n";
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSetKeyCallback(window, onKey);
    glEnable(GL_DEPTH_TEST);
    const unsigned int shader = makeShader(vertexSrc, fragmentSrc);
    if (!shader) { glfwDestroyWindow(window); glfwTerminate(); return 1; }
    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo); glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    const int modelLoc = glGetUniformLocation(shader, "model");
    const int viewLoc = glGetUniformLocation(shader, "view");
    const int projectionLoc = glGetUniformLocation(shader, "projection");
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n'
              << "WASD move | Space/Left Shift up/down | Left Ctrl sprint\n"
              << "O compare LookAt | R reset | P position | 5/0 five/ten cubes | Tab wireframe | Esc exit\n";
    double previous = glfwGetTime(), fpsTimer = previous;
    int frameCount = 0, totalFrames = 0, result = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - previous);
        previous = now;
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0 || height <= 0) { glfwWaitEventsTimeout(0.05); continue; }
        processInput(window, dt);
        glViewport(0, 0, width, height);
        glClearColor(0.065f, 0.085f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);
        // View әр кадрда камераның ЖАҢА орнымен қайта есептеледі.
        const glm::mat4 view = cameraView();
        const glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            static_cast<float>(width) / height, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        glBindVertexArray(vao);
        for (int i = 0; i < cubeCount; ++i) {
            // Модель статикалық: камера қозғалғанда кубтардың орны өзгермейді.
            glm::mat4 model = glm::translate(glm::mat4(1.0f), cubePositions[i]);
            model = glm::rotate(model, glm::radians(12.0f * i), glm::normalize(glm::vec3(1.0f, 0.3f, 0.5f)));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        }
        if (!capturePath.empty()) {
            try { captureFrame(capturePath, width, height); capturePath.clear(); }
            catch (const std::exception& e) { std::cerr << e.what() << '\n'; result = 1; break; }
        }
        if (const auto error = glGetError(); error != GL_NO_ERROR) {
            std::cerr << "OpenGL error: " << error << '\n'; result = 1; break;
        }
        glfwSwapBuffers(window);
        ++frameCount; ++totalFrames;
        if (now - fpsTimer >= 1.0) {
            const std::string title = "Week 09 | " + std::string(compareLookAt ? "LookAt: origin" : "LookAt: camPos + camFront")
                + " | cubes: " + std::to_string(cubeCount) + " | FPS: " + std::to_string(frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0; fpsTimer = now;
        }
        if (frameLimit > 0 && totalFrames >= frameLimit) break;
    }
    glDeleteVertexArrays(1, &vao); glDeleteBuffers(1, &vbo); glDeleteBuffers(1, &ebo);
    glDeleteProgram(shader);
    glfwDestroyWindow(window); glfwTerminate();
    return result;
}
