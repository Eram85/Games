#include <GLUT/glut.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

const int WIDTH = 800;
const int HEIGHT = 600;
const int TARGET_SCORE = 50;


int shipX = 380;
int lives = 1;
int score = 0;

bool startGame = false;
bool gameOver = false;
bool playerWin = false;


bool bulletActive = false;
float bulletX, bulletY;

int i;


struct EnemyBullet
{
    float x, y;
    bool active;
};

const int MAX_ENEMY_BULLETS = 1;
EnemyBullet enemyBullets[MAX_ENEMY_BULLETS];

struct Alien
{
    float x, y;
    float dir;
};

const int MAX_ALIENS = 8;
Alien aliens[MAX_ALIENS];

void drawText(float x, float y, const char* str)
{
    glRasterPos2f(x, y);
    for (i = 0; str[i] != '\0'; i++)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, str[i]);
    }
}


void resetAlien(int i)
{
    aliens[i].x = rand() % 740;
    aliens[i].y = 450 + rand() % 120;
    aliens[i].dir = (rand() % 2 == 0) ? 2 : -2;
}

void initGame()
{
    shipX = 380;
    lives = 1;
    score = 0;

    startGame = true;
    gameOver = false;
    playerWin = false;

    bulletActive = false;

    for (i = 0; i < MAX_ALIENS; i++)
    {
        resetAlien(i);
    }

    for (i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        enemyBullets[i].active = false;
    }
}


void drawShip()
{
       glColor3f(0, 1, 1);

       glBegin(GL_TRIANGLES);
        glVertex2f(shipX, 20);
        glVertex2f(shipX + 40, 20);
        glVertex2f(shipX + 20, 60);
        glEnd();
}

void drawBullet()
{
    if (!bulletActive)
    {
        return;
    }

    glColor3f(1, 1, 0);

    glBegin(GL_QUADS);
        glVertex2f(bulletX, bulletY);
        glVertex2f(bulletX + 4, bulletY);
        glVertex2f(bulletX + 4, bulletY + 15);
        glVertex2f(bulletX, bulletY + 15);
    glEnd();
}

void drawEnemyBullets()
{
    glColor3f(1, 0, 0);

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
    {
        if (enemyBullets[i].active)
            {
            glBegin(GL_QUADS);
                glVertex2f(enemyBullets[i].x, enemyBullets[i].y);
                glVertex2f(enemyBullets[i].x + 4, enemyBullets[i].y);
                glVertex2f(enemyBullets[i].x + 4, enemyBullets[i].y + 10);
                glVertex2f(enemyBullets[i].x, enemyBullets[i].y + 10);
            glEnd();
        }
    }
}

void drawAlien(Alien a)
{
    glColor3f(0, 1, 0);

    glBegin(GL_QUADS);
        glVertex2f(a.x, a.y);
        glVertex2f(a.x + 40, a.y);
        glVertex2f(a.x + 40, a.y + 30);
        glVertex2f(a.x, a.y + 30);
    glEnd();
}


void enemyShoot(float x, float y)
{
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (!enemyBullets[i].active) {
            enemyBullets[i].active = true;
            enemyBullets[i].x = x + 18;
            enemyBullets[i].y = y;
            break;
        }
    }
}

void update(int v)
{
    if (startGame && !gameOver && !playerWin)
    {

        if (bulletActive)
        {
            bulletY += 12;
            if (bulletY > HEIGHT)
                bulletActive = false;
        }
        for (i = 0; i < MAX_ALIENS; i++)
        {
            aliens[i].x += aliens[i].dir;

            if (aliens[i].x < 0 || aliens[i].x > 760)
                aliens[i].dir *= -1;

            if (rand() % 200 == 0)
                enemyShoot(aliens[i].x, aliens[i].y);

            if (bulletActive &&
                bulletX > aliens[i].x &&
                bulletX < aliens[i].x + 40 &&
                bulletY > aliens[i].y &&
                bulletY < aliens[i].y + 30)
            {
                score += 10;
                bulletActive = false;
                resetAlien(i);

                if (score >= TARGET_SCORE)
                    playerWin = true;
            }
        }

        for (i = 0; i < MAX_ENEMY_BULLETS; i++)
        {
            if (enemyBullets[i].active)
            {
                enemyBullets[i].y -= 8;

                if (enemyBullets[i].y < 0)
                    enemyBullets[i].active = false;


                if (enemyBullets[i].x > shipX &&
                    enemyBullets[i].x < shipX + 40 &&
                    enemyBullets[i].y > 20 &&
                    enemyBullets[i].y < 60)
                {
                    enemyBullets[i].active = false;
                    lives--;

                    if (lives <= 0)
                        gameOver = true;
                }
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == 13)
    {
        initGame();
    }

    if (key == ' ' && startGame && !gameOver && !playerWin)
    {
        if (!bulletActive) {
            bulletActive = true;
            bulletX = shipX + 18;
            bulletY = 60;
        }
    }
}

void specialKeys(int key, int x, int y)
{
    if (!startGame || gameOver || playerWin)
        return;

    if (key == GLUT_KEY_LEFT)
        shipX -= 20;

    if (key == GLUT_KEY_RIGHT)
        shipX += 20;

    if (shipX < 0) shipX = 0;
    if (shipX > 760) shipX = 760;
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (!startGame)
    {
        drawText(280, 320, "SPACE SHOOTER");
        drawText(240, 280, "Press ENTER to Start");
    }
    else if (playerWin)
    {
        drawText(300, 320, "PLAYER WIN!");
        drawText(260, 280, "Score reached 50");
        drawText(220, 240, "Press ENTER to restart");
    }
    else if (gameOver)
    {
        drawText(300, 320, "ALIEN WIN!");
        drawText(260, 280, "Lives finished");
        drawText(220, 240, "Press ENTER to restart");
    }
    else
    {
        drawShip();
        drawBullet();
        drawEnemyBullets();

        for (int i = 0; i < MAX_ALIENS; i++)
            drawAlien(aliens[i]);

        char info[100];
        sprintf(info, "Score: %d  Lives: %d", score, lives);
        drawText(10, 570, info);
    }

    glutSwapBuffers();
}

void init()
{
    glClearColor(0, 0, 0, 1);
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
}

int main(int argc, char** argv)
{
    srand(time(0));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Space Shooter");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}