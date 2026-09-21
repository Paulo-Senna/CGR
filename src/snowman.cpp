#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

// ================================================================
// BONECO DE NEVE -- modelo simples e ESTATICO (sem articulacoes,
// sem pose controlada por teclado). Corpo feito de 3 esferas brancas
// empilhadas (base > meio > cabeca), com a mesma cartola preta do
// robo, dois olhos (pontinhos pretos) e um nariz de cenoura (cone
// laranja).
//
// PARTICULAS DE NEVE -- flocos (GL_POINTS, mesmo estilo do exemplo de
// fogos de artificio do professor) caindo em loop continuo sobre o
// boneco, mais um chao branco plano.
// ================================================================

// --- corpo: 3 esferas brancas, cada uma um pouco menor que a de baixo ---
const float BASE_R = 0.55f, MID_R = 0.40f, HEAD_R = 0.28f;
const float STACK_OVERLAP = 0.92f; // <1 = esferas se sobrepoem um pouco, ficam "grudadas"

// --- cartola (mesma peca do robo: aba fina e centralizada + copa alta em caixa), preta ---
const float CAP_BRIM_HALF_W = 0.34f, CAP_BRIM_H = 0.015f, CAP_BRIM_HALF_D = 0.34f;
const float CAP_BODY_HALF_W = 0.19f, CAP_BODY_HALF_H = 0.20f, CAP_BODY_HALF_D = 0.19f;

// --- rosto: dois olhos (pontinhos pretos) e nariz de cenoura (cone laranja) ---
const float EYE_DOT_R = 0.028f;
const float EYE_YAW_DEG = 22.0f;  // afasta cada olho do centro, girando em torno de Y
const float EYE_HEIGHT = 0.10f;   // altura dos olhos acima do centro da cabeca
const float NOSE_R = 0.045f, NOSE_LEN = 0.20f;
const float NOSE_HEIGHT = 0.04f;  // altura do nariz acima do centro da cabeca (um pouco abaixo dos olhos)

// --- bracinhos: dois cilindros pretos, retos, saindo da bola do meio (nao articulados) ---
const float ARM_R = 0.035f, ARM_LEN = 0.45f;
const float ARM_TILT_UP_DEG = 20.0f; // inclinacao fixa para cima, tipo graveto de boneco de neve

const int SLICES = 24, STACKS = 16;

// ---------- camera orbital (so pra olhar o boneco -- ele mesmo nao se move) ----------
float camAzimuth = -25.0f;
float camElevation = 15.0f;
const float camDistance = 4.5f;
const float camTargetY = 1.4f;

// --- neve caindo: area (coluna) por cima e ao redor do boneco ---
const int NUM_SNOW_PARTICLES = 800;
const float SNOW_AREA_HALF = 1.4f;         // largura/profundidade da coluna onde os flocos nascem
const float SNOW_SPAWN_Y_MIN = 3.0f, SNOW_SPAWN_Y_MAX = 4.5f; // acima da cartola
const float SNOW_GROUND_Y = 0.0f;          // flocos "derretem"/renascem ao tocar o chao
const float SNOW_FALL_SPEED_MIN = 0.35f, SNOW_FALL_SPEED_MAX = 0.85f; // unidades/seg
const float SNOW_DRIFT_MAX = 0.06f;        // leve deriva lateral (vento fraco)
const float SNOW_POINT_SIZE = 3.0f;

// --- chao de neve: quad branco plano em y = 0 ---
const float GROUND_HALF_SIZE = 4.0f;

struct SnowParticle { float x, y, z, velY, driftX, driftZ; };
SnowParticle snow[NUM_SNOW_PARTICLES];

GLUquadric* quad;

float toRad(float deg) { return deg * 3.14159265f / 180.0f; }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float randRange(float lo, float hi) { return lo + (float)rand() / (float)RAND_MAX * (hi - lo); }

// ================================================================
// PECAS REUTILIZAVEIS
// ================================================================

// Esfera usada no corpo (3 bolas de neve) e nos olhos.
void drawSphere(float radius) {
    gluSphere(quad, radius, SLICES, STACKS);
}

// Caixa retangular generica, centrada na origem atual (usada na cartola).
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

// ================================================================
// PARTES DO BONECO
// ================================================================

