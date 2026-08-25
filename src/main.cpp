#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <cmath>
#include <iostream>

// ================================================================
// DIMENSOES DO CORPO
// ================================================================

// --- pes (retangulo achatado, preto) ---
const float FOOT_HALF_W = 0.14f, FOOT_HEIGHT = 0.10f, FOOT_LEN = 0.42f;

// --- pernas (canela + coxa), uma de cada lado do quadril ---
const float SHIN_LEN = 0.75f, SHIN_R = 0.14f;
const float KNEE_JOINT_R = 0.16f;
const float THIGH_LEN = 0.75f, THIGH_R = 0.17f;
const float HIP_JOINT_R = 0.19f;
const float HIP_OFFSET = 0.28f;               // distancia do quadril ate o centro do corpo
const float PELVIS_HEIGHT = THIGH_LEN + SHIN_LEN; // altura do quadril em relacao ao chao

// --- tronco (caixa retangular; mais fino de peito/costas do que de ombro a ombro) ---
const float TORSO_HALF_W = 0.38f, TORSO_HALF_D = 0.24f, TORSO_LEN = 1.1f;

// --- pescoco + cabeca ---
const float NECK_LEN = 0.18f, NECK_R = 0.11f;
const float HEAD_HALF = 0.28f; // meio-lado do cubo da cabeca

// --- cartola (aba fina e centralizada + copa alta em caixa), preta ---
const float CAP_BRIM_HALF_W = 0.34f, CAP_BRIM_H = 0.015f, CAP_BRIM_HALF_D = 0.34f; // aba larga, simetrica
const float CAP_BODY_HALF_W = 0.19f, CAP_BODY_HALF_H = 0.20f, CAP_BODY_HALF_D = 0.19f; // copa mais baixa

// --- rosto (dois olhos estilo Minecraft: fundo branco + pupila preta; boca: quadrado preto) ---
const float EYE_HALF_W = 0.065f, EYE_HALF_H = 0.055f; // fundo branco de cada olho (retangulo)
const float EYE_OFFSET_X = 0.13f;                      // distancia de cada olho ate o centro da cara
const float PUPIL_HALF = 0.022f;                       // pupila preta (quadrado)
const float MOUTH_HALF = 0.035f;                      // boca (quadrado preto)
const float FACE_EPS = 0.006f;                        // espessura/deslocamento p/ evitar z-fighting com a cabeca

// --- bracos (braco + antebraco) e maos, um de cada lado do tronco ---
const float SHOULDER_JOINT_R = 0.15f;
const float SHOULDER_OFFSET = 0.50f;          // distancia do ombro ate o centro do tronco
const float SHOULDER_DROP = 0.16f;            // abaixa os ombros em relacao ao topo do tronco
const float UPPER_ARM_LEN = 0.62f, UPPER_ARM_R = 0.12f;
const float ELBOW_JOINT_R = 0.13f;
const float FOREARM_LEN = 0.55f, FOREARM_R = 0.10f;
const float HAND_R = 0.13f;

// --- garrinhas (garras cilindricas vermelhas na mao, articulam abrindo/fechando) ---
const float CLAW_LEN = 0.09f, CLAW_R = 0.028f;

const int SLICES = 24, STACKS = 8;

// ================================================================
// CORES -- escala de cinza: caixa toracica mais escura, membros um
// pouco mais claros, e as articulacoes no cinza mais claro de todos.
// ================================================================
const float TORSO_COLOR[3] = { 0.30f, 0.34f, 0.40f };
const float LIMB_COLOR[3]  = { 0.44f, 0.46f, 0.51f };
const float JOINT_COLOR[3] = { 0.78f, 0.80f, 0.84f };
const float HAND_COLOR[3]  = { 0.03f, 0.03f, 0.03f };

// ================================================================
// POSE ATUAL -- angulos (graus) 
// ================================================================

float waistAngle = 0.0f;      // giro da cintura (torco do tronco sobre o quadril, eixo Y)
float shoulderAngleL = 15.0f, shoulderAngleR = 15.0f; // balanco do ombro (flexao), eixo X
float shoulderAbductionL = 0.0f, shoulderAbductionR = 0.0f; // abertura lateral do ombro (abducao), eixo Z
float elbowAngleL = -25.0f,   elbowAngleR = -25.0f;   // dobra do cotovelo, eixo X
float hipAngleL = -8.0f,      hipAngleR = -8.0f;      // balanco do quadril, eixo X
float kneeAngleL = -15.0f,    kneeAngleR = -15.0f;    // dobra do joelho, eixo X
float headAngle = 0.0f;       // giro da cabeca para D/E, eixo Y
float clawAngle = 55.0f;      // abre/fecha os dedos da garra (mais alto = mais fechada)

