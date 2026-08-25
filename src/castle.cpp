#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <cmath>
#include <iostream>

// ================================================================
// CASTELO -- modelo simples e ESTATICO (sem articulacoes, sem pose
// controlada por teclado). Base verde (gramado), 4 torres cinzas
// (cilindro) com telhado conico marrom nas pontas, ligadas por
// muralhas (caixas retangulares) entre elas.
// ================================================================

// --- base (retangulo verde, o "gramado" onde o castelo fica em cima) ---
const float GROUND_HALF_W = 2.2f, GROUND_HALF_D = 2.2f, GROUND_HALF_H = 0.05f;

// --- torres (cilindro cinza + telhado conico marrom), uma em cada canto ---
const float TOWER_OFFSET = 1.5f;           // distancia do centro do castelo ate cada torre (eixos X e Z)
const float TOWER_R = 0.35f, TOWER_H = 1.6f;
const float ROOF_R = TOWER_R * 1.2f, ROOF_H = 0.55f; // telhado um pouco mais largo que a torre

// --- muralhas (retangulos ligando os lados das 4 torres) ---
const float WALL_H = 0.9f, WALL_THICK = 0.16f; // mais baixas que as torres

const int SLICES = 24, STACKS = 8;

// --- cores ---
const float GROUND_COLOR[3] = { 0.25f, 0.55f, 0.25f }; // verde
const float TOWER_COLOR[3]  = { 0.50f, 0.50f, 0.53f }; // cinza pedra
const float ROOF_COLOR[3]   = { 0.45f, 0.28f, 0.14f }; // marrom

// ---------- camera orbital (so pra olhar o castelo -- ele nao se move) ----------
float camAzimuth = -30.0f;
float camElevation = 25.0f;
const float camDistance = 7.5f;
const float camTargetY = 1.0f;

GLUquadric* quad;

float toRad(float deg) { return deg * 3.14159265f / 180.0f; }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ================================================================
// PECAS REUTILIZAVEIS
// ================================================================

// Caixa retangular generica, centrada na origem atual (usada na base e nas muralhas).
void drawBox(float hx, float hy, float hz) {
    glBegin(GL_QUADS);
        // Frente (+Z)
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(-hx, -hy,  hz);
        glVertex3f( hx, -hy,  hz);
        glVertex3f( hx,  hy,  hz);
        glVertex3f(-hx,  hy,  hz);
        // Tras (-Z)
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(-hx, -hy, -hz);
        glVertex3f(-hx,  hy, -hz);
        glVertex3f( hx,  hy, -hz);
        glVertex3f( hx, -hy, -hz);
        // Topo (+Y)
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-hx,  hy, -hz);
        glVertex3f(-hx,  hy,  hz);
        glVertex3f( hx,  hy,  hz);
        glVertex3f( hx,  hy, -hz);
        // Base (-Y)
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(-hx, -hy, -hz);
        glVertex3f( hx, -hy, -hz);
        glVertex3f( hx, -hy,  hz);
        glVertex3f(-hx, -hy,  hz);
        // Direita (+X)
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f( hx, -hy, -hz);
        glVertex3f( hx,  hy, -hz);
        glVertex3f( hx,  hy,  hz);
        glVertex3f( hx, -hy,  hz);
        // Esquerda (-X)
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-hx, -hy, -hz);
        glVertex3f(-hx, -hy,  hz);
        glVertex3f(-hx,  hy,  hz);
        glVertex3f(-hx,  hy, -hz);
    glEnd();
}

// Cilindro em pe (eixo Y), tampado nas duas pontas, crescendo do chao
// atual (y=0) ate y=height. Usado no corpo de cada torre.
void drawCylinderUp(float radius, float height) {
    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // cilindro nasce no eixo Z; aponta para +Y
        gluDisk(quad, 0.0, radius, SLICES, 1);
        gluCylinder(quad, radius, radius, height, SLICES, STACKS);
        glTranslatef(0.0f, 0.0f, height);
        gluDisk(quad, 0.0, radius, SLICES, 1);
    glPopMatrix();
}

// Telhado conico (cilindro com raio do topo = 0), crescendo do chao
// atual (y=0, ja no topo da torre) ate a ponta em y=height.
void drawRoofCone(float baseRadius, float height) {
    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluDisk(quad, 0.0, baseRadius, SLICES, 1); // tampa a torre por baixo do telhado
        gluCylinder(quad, baseRadius, 0.0, height, SLICES, 1);
    glPopMatrix();
}

