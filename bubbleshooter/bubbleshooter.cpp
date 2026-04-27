#include <GLUT/glut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

#define PI 3.14159265
using namespace std;

// --- Game Constants ---
const int width = 800, height = 600;
const int ROWS = 16;
const int COLS = 12;
const float radius = 20.0f;
const float gridOffsetX = 160.0f;
const float gridOffsetY = 50.0f;

enum GameState { MENU, PLAYING, GAMEOVER, LEVELUP };
GameState currentState = MENU;

struct Bubble
{
    float x, y;
    int color;
    bool active;
};

struct FallingBubble
{
    float x, y, vy;
    int color;
};

Bubble grid[ROWS][COLS];
vector<FallingBubble> fallingBubbles;
Bubble shot;
int nextColor;
int score = 0, highScore = 0;
int shotsTaken = 0, currentLevel = 1;
int maxShotsForLevel = 6; // Starts at 6, gets smaller

float shooterX = width / 2, shooterY = 40;
float angle = 90, speed = 15.0f;

float colors[5][3] =
{
    {1.0f, 0.2f, 0.2f}, {0.2f, 1.0f, 0.2f}, {0.2f, 0.5f, 1.0f}, {1.0f, 1.0f, 0.2f}, {0.8f, 0.2f, 1.0f}
};

// --- Utilities ---
void drawText(string str, int x, int y)
{
    glRasterPos2i(x, y);
    for (char c : str) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
}

void drawCircle(float cx, float cy, float r)
{
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= 30; i++)
    {
        float a = 2.0f * PI * i / 30.0f;
        glVertex2f(cx + cos(a) * r, cy + sin(a) * r);
    }
    glEnd();
}

// --- Logic ---
void initGrid()
{
    fallingBubbles.clear();
    shotsTaken = 0;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            float xOffset = (i % 2 == 0) ? 0 : radius;
            grid[i][j].x = j * 2 * radius + gridOffsetX + xOffset;
            grid[i][j].y = height - (i * 2 * radius + gridOffsetY);
            grid[i][j].active = (i < 4);
            grid[i][j].color = rand() % 5;
        }
    }
}

void dropCeiling()
{
    for (int i = ROWS - 1; i > 0; i--)
    {
        for (int j = 0; j < COLS; j++)
        {
            grid[i][j].active = grid[i - 1][j].active;
            grid[i][j].color = grid[i - 1][j].color;
        }
    }
    for (int j = 0; j < COLS; j++)
    {
        grid[0][j].active = true;
        grid[0][j].color = rand() % 5;
    }
    shotsTaken = 0;
}

void findMatches(int r, int c, int targetColor, vector<pair<int, int>>& matches)
{
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS || !grid[r][c].active || grid[r][c].color != targetColor) return;
    for (auto& m : matches) if (m.first == r && m.second == c) return;
    matches.push_back({r, c});
    int evenRow[6][2] = { {0,1}, {0,-1}, {1,0}, {-1,0}, {1,-1}, {-1,-1} };
    int oddRow[6][2] = { {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {-1,1} };
    for (int i = 0; i < 6; i++)
    {
        int nr = r + (r % 2 == 0 ? evenRow[i][0] : oddRow[i][0]);
        int nc = c + (r % 2 == 0 ? evenRow[i][1] : oddRow[i][1]);
        findMatches(nr, nc, targetColor, matches);
    }
}

void markConnected(int r, int c, bool connected[ROWS][COLS])
{
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS || !grid[r][c].active || connected[r][c]) return;
    connected[r][c] = true;
    int evenRow[6][2] = { {0,1}, {0,-1}, {1,0}, {-1,0}, {1,-1}, {-1,-1} };
    int oddRow[6][2] = { {0,1}, {0,-1}, {1,0}, {-1,0}, {1,1}, {-1,1} };
    for (int i = 0; i < 6; i++)
    {
        int nr = r + (r % 2 == 0 ? evenRow[i][0] : oddRow[i][0]);
        int nc = c + (r % 2 == 0 ? evenRow[i][1] : oddRow[i][1]);
        markConnected(nr, nc, connected);
    }
}