// Limites de cada junta, para o robo nao "atravessar" o proprio corpo.
const float SHOULDER_MIN = -50.0f,  SHOULDER_MAX = 110.0f; // tras / frente
const float ABDUCTION_MIN = 0.0f,   ABDUCTION_MAX = 90.0f; // encostado no corpo / ate perpendicular (T)
const float ELBOW_MIN    = -140.0f, ELBOW_MAX    = -5.0f;  // so dobra p/ um lado
const float HIP_MIN      = -45.0f,  HIP_MAX      = 60.0f;  // tras / frente
const float KNEE_MIN     = -120.0f, KNEE_MAX     = -5.0f;  // so dobra p/ um lado
const float HEAD_MIN     = -45.0f,  HEAD_MAX     = 45.0f;  // esquerda / direita
const float CLAW_MIN     = 15.0f,   CLAW_MAX     = 90.0f;  // aberta / fechada






// ---------- camera orbital ----------
float camAzimuth = -35.0f;
float camElevation = 18.0f;
const float camDistance = 6.5f;
const float camTargetY = 1.6f; // mais ou menos na altura do peito do robo

GLUquadric* quad;

float toRad(float deg) { return deg * 3.14159265f / 180.0f; }
float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }








// ================================================================
// PECAS REUTILIZAVEIS
// ================================================================

// Osso que cresce para CIMA a partir da origem atual (usado no pescoco).
// Desenha o cilindro tampado nas duas pontas e avanca o cursor ate a ponta dele.
void drawLinkUp(float length, float radius) {
    glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f); // cilindro nasce no eixo Z; aponta para +Y
        gluDisk(quad, 0.0, radius, SLICES, 1);
        gluCylinder(quad, radius, radius, length, SLICES, STACKS);
        glTranslatef(0.0f, 0.0f, length);
        gluDisk(quad, 0.0, radius, SLICES, 1);
    glPopMatrix();
    glTranslatef(0.0f, length, 0.0f);
}

// Osso que cresce para BAIXO a partir da origem atual (usado em bracos e pernas,
// que pendem do ombro/quadril para baixo).
void drawLinkDown(float length, float radius) {
    glPushMatrix();
        glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // cilindro nasce no eixo Z; aponta para -Y
        gluDisk(quad, 0.0, radius, SLICES, 1);
        gluCylinder(quad, radius, radius, length, SLICES, STACKS);
        glTranslatef(0.0f, 0.0f, length);
        gluDisk(quad, 0.0, radius, SLICES, 1);
    glPopMatrix();
    glTranslatef(0.0f, -length, 0.0f);
}

// ESFERA DAS JUNTAS
void drawSphere(float radius) {
    gluSphere(quad, radius, SLICES, STACKS);
}

//caixa
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

// Cubo (usado na cabeca): caso especial da caixa, com os 3 meio-lados iguais.
void drawCube(float halfSize) {
    drawBox(halfSize, halfSize, halfSize);
}

// Osso em forma de caixa que cresce para CIMA a partir da origem atual (USADO NO TRONCO)
void drawBoxLinkUp(float length, float hx, float hz) {
    glPushMatrix();
        glTranslatef(0.0f, length * 0.5f, 0.0f);
        drawBox(hx, length * 0.5f, hz);
    glPopMatrix();
    glTranslatef(0.0f, length, 0.0f);
}

// Osso em forma de caixa que cresce para BAIXO a partir da origem atual (USADO NO BICEPS)
void drawBoxLinkDown(float length, float hx, float hz) {
    glPushMatrix();
        glTranslatef(0.0f, -length * 0.5f, 0.0f);
        drawBox(hx, length * 0.5f, hz);
    glPopMatrix();
    glTranslatef(0.0f, -length, 0.0f);
}

//==================================================================




// ================================================================
// PARTES DO CORPO
// ================================================================

// PE: um retangulo achatado preto (eixo Z, "para frente") representando a
// sola, desenhado a partir do tornozelo (ponta da canela).
void drawFoot() {
    glColor3f(0.03f, 0.03f, 0.03f);
    glPushMatrix();
        glTranslatef(0.0f, -FOOT_HEIGHT * 0.5f, FOOT_LEN * 0.5f); // calcanhar -> bico, achatado
        drawBox(FOOT_HALF_W, FOOT_HEIGHT * 0.5f, FOOT_LEN * 0.5f);
    glPopMatrix();
}

