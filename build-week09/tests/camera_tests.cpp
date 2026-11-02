// Нақты main.cpp функцияларын терезе ашпай тексеру.
#define main week09_application_main
#include "../main.cpp"
#undef main

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(glm::vec3 a, glm::vec3 b) { return glm::length(a - b) < 0.0002f; }
bool finiteMatrix(const glm::mat4& m) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (!std::isfinite(m[c][r])) return false;
    return true;
}
int main() {
    try {
        // Бір секунд жүру: 30 және 144 FPS кезінде бірдей қашықтық.
        for (int fps : {30, 144}) {
            resetCamera();
            for (int i = 0; i < fps; ++i)
                moveCamera(1.0f / fps, true,false,false,false,false,false,false);
            require(near(camPos, {0,0,.5f}), "Forward movement depends on FPS");
        }
        resetCamera();
        moveCamera(1, true,false,false,false,false,false,true);
        require(near(camPos, {0,0,-2}), "Ctrl must double speed");
        resetCamera();
        moveCamera(1, false,true,false,false,false,false,false);
        require(near(camPos, {0,0,5.5f}), "S direction");
        resetCamera();
        moveCamera(1, false,false,true,false,false,false,false);
        require(near(camPos, {-2.5f,0,3}), "A must move left");
        resetCamera();
        moveCamera(1, false,false,false,true,false,false,false);
        require(near(camPos, {2.5f,0,3}), "D must move right");
        resetCamera();
        moveCamera(1, false,false,false,false,true,false,false);
        require(near(camPos, {0,2.5f,3}), "Space must move up");
        moveCamera(1, false,false,false,false,false,true,false);
        require(near(camPos, {0,0,3}), "Shift must move down");
        // Нөлдік dt, қарама-қарсы пернелер және камераның бұрылған бағыты.
        moveCamera(0, true,false,false,false,false,false,true);
        require(near(camPos, {0,0,3}), "Zero dt should not move camera");
        moveCamera(1, true,true,true,true,true,true,false);
        require(near(camPos, {0,0,3}), "Opposite keys should cancel");
        camFront = glm::normalize(glm::vec3(0,1,-1));
        moveCamera(1, false,false,false,true,false,false,false);
        require(near(camPos, {2.5f,0,3}), "Strafe vector must be normalized");
        resetCamera();
        camFront = {1,0,0};
        moveCamera(1, false,false,false,true,false,false,false);
        require(near(camPos, {0,0,5.5f}), "Strafe must follow camera orientation");
        resetCamera();
        moveCamera(100, false,true,false,false,false,false,false);
        require(std::abs(glm::length(camPos) - 20) < .0002f, "Radius limit");
        resetCamera();
        camPos = {2,1,3};
        const glm::mat4 view = cameraView();
        require(near(glm::vec3(view * glm::vec4(camPos,1)), {0,0,0}), "View must map eye to origin");
        require(near(glm::vec3(view * glm::vec4(camPos + camFront,1)), {0,0,-1}), "View target must follow camera");
        onKey(nullptr, GLFW_KEY_O, 0, GLFW_PRESS, 0);
        const glm::mat4 originView = cameraView();
        require(std::abs(originView[0][0] - view[0][0]) > .01f, "Comparison mode must change viewing direction");
        for (glm::vec3 eye : {glm::vec3(0,0,0), glm::vec3(0,3,0)}) {
            camPos = eye;
            require(finiteMatrix(cameraView()), "Degenerate comparison view");
        }
        onKey(nullptr, GLFW_KEY_R, 0, GLFW_PRESS, 0);
        require(near(camPos, {0,0,3}) && !compareLookAt, "R must restore default camera and mode");
        onKey(nullptr, GLFW_KEY_0, 0, GLFW_PRESS, 0);
        require(cubeCount == 10, "Ten-cube scene");
        onKey(nullptr, GLFW_KEY_5, 0, GLFW_PRESS, 0);
        require(cubeCount == 5, "Five-cube scene");
        std::cout << "PASS: FPS, sprint, WASD, vertical movement, strafe, radius, LookAt, reset, scene modes\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
    return 0;
}