// CARTOLA: aba fina centralizada no topo da cabeca + copa alta em caixa
// por cima. Chamada com a origem ja no centro da esfera da cabeca.
void drawTopHat() {
    glColor3f(0.02f, 0.02f, 0.02f); // preto

    glPushMatrix(); // ABA
        glTranslatef(0.0f, HEAD_R + CAP_BRIM_H, 0.0f);
        drawBox(CAP_BRIM_HALF_W, CAP_BRIM_H, CAP_BRIM_HALF_D);
    glPopMatrix();

    glPushMatrix(); // COPA
        glTranslatef(0.0f, HEAD_R + CAP_BRIM_H * 2.0f + CAP_BODY_HALF_H, 0.0f);
        drawBox(CAP_BODY_HALF_W, CAP_BODY_HALF_H, CAP_BODY_HALF_D);
    glPopMatrix();
}

// ROSTO: dois olhos (pontinhos pretos) e um nariz de cenoura (cone
// laranja), colados na superficie frontal (+Z) da cabeca. Chamada com
// a origem ja no centro da esfera da cabeca.
void drawFace() {
    glColor3f(0.03f, 0.03f, 0.03f);
    glPushMatrix(); // olho ESQUERDO
        glRotatef(-EYE_YAW_DEG, 0.0f, 1.0f, 0.0f); // afasta do centro, girando em Y
        glTranslatef(0.0f, EYE_HEIGHT, HEAD_R * 0.97f); // quase na superficie da cabeca
        drawSphere(EYE_DOT_R);
    glPopMatrix();
    glPushMatrix(); // olho DIREITO
        glRotatef(EYE_YAW_DEG, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.0f, EYE_HEIGHT, HEAD_R * 0.97f);
        drawSphere(EYE_DOT_R);
    glPopMatrix();

    glColor3f(0.95f, 0.55f, 0.10f); // laranja (cenoura)
    glPushMatrix(); // nariz: cone pequeno, comeca dentro da cabeca e fura a superficie
        glTranslatef(0.0f, NOSE_HEIGHT, HEAD_R * 0.85f);
        gluCylinder(quad, NOSE_R, 0.0, NOSE_LEN, SLICES, 1); // base -> ponta = cone
    glPopMatrix();
}

// BRACINHOS: dois cilindros pretos retos, saindo dos dois lados da bola do
// meio, com uma leve inclinacao fixa para cima. Sem juntas -- nao se
// articulam. Chamada com a origem ja no centro da esfera do meio.
void drawArms() {
    glColor3f(0.02f, 0.02f, 0.02f);
    for (int i = 0; i < 2; ++i) {
        float side = (i == 0) ? -1.0f : 1.0f; // -1 = esquerdo, +1 = direito
        glPushMatrix();
            glRotatef(side * 90.0f, 0.0f, 1.0f, 0.0f);      // gira o eixo do cilindro (Z) para apontar pro lado (+-X)
            glRotatef(-ARM_TILT_UP_DEG, 1.0f, 0.0f, 0.0f);  // inclina um pouco para cima
            glTranslatef(0.0f, 0.0f, MID_R * 0.85f);        // comeca dentro da bola, quase na superficie
            gluCylinder(quad, ARM_R, ARM_R, ARM_LEN, SLICES / 2, 1);
        glPopMatrix();
    }
}





// ================================================================
// PARTICULAS DE NEVE
// ================================================================

// (Re)nasce o floco `i` no topo da coluna, em posicao X/Z aleatoria.
// `randomHeight` = true espalha a altura inicial (usado so no InitSnow,
// pra nao nascerem todos "empilhados" no mesmo instante); nos respawns
// durante a queda, nasce sempre perto do topo (SNOW_SPAWN_Y_MAX).
void resetSnowParticle(int i, bool randomHeight) {
    snow[i].x = randRange(-SNOW_AREA_HALF, SNOW_AREA_HALF);
    snow[i].z = randRange(-SNOW_AREA_HALF, SNOW_AREA_HALF);
    snow[i].y = randomHeight ? randRange(SNOW_GROUND_Y, SNOW_SPAWN_Y_MAX)
                             : randRange(SNOW_SPAWN_Y_MIN, SNOW_SPAWN_Y_MAX);
    snow[i].velY = randRange(SNOW_FALL_SPEED_MIN, SNOW_FALL_SPEED_MAX);
    snow[i].driftX = randRange(-SNOW_DRIFT_MAX, SNOW_DRIFT_MAX);
    snow[i].driftZ = randRange(-SNOW_DRIFT_MAX, SNOW_DRIFT_MAX);
}

