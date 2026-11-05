// =====================================================================
//  9-апта: Камера моделі, lookAt, пернетақтамен жылжу
//  Сахна: 8-аптадағы бес куб (айналып тұрады). Камера кубтардың арасында ұшады.
//
//  БАСҚАРУ
//    W / S           — алға / артқа (камера қараған бағытпен)
//    A / D           — солға / оңға (strafe)
//    Space           — жоғары          Left Shift — төмен
//    Left Ctrl       — жүгіру (жылдамдық x2)
//    1 / 2           — 1-тапсырма: 1 = дұрыс lookAt (камера ұшады)
//                                   2 = қате lookAt (камера орбитамен айналады)
//    R               — камераны бастапқы орнына қайтару
//    P               — camPos-ты консольге шығару
//    C               — екінші камера (жоғарыдан қарау) ↔ еркін камера
//    ESC             — шығу
// =====================================================================
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

int gWidth = 800, gHeight = 600;

// ---------- 1-қадам: камераның үш айнымалысы (жаһандық, цикл сыртында) ----------
const glm::vec3 CAM_START(0.0f, 0.0f, 3.0f);
glm::vec3 camPos   = CAM_START;
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);   // қарайтын бағыт (нүкте емес, БАҒЫТ!)
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

bool wrongLookAt = false;   // 1-тапсырма: true болса, lookAt-тың 2-параметрі қате (тұрақты нүкте)
bool topView     = false;   // қосымша: екінші камера

void onResize(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
    gWidth = w;
    gHeight = (h > 0) ? h : 1;
}

// Перне ЕКІ ретті басылмай, тек басылған сәтте бір рет сигнал береді
bool pressedOnce(GLFWwindow* window, int key) {
    static bool prev[GLFW_KEY_LAST + 1] = {};
    bool now = glfwGetKey(window, key) == GLFW_PRESS;
    bool fired = now && !prev[key];
    prev[key] = now;
    return fired;
}

// ---------- 2-қадам: пернетақтаны өңдеу (цикл ішінде, dt-ден кейін) ----------
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    float speed = 2.5f * dt;                                                    // dt — FPS-ке тәуелсіз
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) speed *= 2.0f; // 3-тапсырма: жүгіру

    glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));              // normalize ұмытпау!
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += right * speed;

    // 2-тапсырма: жоғары-төмен. camUp бойымен (камераның төбесі) — ұшақ сияқты.
    // Әлемнің Y осі бойымен (glm::vec3(0,1,0)) — ойындағы адам сияқты.
    // Қазір камера еңкеймейді, сондықтан екеуі бірдей. 10-аптада тінтуір қосылғанда айырмашылығы шығады.
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)      camPos += speed * camUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camPos -= speed * camUp;

    // Қосымша: камера сахнадан тым алыс кетпесін (|camPos| <= 20)
    if (glm::length(camPos) > 20.0f) camPos = glm::normalize(camPos) * 20.0f;

    if (pressedOnce(window, GLFW_KEY_1)) wrongLookAt = false;
    if (pressedOnce(window, GLFW_KEY_2)) wrongLookAt = true;
    if (pressedOnce(window, GLFW_KEY_C)) topView = !topView;
    if (pressedOnce(window, GLFW_KEY_R)) { camPos = CAM_START; camFront = glm::vec3(0, 0, -1); }
    if (pressedOnce(window, GLFW_KEY_P))
        std::cout << "camPos = " << camPos.x << ", " << camPos.y << ", " << camPos.z << "\n";
}

// ---------- шейдерлер ----------
const char* vsSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
uniform mat4 uModel, uView, uProj;
out vec3 vColor;
void main() { gl_Position = uProj * uView * uModel * vec4(aPos, 1.0); vColor = aColor; }
)";
const char* fsSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() { FragColor = vec4(vColor, 1.0); }
)";

unsigned int makeShader() {
    auto compile = [](GLenum type, const char* src) {
        unsigned int id = glCreateShader(type);
        glShaderSource(id, 1, &src, nullptr);
        glCompileShader(id);
        int ok;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok) { char log[512]; glGetShaderInfoLog(id, 512, nullptr, log); std::cerr << log << "\n"; }
        return id;
    };
    unsigned int vs = compile(GL_VERTEX_SHADER, vsSrc), fs = compile(GL_FRAGMENT_SHADER, fsSrc);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// Бірлік куб: 6 бет x 4 төбе, әр беттің өз түсі (жоғарғы төбелер ашығырақ — көлем сезіліп тұрады)