void dropOrphans()
{
    bool connected[ROWS][COLS] = {false};
    for (int j = 0; j < COLS; j++) if (grid[0][j].active) markConnected(0, j, connected);
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (grid[i][j].active && !connected[i][j])
            {
                fallingBubbles.push_back({grid[i][j].x, grid[i][j].y, -1.2f, grid[i][j].color});
                grid[i][j].active = false;
                score += 15;
            }
        }
    }
}

void checkLevelUp()
{
    if (score >= currentLevel * 500)
    {
        currentLevel++;
        currentState = LEVELUP;
        if (maxShotsForLevel > 2) maxShotsForLevel--; // Speed up drops
        speed += 1.5f; // Increase shot speed
    }
}

void handleCollision()
{
    int bestR = -1, bestC = -1;
    float minDist = 10000;
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            if (!grid[i][j].active)
            {
                float dx = shot.x - grid[i][j].x, dy = shot.y - grid[i][j].y;
                float dist = sqrt(dx*dx + dy*dy);
                if (dist < minDist)
                {
                    minDist = dist;
                    bestR = i;
                    bestC = j;
                }
            }
        }
    }
    if (bestR != -1)
    {
        grid[bestR][bestC].active = true;
        grid[bestR][bestC].color = shot.color;
        vector<pair<int, int>> matches;
        findMatches(bestR, bestC, grid[bestR][bestC].color, matches);
        if (matches.size() >= 3)
        {
            for (auto m : matches) grid[m.first][m.second].active = false;
            score += matches.size() * 10;
            dropOrphans();
            checkLevelUp();
        }
        else
        {
            shotsTaken++;
            if (shotsTaken >= maxShotsForLevel) dropCeiling();
        }
        for(int j=0; j<COLS; j++) if(grid[ROWS-2][j].active) currentState = GAMEOVER;
    }
    shot.active = false;
    if (score > highScore) highScore = score;
}

void drawFancyShooter(float x, float y, float ang)
{
    float rad = ang * PI / 180.0f, len = 75.0f, w = 16.0f;
    glColor3f(0.6f, 0.6f, 0.6f);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    float dotX = x, dotY = y;
    bool hitSomething = false;
    for(float d = 80; d < 600 && !hitSomething; d += 25)
    {
        dotX = x + cos(rad) * d;
        dotY = y + sin(rad) * d;
        if (dotX <= radius || dotX >= width - radius) break;
        for(int i=0; i<ROWS; i++)
        {
            for(int j=0; j<COLS; j++)
            {
                if(grid[i][j].active)
                {
                    float dist = sqrt(pow(dotX - grid[i][j].x, 2) + pow(dotY - grid[i][j].y, 2));
                    if(dist < radius * 1.8)
                    {
                        hitSomething = true;
                        break;
                    }
                }
            }
            if(hitSomething) break;
        }
        if(dotY >= height - radius) hitSomething = true;
        glVertex2f(dotX, dotY);
    }
    glEnd();
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(ang, 0, 0, 1);
    glBegin(GL_QUADS);
    glColor3f(0.2, 0.2, 0.2);
    glVertex2f(0, -w);
    glColor3f(0.5, 0.5, 0.5);
    glVertex2f(len, -w + 4);
    glColor3f(0.5, 0.5, 0.5);
    glVertex2f(len, w - 4);
    glColor3f(0.2, 0.2, 0.2);
    glVertex2f(0, w);
    glEnd();
    glBegin(GL_TRIANGLES);
    glColor3f(1, 1, 1);
    glVertex2f(len, w);
    glVertex2f(len + 15, 0);
    glVertex2f(len, -w);
    glEnd();
    glPopMatrix();
    glColor3f(0.1, 0.1, 0.1);
    drawCircle(x, y, 25);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    if (currentState == MENU)
    {
        glColor3f(1, 1, 1);
        drawText("BUBBLE SHOOTER ELITE", width/2 - 110, height/2 + 20);
        drawText("Press 'SPACE' to Start", width/2 - 90, height/2 - 20);
    }
    else if (currentState == LEVELUP)
    {
        glColor3f(1, 1, 0);
        drawText("LEVEL " + to_string(currentLevel) + " UNLOCKED!", width/2 - 100, height/2 + 20);
        drawText("Press 'C' to Continue", width/2 - 90, height/2 - 20);
    }
    else
    {
        for (int i = 0; i < ROWS; i++)
        {
            for (int j = 0; j < COLS; j++)
            {
                if (grid[i][j].active)
                {
                    glColor3fv(colors[grid[i][j].color]);
                    drawCircle(grid[i][j].x, grid[i][j].y, radius - 1);
                    glColor3f(1,1,1);
                    drawCircle(grid[i][j].x - 6, grid[i][j].y + 6, 4);
                }
            }
        }
        for (auto &fb : fallingBubbles)
        {
            glColor3fv(colors[fb.color]);
            drawCircle(fb.x, fb.y, radius - 1);
        }
        drawFancyShooter(shooterX, shooterY, angle);
        if (shot.active)
        {
            glColor3fv(colors[shot.color]);
            drawCircle(shot.x, shot.y, radius);
            glColor3f(1,1,1);
            drawCircle(shot.x - 6, shot.y + 6, 4);
        }
        glColor3f(1, 1, 1);
        drawText("SCORE: " + to_string(score), 25, height - 40);
        glColor3f(1, 0.8, 0);
        drawText("BEST: " + to_string(highScore), 25, height - 70);
        drawText("LEVEL: " + to_string(currentLevel), 25, height - 100);
        if (shotsTaken >= maxShotsForLevel - 1) glColor3f(1, 0, 0);
        else glColor3f(1, 1, 1);
        drawText("DROPS IN: " + to_string(maxShotsForLevel - shotsTaken), 25, 130);
        glBegin(GL_LINE_LOOP);
        glVertex2f(20, 30);
        glVertex2f(100, 30);
        glVertex2f(100, 110);
        glVertex2f(20, 110);
        glEnd();
        glColor3fv(colors[nextColor]);
        drawCircle(60, 65, radius - 2);
        if (currentState == GAMEOVER)
        {
            glColor3f(1, 0, 0);
            drawText("GAME OVER!", width/2 - 55, height/2);
            glColor3f(1, 1, 1);
            drawText("R to RESTART", width/2 - 60, height/2 - 40);
        }
    }
    glutSwapBuffers();
}