// PERNA (quadril -> coxa -> joelho -> canela -> pe). `side` = +1 (direita) ou -1
// (esquerda), so desloca a perna para o lado certo; `hipAngle`/`kneeAngle` sao
// os angulos DESSE lado (cada perna tem os seus, movem independente uma da outra).
// Chamada com a origem ja no nivel do quadril (PELVIS_HEIGHT).
//
// Segue o MESMO padrao do modelo de referencia (shoulder/elbow em 2D):
//   glPushMatrix() -> translate ate a junta -> desenha a junta -> glRotatef()
//   -> desenha o osso (e avanca ate a ponta dele) -> glPushMatrix() para o
//   PROXIMO elo (mesma ideia do push interno do exemplo, antes do "elbow").
void drawLeg(float side, float hipAngle, float kneeAngle) {
    glPushMatrix();
        glTranslatef(side * HIP_OFFSET, 0.0f, 0.0f); // 1) posiciona a junta do quadril

        glColor3fv(JOINT_COLOR);
        drawSphere(HIP_JOINT_R);                 // JUNTA: quadril
        glRotatef(hipAngle, 1.0f, 0.0f, 0.0f);    // 2) roda no quadril (equivalente ao "shoulder")

        glColor3fv(LIMB_COLOR);
        drawLinkDown(THIGH_LEN, THIGH_R);        // 3) OSSO: coxa -- desenha e avanca ate o joelho

        glPushMatrix();                          // igual ao push interno do modelo (proximo elo)
            glColor3fv(JOINT_COLOR);
            drawSphere(KNEE_JOINT_R);                // JUNTA: joelho
            glRotatef(kneeAngle, 1.0f, 0.0f, 0.0f);   // roda no joelho (equivalente ao "elbow")

            glColor3fv(LIMB_COLOR);
            drawLinkDown(SHIN_LEN, SHIN_R);          // OSSO: canela -- desenha e avanca ate o tornozelo

            drawFoot();                               // PE
        glPopMatrix();
    glPopMatrix();
}

// TRONCO: caixa retangular (corpo "quadrado"), mais fina de peito/costas
// (profundidade) do que de ombro a ombro (largura). Chamado com a origem ja
// no nivel do quadril (mesmo ponto onde as pernas nascem), entao a base do
// tronco encaixa exatamente nas juntas do quadril.
void drawTorso() {
    glColor3fv(TORSO_COLOR);
    drawBoxLinkUp(TORSO_LEN, TORSO_HALF_W, TORSO_HALF_D); // cursor sobe ate a altura dos ombros
}

//CARTOLERA ESTILOSA
void drawTopHat() {
    glColor3f(0.02f, 0.02f, 0.02f); // preto

    glPushMatrix(); // ABA: retangulo fino, centralizado no topo da cabeca
        glTranslatef(0.0f, HEAD_HALF + CAP_BRIM_H, 0.0f);
        drawBox(CAP_BRIM_HALF_W, CAP_BRIM_H, CAP_BRIM_HALF_D);
    glPopMatrix();

    glPushMatrix(); // COPA: caixa alta em cima da aba
        glTranslatef(0.0f, HEAD_HALF + CAP_BRIM_H * 2.0f + CAP_BODY_HALF_H, 0.0f);
        drawBox(CAP_BODY_HALF_W, CAP_BODY_HALF_H, CAP_BODY_HALF_D);
    glPopMatrix();
}


// ROSTO do HOMI
void drawFace() {
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix(); // fundo branco do olho ESQUERDO
        glTranslatef(-EYE_OFFSET_X, HEAD_HALF * 0.20f, HEAD_HALF + FACE_EPS);
        drawBox(EYE_HALF_W, EYE_HALF_H, FACE_EPS);
    glPopMatrix();
    glPushMatrix(); // fundo branco do olho DIREITO
        glTranslatef(EYE_OFFSET_X, HEAD_HALF * 0.20f, HEAD_HALF + FACE_EPS);
        drawBox(EYE_HALF_W, EYE_HALF_H, FACE_EPS);
    glPopMatrix();

    glColor3f(0.02f, 0.02f, 0.02f);
    glPushMatrix(); // pupila preta ESQUERDA, um pouco mais a frente pra nao sumir dentro do branco
        glTranslatef(-EYE_OFFSET_X, HEAD_HALF * 0.20f, HEAD_HALF + FACE_EPS * 3.0f);
        drawBox(PUPIL_HALF, PUPIL_HALF, FACE_EPS);
    glPopMatrix();
    glPushMatrix(); // pupila preta DIREITA
        glTranslatef(EYE_OFFSET_X, HEAD_HALF * 0.20f, HEAD_HALF + FACE_EPS * 3.0f);
        drawBox(PUPIL_HALF, PUPIL_HALF, FACE_EPS);
    glPopMatrix();

    glPushMatrix(); // boca
        glTranslatef(0.0f, -HEAD_HALF * 0.40f, HEAD_HALF + FACE_EPS);
        drawBox(MOUTH_HALF, MOUTH_HALF, FACE_EPS);
    glPopMatrix();
}