// ================================================================
// PARTES DO CASTELO
// ================================================================

// TORRE: cilindro cinza + telhado conico marrom, numa das quatro pontas
// do castelo (x, z sao a posicao da torre no chao).
void drawTower(float x, float z) {
    glPushMatrix();
        glTranslatef(x, 0.0f, z);

        glColor3fv(TOWER_COLOR);
        drawCylinderUp(TOWER_R, TOWER_H);

        glTranslatef(0.0f, TOWER_H, 0.0f);
        glColor3fv(ROOF_COLOR);
        drawRoofCone(ROOF_R, ROOF_H);
    glPopMatrix();
}

// Um segmento de muralha: caixa retangular centrada em (cx, WALL_H/2, cz),
// com meio-comprimento halfLenX/halfLenZ (um dos dois e a espessura da
// parede, o outro e o comprimento que liga duas torres).
void drawWallSegment(float cx, float cz, float halfLenX, float halfLenZ) {
    glPushMatrix();
        glTranslatef(cx, WALL_H * 0.5f, cz);
        drawBox(halfLenX, WALL_H * 0.5f, halfLenZ);
    glPopMatrix();
}

// MURALHAS: liga os quatro lados das torres com retangulos, fechando o
// perimetro do castelo (como se fossem as paredes entre as torres).
void drawWalls() {
    glColor3fv(TOWER_COLOR);
    float halfSpan = TOWER_OFFSET - TOWER_R; // metade do vao livre entre duas torres vizinhas

    // muralhas "de frente/fundo" (variam em X, ligam torres com o mesmo Z)
    drawWallSegment(0.0f,  TOWER_OFFSET, halfSpan, WALL_THICK);
    drawWallSegment(0.0f, -TOWER_OFFSET, halfSpan, WALL_THICK);

    // muralhas "laterais" (variam em Z, ligam torres com o mesmo X)
    drawWallSegment( TOWER_OFFSET, 0.0f, WALL_THICK, halfSpan);
    drawWallSegment(-TOWER_OFFSET, 0.0f, WALL_THICK, halfSpan);
}

// CASTELO COMPLETO: base verde, muralhas e as quatro torres nos cantos.
void drawCastle() {
    glColor3fv(GROUND_COLOR);
    glPushMatrix();
        glTranslatef(0.0f, -GROUND_HALF_H, 0.0f); // deixa o topo da base exatamente em y=0
        drawBox(GROUND_HALF_W, GROUND_HALF_H, GROUND_HALF_D);
    glPopMatrix();

    drawWalls();

    drawTower( TOWER_OFFSET,  TOWER_OFFSET);
    drawTower( TOWER_OFFSET, -TOWER_OFFSET);
    drawTower(-TOWER_OFFSET,  TOWER_OFFSET);
    drawTower(-TOWER_OFFSET, -TOWER_OFFSET);
}

// ================================================================
// JANELA, CAMERA E ENTRADA
// ================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)width / (double)height, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// So mexe na camera -- o castelo em si nao tem pose/movimento.
void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float camSpeed = 60.0f * dt;

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camAzimuth -= camSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camAzimuth += camSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camElevation += camSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camElevation -= camSpeed;
    camElevation = clampf(camElevation, 5.0f, 85.0f);
}

void display() {
    glClearColor(0.55f, 0.75f, 0.90f, 1.0f); // ceu azul claro
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float eyeX = camDistance * cosf(toRad(camElevation)) * sinf(toRad(camAzimuth));
    float eyeY = camDistance * sinf(toRad(camElevation)) + camTargetY;
    float eyeZ = camDistance * cosf(toRad(camElevation)) * cosf(toRad(camAzimuth));
    gluLookAt(eyeX, eyeY, eyeZ, 0.0, camTargetY, 0.0, 0.0, 1.0, 0.0);

    drawCastle();
}

int main() {
    if (!glfwInit()) {
        std::cout << "Falha ao iniciar GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "CGR - Castelo (Quadricas)", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        return -1;
    }

    quad = gluNewQuadric();
    gluQuadricNormals(quad, GLU_SMOOTH);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat lightPos[] = { 5.0f, 8.0f, 5.0f, 1.0f };
    GLfloat lightAmbient[] = { 0.30f, 0.30f, 0.30f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    framebuffer_size_callback(window, fbWidth, fbHeight);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        processInput(window, dt);
        display();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    gluDeleteQuadric(quad);
    glfwTerminate();
    return 0;
}