void update(int value)
{
    if (currentState == PLAYING)
    {
        if (shot.active)
        {
            shot.x += cos(angle * PI / 180) * speed;
            shot.y += sin(angle * PI / 180) * speed;
            if (shot.x <= radius || shot.x >= width - radius) angle = 180 - angle;
            if (shot.y >= height - radius) handleCollision();
            else
            {
                for (int i = 0; i < ROWS; i++)
                {
                    for (int j = 0; j < COLS; j++)
                    {
                        if (grid[i][j].active)
                        {
                            float dx = shot.x - grid[i][j].x, dy = shot.y - grid[i][j].y;
                            if (sqrt(dx*dx + dy*dy) < radius * 1.8)
                            {
                                handleCollision();
                                goto skip;
                            }
                        }
                    }
                }
            }
        }
        for (int i = 0; i < (int)fallingBubbles.size(); i++)
        {
            fallingBubbles[i].y += fallingBubbles[i].vy;
            fallingBubbles[i].vy -= 0.4f;
            if (fallingBubbles[i].y < -20)
            {
                fallingBubbles.erase(fallingBubbles.begin() + i);
                i--;
            }
        }
    }
skip:
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    if (key == ' ' && currentState == MENU) currentState = PLAYING;
    else if (key == ' ' && currentState == PLAYING && !shot.active)
    {
        shot.active = true;
        shot.x = shooterX;
        shot.y = shooterY;
        shot.color = nextColor;
        nextColor = rand() % 5;
    }
    else if ((key == 'r' || key == 'R') && currentState == GAMEOVER)
    {
        score = 0;
        currentLevel = 1;
        maxShotsForLevel = 6;
        speed = 15.0f;
        initGrid();
        currentState = PLAYING;
    }
    else if ((key == 'c' || key == 'C') && currentState == LEVELUP)
    {
        currentState = PLAYING;
    }
}

void specialKeys(int key, int x, int y)
{
    if (key == GLUT_KEY_LEFT && angle < 170) angle += 5;
    if (key == GLUT_KEY_RIGHT && angle > 10) angle -= 5;
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Bubble Shooter Elite - Level System");
    glClearColor(0.05, 0.05, 0.08, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    srand(time(0));
    nextColor = rand() % 5;
    initGrid();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, update, 0);
    glutMainLoop();
    return 0;
}