// PESCOCO + CABECA (cubo) + ROSTO + CARTOLA, a partir da altura dos ombros
// (topo do tronco).
void drawNeckAndHead() {
    glColor3f(0.40f, 0.44f, 0.50f);
    drawLinkUp(NECK_LEN, NECK_R);            // OSSO: pescoco

    glRotatef(headAngle, 0.0f, 1.0f, 0.0f);  // gira a cabeca (rosto e cartola juntos) p/ D ou E, eixo Y

    glColor3f(0.80f, 0.72f, 0.55f);
    drawCube(HEAD_HALF);                      // CABECA (cubo)

    drawFace();                                // olho + boca na face frontal
    drawTopHat();                               // cartola preta em cima da cabeca
}





//GARRAS VERMELHAS
void drawHandClaws(float angle) {
    glColor3f(0.80f, 0.08f, 0.08f);
    for (int i = 0; i < 3; ++i) {
        glPushMatrix();
            glRotatef(i * 120.0f, 0.0f, 1.0f, 0.0f); // distribui ao redor da mao
            glRotatef(angle, 1.0f, 0.0f, 0.0f);      // abre/fecha (curva para baixo/frente, feito garra)
            glTranslatef(0.0f, 0.0f, HAND_R * 0.75f); // nasce perto da superficie da esfera
            gluCylinder(quad, CLAW_R, CLAW_R * 0.35f, CLAW_LEN, SLICES / 2, 1); // afunila na ponta
        glPopMatrix();
    }
}



//BRAÇO
void drawArm(float side, float shoulderAngle, float elbowAngle, float abduction) {
    glPushMatrix();
        glTranslatef(side * SHOULDER_OFFSET, -SHOULDER_DROP, 0.0f); // 1) posiciona a junta do ombro (afastada e mais baixa)

        glColor3fv(JOINT_COLOR);
        drawSphere(SHOULDER_JOINT_R);                    // JUNTA: ombro
        glRotatef(abduction * side, 0.0f, 0.0f, 1.0f);    // 2a) abre o braco para o lado (abducao), eixo Z
        glRotatef(shoulderAngle, 1.0f, 0.0f, 0.0f);       // 2b) balanco frente/tras (flexao), eixo X

        glColor3fv(LIMB_COLOR);
        drawBoxLinkDown(UPPER_ARM_LEN, UPPER_ARM_R, UPPER_ARM_R); // 3) OSSO: biceps (caixa) -- desenha e avanca ate o cotovelo

        glPushMatrix();                              // igual ao push interno do modelo (proximo elo)
            glColor3fv(JOINT_COLOR);
            drawSphere(ELBOW_JOINT_R);                 // JUNTA: cotovelo
            glRotatef(elbowAngle, 1.0f, 0.0f, 0.0f);    // roda no cotovelo (igual ao "elbow" do modelo)

            glColor3fv(LIMB_COLOR);
            drawLinkDown(FOREARM_LEN, FOREARM_R);      // OSSO: antebraco -- desenha e avanca ate o pulso

            glColor3fv(HAND_COLOR);
            drawSphere(HAND_R);                         // MAO (mao fechada = esfera, preta)
            drawHandClaws(clawAngle);                    // 3 garrinhas vermelhas, articuladas
        glPopMatrix();
    glPopMatrix();
}







