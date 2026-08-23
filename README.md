# Cola OpenGL — Robô (GLFW + GLAD + GLU, Compatibility Profile)

Tudo aqui parte do princípio que você já tem `quad = gluNewQuadric();` criado no `main()`
e que já está dentro do contexto certo (`glMatrixMode(GL_MODELVIEW)` ativo, dentro do loop
de `display()`). Todo bloco abaixo pode ser colado direto onde você quiser desenhar algo.

---

## Formas geométricas

Todas nascem centradas/alinhadas na origem atual da matriz (onde o "cursor" estiver
depois dos `glTranslatef`/`glRotatef` anteriores).

### Esfera
```cpp
gluSphere(quad, RAIO, SLICES, STACKS);
```
- `RAIO`: tamanho da esfera.
- `SLICES`, `STACKS`: já são globais no seu código (24 e 8) — controlam suavidade. Não mexe, a não ser que queira mais/menos poligonal.
- Nasce **centrada** na origem atual.

### Cilindro (osso)
```cpp
glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // aponta para -Y (pra baixo) — usa -90 pra apontar pra +Y (pra cima)
    gluDisk(quad, 0.0, RAIO, SLICES, 1);              // tampa de baixo
    gluCylinder(quad, RAIO, RAIO, ALTURA, SLICES, STACKS);
    glTranslatef(0.0f, 0.0f, ALTURA);
    gluDisk(quad, 0.0, RAIO, SLICES, 1);              // tampa de cima
glPopMatrix();
```
- `RAIO`: espessura do cilindro.
- `ALTURA`: comprimento do "osso".
- Isso já é basicamente o que `drawLinkDown`/`drawLinkUp` fazem — se for um osso novo, é melhor **chamar essas duas funções** em vez de reescrever isso:
```cpp
drawLinkDown(ALTURA, RAIO); // cresce pra baixo e "empurra" o cursor pra ponta
drawLinkUp(ALTURA, RAIO);   // cresce pra cima e "empurra" o cursor pra ponta
```

### Cone (ex: para pés pontudos, chifres, nariz, etc.)
```cpp
gluCylinder(quad, RAIO_BASE, 0.0, ALTURA, SLICES, STACKS);
```
- Igual ao cilindro, mas com `RAIO_TOPO = 0.0` — vira um cone. Pode usar qualquer valor entre `RAIO_BASE` e `0.0` pra fazer tronco de cone (o tronco do robô já usa isso: `TORSO_BASE_R` → `TORSO_TOP_R`).

### Disco (tampa plana, ex: base de um pé, rodela)
```cpp
gluDisk(quad, RAIO_INTERNO, RAIO_EXTERNO, SLICES, 1);
```
- `RAIO_INTERNO = 0.0` → disco cheio. `RAIO_INTERNO > 0.0` → vira um anel (ex: cinto, argola).

### Cubo (cabeça, caixas, blocos)
```cpp
drawCube(MEIO_LADO);
```
- `MEIO_LADO`: metade do tamanho da aresta (ex: `0.28f` = cubo de 0.56 de lado).
- Já existe pronta no seu código (`drawCube`), com normais certas pra luz funcionar. É só chamar.
- Nasce **centrada** na origem atual (metade pra cada lado nos 3 eixos).

---

## Mudar cor

```cpp
glColor3f(R, G, B);
```
- `R`, `G`, `B`: valores de **0.0 a 1.0** (não é 0–255).
- Chame **antes** de cada `draw...()` — a cor vale pra tudo que for desenhado depois dela, até você chamar outro `glColor3f`.
- Exemplos prontos:
```cpp
glColor3f(1.0f, 0.0f, 0.0f); // vermelho
glColor3f(0.0f, 1.0f, 0.0f); // verde
glColor3f(0.0f, 0.0f, 1.0f); // azul
glColor3f(1.0f, 1.0f, 1.0f); // branco
glColor3f(0.0f, 0.0f, 0.0f); // preto
glColor3f(0.95f, 0.65f, 0.15f); // laranja (cor das juntas no seu código)
```

---

## Posicionar e girar (transformações)

Essas são a base de tudo — todo "osso" ou peça nova usa essa receita:

```cpp
glPushMatrix();                                  // salva a posição/rotação atual
    glTranslatef(DX, DY, DZ);                    // move o "cursor"
    glRotatef(ANGULO_GRAUS, EIXO_X, EIXO_Y, EIXO_Z); // gira em torno de um eixo (0 ou 1 em cada)
    // ... desenha algo aqui ...
glPopMatrix();                                   // volta pra posição/rotação salva
```
- `glPushMatrix()` / `glPopMatrix()`: **sempre em par**. Tudo que acontece entre eles não afeta o que vem depois do `glPopMatrix()`.
- `glRotatef(45.0f, 1.0f, 0.0f, 0.0f)` → gira 45° em torno do eixo **X** (balanço frente/trás — usado no seu robô pra ombro/cotovelo/quadril/joelho).
- `glRotatef(45.0f, 0.0f, 1.0f, 0.0f)` → gira em torno do eixo **Y** (giro tipo pião — usado na cintura).
- `glRotatef(45.0f, 0.0f, 0.0f, 1.0f)` → gira em torno do eixo **Z** (balanço lateral — evite nas juntas do corpo, é o que causava atravessar o tronco).
- Sem `glPushMatrix`/`glPopMatrix`, o `glTranslatef` "empurra" o cursor permanentemente — é assim que `drawLinkDown`/`drawLinkUp` fazem a próxima peça nascer already encaixada na ponta da anterior.