unsigned int makeCube(int& indexCount) {
    struct Face { glm::vec3 n, u, v, color; };
    const Face faces[6] = {
        {{ 1, 0, 0}, {0, 0,-1}, {0, 1, 0}, {0.95f, 0.35f, 0.35f}},
        {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0.95f, 0.65f, 0.25f}},
        {{ 0, 1, 0}, {1, 0, 0}, {0, 0,-1}, {0.95f, 0.90f, 0.40f}},
        {{ 0,-1, 0}, {1, 0, 0}, {0, 0, 1}, {0.40f, 0.85f, 0.45f}},
        {{ 0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {0.35f, 0.65f, 0.95f}},
        {{ 0, 0,-1}, {-1,0, 0}, {0, 1, 0}, {0.70f, 0.50f, 0.95f}},
    };
    std::vector<float> vert;
    std::vector<unsigned int> idx;
    for (int f = 0; f < 6; f++) {
        unsigned int base = (unsigned int)(vert.size() / 6);
        for (int k = 0; k < 4; k++) {
            float su = (k == 0 || k == 3) ? -0.5f : 0.5f, sv = (k < 2) ? -0.5f : 0.5f;
            glm::vec3 p = faces[f].n * 0.5f + faces[f].u * su + faces[f].v * sv;
            glm::vec3 c = faces[f].color * (sv > 0 ? 1.0f : 0.75f);
            vert.insert(vert.end(), { p.x, p.y, p.z, c.r, c.g, c.b });
        }
        idx.insert(idx.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }
    indexCount = (int)idx.size();
    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);                                                   // VAO бірінші
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vert.size() * sizeof(float), vert.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);                               // EBO — VAO байланып тұрғанда
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return vao;
}

int main() {
    if (!glfwInit()) { std::cerr << "GLFW iske qosylmady\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    GLFWwindow* window = glfwCreateWindow(gWidth, gHeight, "Week 9 - camera", nullptr, nullptr);
    if (!window) { std::cerr << "Tereze zhasalmady\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, onResize);
    glfwSwapInterval(1);
    if (gladLoadGL(glfwGetProcAddress) == 0) { std::cerr << "GLAD zhuktelmedi\n"; return -1; }
    std::cout << "GPU: " << glGetString(GL_RENDERER) << "\n";
    glEnable(GL_DEPTH_TEST);            // 8-апта: тереңдік тесті
    glEnable(GL_CULL_FACE);             // 8-апта: артқы беттерді сызбау
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    int indexCount = 0;
    unsigned int cube = makeCube(indexCount);
    unsigned int shader = makeShader();
    int locModel = glGetUniformLocation(shader, "uModel");
    int locView  = glGetUniformLocation(shader, "uView");
    int locProj  = glGetUniformLocation(shader, "uProj");

    // 8-аптадағы бес куб (z = 0 ... -4)
    const glm::vec3 cubePos[5] = { {0.0f, 0.0f, 0.0f}, {2.0f, 0.5f, -2.0f}, {-1.8f, -0.3f, -1.0f}, {1.2f, -0.8f, -3.5f}, {-2.2f, 1.0f, -4.0f} };

    float last = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime(), dt = std::min(now - last, 0.05f);
        last = now;

        processInput(window, dt);

        glClearColor(0.07f, 0.08f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

        // ---------- 3-қадам: view матрицасы — ЦИКЛ ІШІНДЕ, айнымалылардан ----------
        glm::mat4 view;
        if (topView)                                    // екінші камера: жоғарыдан қарайды
            view = glm::lookAt(glm::vec3(0.0f, 14.0f, -2.0f), glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, -1.0f));
        else if (wrongLookAt)                           // ҚАТЕ: мақсат — тұрақты нүкте, камера орбитамен айналады
            view = glm::lookAt(camPos, glm::vec3(0.0f), camUp);
        else                                            // ДҰРЫС: мақсат камерамен бірге жылжиды, камера ұшады
            view = glm::lookAt(camPos, camPos + camFront, camUp);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)gWidth / (float)gHeight, 0.1f, 100.0f);
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(cube);
        for (int i = 0; i < 5; i++) {
            glm::mat4 model = glm::mat4(1.0f);                       // model цикл ішінде
            model = glm::translate(model, cubePos[i]);
            model = glm::rotate(model, now * (0.5f + i * 0.3f), glm::vec3(0.3f, 1.0f, 0.0f));
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}