// ================================================================
// MONTAGEM COMPLETA DO ROBO
// ================================================================
void drawRobot() {
    glPushMatrix();
        // Sobe do chao ate a altura do quadril UMA vez so -- pernas e tronco
        // nascem exatamente nesse mesmo ponto, entao ficam encaixados.
        glTranslatef(0.0f, PELVIS_HEIGHT, 0.0f);

        // Pernas penduram do quadril ate o chao e NAO seguem o giro da
        // cintura -- os pes ficam plantados no chao mesmo se o tronco girar.
        glPushMatrix();
            drawLeg(+1.0f, hipAngleR, kneeAngleR);  // perna direita
            drawLeg(-1.0f, hipAngleL, kneeAngleL);  // perna esquerda
        glPopMatrix();

        glRotatef(waistAngle, 0.0f, 1.0f, 0.0f);  // cintura: gira tudo daqui pra cima

        drawTorso(); // cursor termina na altura dos ombros

        glPushMatrix();
            drawNeckAndHead(); // ramo isolado: nao deve afetar os bracos
        glPopMatrix();

        drawArm(+1.0f, shoulderAngleR, elbowAngleR, shoulderAbductionR);  // braco direito
        drawArm(-1.0f, shoulderAngleL, elbowAngleL, shoulderAbductionL);  // braco esquerdo
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

void processInput(GLFWwindow* window, float dt) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float speed = 60.0f * dt;   // graus/seg
    const float camSpeed = 60.0f * dt;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) waistAngle += speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) waistAngle -= speed;
    waistAngle = fmodf(waistAngle, 360.0f);

    // braco ESQUERDO: ombro W/S, cotovelo E/D
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) shoulderAngleL += speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) shoulderAngleL -= speed;
    shoulderAngleL = clampf(shoulderAngleL, SHOULDER_MIN, SHOULDER_MAX);

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) elbowAngleL += speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) elbowAngleL -= speed;
    elbowAngleL = clampf(elbowAngleL, ELBOW_MIN, ELBOW_MAX);

    // braco ESQUERDO: abre para o lado (abducao) Z/X, ate ficar perpendicular ao corpo
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) shoulderAbductionL += speed;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) shoulderAbductionL -= speed;
    shoulderAbductionL = clampf(shoulderAbductionL, ABDUCTION_MIN, ABDUCTION_MAX);

    // braco DIREITO: ombro I/K, cotovelo O/L
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) shoulderAngleR += speed;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) shoulderAngleR -= speed;
    shoulderAngleR = clampf(shoulderAngleR, SHOULDER_MIN, SHOULDER_MAX);

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) elbowAngleR += speed;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) elbowAngleR -= speed;
    elbowAngleR = clampf(elbowAngleR, ELBOW_MIN, ELBOW_MAX);

    // braco DIREITO: abre para o lado (abducao) virgula/ponto, ate ficar perpendicular ao corpo
    if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) shoulderAbductionR += speed;
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) shoulderAbductionR -= speed;
    shoulderAbductionR = clampf(shoulderAbductionR, ABDUCTION_MIN, ABDUCTION_MAX);

    // perna ESQUERDA: quadril R/F, joelho T/G
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) hipAngleL += speed;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) hipAngleL -= speed;
    hipAngleL = clampf(hipAngleL, HIP_MIN, HIP_MAX);

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) kneeAngleL += speed;
    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) kneeAngleL -= speed;
    kneeAngleL = clampf(kneeAngleL, KNEE_MIN, KNEE_MAX);

    // perna DIREITA: quadril C/V, joelho B/N
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) hipAngleR += speed;
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) hipAngleR -= speed;
    hipAngleR = clampf(hipAngleR, HIP_MIN, HIP_MAX);

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) kneeAngleR += speed;
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) kneeAngleR -= speed;
    kneeAngleR = clampf(kneeAngleR, KNEE_MIN, KNEE_MAX);

    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) headAngle += speed;
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) headAngle -= speed;
    headAngle = clampf(headAngle, HEAD_MIN, HEAD_MAX);

    // garra (as duas maos): U fecha, J abre
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) clawAngle += speed;
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) clawAngle -= speed;
    clawAngle = clampf(clawAngle, CLAW_MIN, CLAW_MAX);

    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camAzimuth -= camSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camAzimuth += camSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camElevation += camSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camElevation -= camSpeed;
    camElevation = clampf(camElevation, 5.0f, 85.0f);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        waistAngle = 0.0f;
        shoulderAngleL = shoulderAngleR = 15.0f;
        shoulderAbductionL = shoulderAbductionR = 0.0f;
        elbowAngleL = elbowAngleR = -25.0f;
        hipAngleL = hipAngleR = -8.0f;
        kneeAngleL = kneeAngleR = -15.0f;
        headAngle = 0.0f;
        clawAngle = 55.0f;
    }
}


//FUNDO -> ignora
void display() {
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float eyeX = camDistance * cosf(toRad(camElevation)) * sinf(toRad(camAzimuth));
    float eyeY = camDistance * sinf(toRad(camElevation)) + camTargetY;
    float eyeZ = camDistance * cosf(toRad(camElevation)) * cosf(toRad(camAzimuth));
    gluLookAt(eyeX, eyeY, eyeZ, 0.0, camTargetY, 0.0, 0.0, 1.0, 0.0);

    drawRobot();
}



int main() {
    if (!glfwInit()) {
        std::cout << "Falha ao iniciar GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "CGR - Robo Humanoide (Quadricas)", nullptr, nullptr);
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