void initSnow() {
    for (int i = 0; i < NUM_SNOW_PARTICLES; ++i)
        resetSnowParticle(i, true);
}

// Ao contrario dos fogos (particulas com "lifetime" que se apagam todas
// juntas), aqui cada floco cai sem parar: ao tocar o chao, renasce
// direto no topo -- efeito de nevasca continua.
void updateSnow(float dt) {
    for (int i = 0; i < NUM_SNOW_PARTICLES; ++i) {
        snow[i].y -= snow[i].velY * dt;
        snow[i].x += snow[i].driftX * dt;
        snow[i].z += snow[i].driftZ * dt;
        if (snow[i].y <= SNOW_GROUND_Y)
            resetSnowParticle(i, false);
    }
}

// nao tinha esse chao branco antes, adicionei pra fazer mais sentido com a neve
void drawGround() {
    glColor3f(0.92f, 0.95f, 1.0f);
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-GROUND_HALF_SIZE, 0.0f, -GROUND_HALF_SIZE);
        glVertex3f(-GROUND_HALF_SIZE, 0.0f,  GROUND_HALF_SIZE);
        glVertex3f( GROUND_HALF_SIZE, 0.0f,  GROUND_HALF_SIZE);
        glVertex3f( GROUND_HALF_SIZE, 0.0f, -GROUND_HALF_SIZE);
    glEnd();
}

// FLOCOS: pontos brancos (GL_POINTS)
void drawSnowParticles() {
    glDisable(GL_LIGHTING);
    glEnable(GL_POINT_SMOOTH);
    glPointSigze(SNOW_POINT_SIZE);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_POINTS);
        for (int i = 0; i < NUM_SNOW_PARTICLES; ++i)
            glVertex3f(snow[i].x, snow[i].y, snow[i].z);
    glEnd();

    glEnable(GL_LIGHTING);
}

// BONECO DE NEVE COMPLETO: 3 esferas empilhadas do chao pra cima
void drawSnowman() {
    glPushMatrix();
        glColor3f(0.97f, 0.97f, 0.99f); // branco de neve

        glTranslatef(0.0f, BASE_R, 0.0f); // sobe ate o centro da esfera de baixo
        drawSphere(BASE_R);

        glTranslatef(0.0f, (BASE_R + MID_R) * STACK_OVERLAP, 0.0f); // sobe ate o centro do meio
        drawSphere(MID_R);
        drawArms();

        glTranslatef(0.0f, (MID_R + HEAD_R) * STACK_OVERLAP, 0.0f); // sobe ate o centro da cabeca
        glColor3f(0.97f, 0.97f, 0.99f); // drawArms() mudou a cor -- volta pro branco de neve
        drawSphere(HEAD_R);

        drawFace();
        drawTopHat();
    glPopMatrix();
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

// So mexe na camera -- o boneco de neve em si nao tem pose/movimento.
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
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float eyeX = camDistance * cosf(toRad(camElevation)) * sinf(toRad(camAzimuth));
    float eyeY = camDistance * sinf(toRad(camElevation)) + camTargetY;
    float eyeZ = camDistance * cosf(toRad(camElevation)) * cosf(toRad(camAzimuth));
    gluLookAt(eyeX, eyeY, eyeZ, 0.0, camTargetY, 0.0, 0.0, 1.0, 0.0);

    drawGround();
    drawSnowman();
    drawSnowParticles();
}

int main() {
    if (!glfwInit()) {
        std::cout << "Falha ao iniciar GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "CGR - Boneco de Neve (Quadricas)", nullptr, nullptr);
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

    srand((unsigned int)time(nullptr));
    initSnow();

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    framebuffer_size_callback(window, fbWidth, fbHeight);

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        processInput(window, dt);
        updateSnow(dt);
        display();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    gluDeleteQuadric(quad);
    glfwTerminate();
    return 0;
}