---

## Articulação (peça nova pendurada em uma junta)

Receita padrão pra adicionar um "osso" novo com uma junta (esfera) na base, igual o resto do robô:

```cpp
glColor3f(0.95f, 0.65f, 0.15f);
drawSphere(RAIO_JUNTA);              // a "bolinha" da articulação
glRotatef(anguloDaJunta, 1.0f, 0.0f, 0.0f); // eixo X = frente/trás (recomendado)

glColor3f(0.25f, 0.55f, 0.80f);
drawLinkDown(COMPRIMENTO, RAIO_OSSO); // o "osso", já empurra o cursor pra ponta dele
```
- Depois desse bloco, o cursor já está na **ponta** do osso — dá pra encadear outra junta + osso direto na sequência (é assim que perna e braço fazem: quadril→coxa→joelho→canela).
- Pra um membro que sai pro lado (braço, perna), lembra de deslocar primeiro:
```cpp
glPushMatrix();
    glTranslatef(lado * OFFSET, 0.0f, 0.0f); // lado = +1.0f ou -1.0f
    // ... junta + osso aqui ...
glPopMatrix();
```

### Limitar o ângulo de uma junta (não deixar "atravessar" o corpo)
```cpp
anguloDaJunta = clampf(anguloDaJunta, MIN, MAX);
```
- `clampf` já existe pronta no seu código.
- Regra prática: quanto mais perto do tronco a peça girar (`MAX` alto pra frente ou `MIN` baixo pra trás), maior a chance de atravessar — teste no jogo e vá reduzindo o range até parar de cruzar visualmente.

---

## Teclas — controle do robô

Padrão usado no código (dentro de `processInput`):

```cpp
if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) variavel += speed;
```
- Troca `GLFW_KEY_X` pela tecla que quiser (ex: `GLFW_KEY_1`, `GLFW_KEY_SPACE`, `GLFW_KEY_LEFT_SHIFT`).
- Troca `variavel` pelo ângulo/valor que essa tecla deve mexer.
- `speed` já é calculado como graus por segundo (`60.0f * dt`) — não precisa mexer.

Mapa atual do seu robô:

| Tecla | Ação |
|---|---|
| Q / A | Gira a cintura (esquerda / direita) |
| W / S | Ombro pra frente / pra trás |
| E / D | Dobra / estica o cotovelo |
| R / F | Quadril pra frente / pra trás |
| T / G | Dobra / estica o joelho |
| Setas | Move a câmera (órbita) |
| Espaço | Reseta a pose pro padrão |
| ESC | Fecha a janela |

Pra adicionar uma junta nova ao controle (ex: um pescoço que gira), o combo é sempre:
```cpp
// 1. variável global da pose (perto do topo do arquivo)
float pescocoAngle = 0.0f;

// 2. dentro de processInput()
if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) pescocoAngle += speed;
if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) pescocoAngle -= speed;
pescocoAngle = clampf(pescocoAngle, -30.0f, 30.0f);

// 3. usar no desenho, dentro da função da peça (ex: drawNeckAndHead)
glRotatef(pescocoAngle, 0.0f, 1.0f, 0.0f);
```

---

## Extras

### Adicionar uma peça nova ao robô (checklist rápido)
1. Declara as dimensões dela lá no topo do arquivo (junto das outras `const float`).
2. Escreve (ou reaproveita) uma função `drawAlgumaCoisa()` usando os blocos de "Formas geométricas" acima.
3. Chama essa função dentro de `drawRobot()` (ou dentro de outra parte, tipo `drawTorso()`), no lugar certo da hierarquia — **depois** do `glTranslatef`/`glRotatef` que a posiciona.
4. Se ela precisa se mexer, cria a variável de ângulo + tecla, seguindo o bloco de "Teclas" acima.

### Ordem de desenho importa
- Tudo que tá dentro de um `glPushMatrix() ... glPopMatrix()` herda a posição/rotação de tudo que veio **antes** dele, mas não afeta o que vem depois do `glPopMatrix()`.
- Isso é o que faz o braço girar junto com o ombro, mas a cabeça não ser afetada pelo braço.

### Cor de fundo da janela
```cpp
glClearColor(R, G, B, 1.0f); // dentro de display(), já existe
```

### Luz (já configurada no main(), raramente precisa mexer)
```cpp
GLfloat lightPos[] = { X, Y, Z, 1.0f }; // posição da luz no mundo
glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
```
- Só mexe se quiser mudar de onde a luz "vem" (ex: luz vindo de baixo, de trás, etc.)

### Testar rápido uma peça nova sem quebrar o robô
Comenta a chamada dela dentro de `drawRobot()` com `//` até terminar de ajustar tamanho/cor, depois descomenta.
