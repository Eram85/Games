#include <GLUT/glut.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cctype>

using namespace std;

/* ---------------- GAME VARIABLES ---------------- */

int gameState = 0;

string secretWord = "";
string currentGuess = "";

vector<string> guesses;

int maxGuesses = 8; // Player gets 8 chances
bool gameOver = false;

int score = 0;

string message = "ENTER SECRET 5-LETTER WORD:";

/* ---------------- ANIMATION VARIABLES ---------------- */

int animatingRow = -1;
int animatingCol = -1;

float flipScale = 1.0f;
bool isShrinking = true;

float winScale = 1.0f;
float winTimer = 0.0f;

/* ---------------- KEYBOARD & COLORS ---------------- */

map<char, int> keyStates;

string keyboardRows[] = {
    "QWERTYUIOP",
    "ASDFGHJKL",
    "ZXCVBNM"
};

float green[] = {0.32f, 0.55f, 0.31f};
float yellow[] = {0.71f, 0.62f, 0.23f};
float grey[] = {0.25f, 0.25f, 0.25f};
float darkBox[] = {0.1f, 0.1f, 0.1f};
float defaultKey[] = {0.4f, 0.4f, 0.4f};

/* ---------------- TEXT DRAWING ---------------- */

void drawText(float x, float y, string text,
              void* font = GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x, y);

    for(char c : text)
        glutBitmapCharacter(font, c);
}

/* ---------------- DRAW BOX ---------------- */

void drawBox(float x, float y,
             float w, float h,
             float r, float g, float b,
             char letter,
             float scaleY = 1.0f,
             bool isKey = false)
{
    float centerY = y - h / 2.0f;
    float scaledH = h * scaleY;

    glColor3f(r, g, b);

    glBegin(GL_QUADS);

    glVertex2f(x, centerY + scaledH / 2.0f);
    glVertex2f(x + w, centerY + scaledH / 2.0f);
    glVertex2f(x + w, centerY - scaledH / 2.0f);
    glVertex2f(x, centerY - scaledH / 2.0f);

    glEnd();

    glColor3f(0.3f, 0.3f, 0.3f);

    glBegin(GL_LINE_LOOP);

    glVertex2f(x, centerY + scaledH / 2.0f);
    glVertex2f(x + w, centerY + scaledH / 2.0f);
    glVertex2f(x + w, centerY - scaledH / 2.0f);
    glVertex2f(x, centerY - scaledH / 2.0f);

    glEnd();

    if(scaleY > 0.1f)
    {
        glColor3f(1, 1, 1);

        string s(1, letter);

        float xOffset = isKey ? w / 3.5f : w / 3.0f;
        float yOffset = isKey ?
                        h / 1.6f - (h * (1 - scaleY) / 2) :
                        h / 1.5f - (h * (1 - scaleY) / 2);

        drawText(x + xOffset, y - yOffset, s,
                 isKey ? GLUT_BITMAP_HELVETICA_12
                       : GLUT_BITMAP_HELVETICA_18);
    }
}

/* ---------------- TIMER ---------------- */

void timer(int value)
{
    if(animatingCol != -1)
    {
        if(isShrinking)
        {
            flipScale -= 0.15f;

            if(flipScale <= 0.0f)
            {
                flipScale = 0.0f;
                isShrinking = false;
            }
        }
        else
        {
            flipScale += 0.15f;

            if(flipScale >= 1.0f)
            {
                flipScale = 1.0f;

                isShrinking = true;

                animatingCol++;

                if(animatingCol >= 5)
                {
                    animatingCol = -1;
                    animatingRow = -1;
                }
            }
        }
    }

    if(gameOver && message.find("WINNER") != string::npos)
    {
        winTimer += 0.12f;
        winScale = 1.0f + 0.1f * sin(winTimer);
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

/* ---------------- UPDATE KEY COLORS ---------------- */

void updateKeyStates(string guess)
{
    for(int i = 0; i < 5; i++)
    {
        char c = guess[i];

        int state;

        if(c == secretWord[i])
            state = 3;
        else if(secretWord.find(c) != string::npos)
            state = 2;
        else
            state = 1;

        if(state > keyStates[c])
            keyStates[c] = state;
    }
}

/* ---------------- DISPLAY ---------------- */

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    /* ---------- START SCREEN ---------- */

    if(gameState == 0)
    {
        glColor3f(0.0f, 0.8f, 0.4f);

        drawText(-0.2f, 0.75f,
                 "MINI WORDLE",
                 GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(1, 1, 1);

        drawText(-0.45f, 0.55f, message);

        float boxW = 0.15f;
        float gap = 0.03f;

        float totalW = 5 * boxW + 4 * gap;
        float startX = -totalW / 2.0f;

        for(int i = 0; i < 5; i++)
        {
            char ch = (i < secretWord.length()) ? '*' : ' ';

            drawBox(startX + i * (boxW + gap),
                    0.3f,
                    boxW,
                    boxW,
                    0.15f,
                    0.15f,
                    0.15f,
                    ch);
        }

        glColor3f(0.6f, 0.6f, 0.6f);

        drawText(-0.42f,
                 -0.05f,
                 "Type secret word and press ENTER",
                 GLUT_BITMAP_HELVETICA_12);
    }

    /* ---------- GAME SCREEN ---------- */

    else
    {
        glColor3f(1, 1, 1);

        drawText(0.55f, 0.9f,
                 "SCORE: " + to_string(score));

        float boxS = 0.14f;
        float gapS = 0.025f;

        float totalW = 5 * boxS + 4 * gapS;
        float startX = -totalW / 2.0f;

        float startY = 0.88f;

        /* ---------- DRAW GRID ---------- */

        for(int r = 0; r < maxGuesses; r++)
        {
            string rowWord = "";

            if(r < guesses.size())
                rowWord = guesses[r];
            else if(r == guesses.size())
                rowWord = currentGuess;

            for(int c = 0; c < 5; c++)
            {
                float* col = darkBox;
                float scale = 1.0f;

                if(r < guesses.size())
                {
                    if(rowWord[c] == secretWord[c])
                        col = green;
                    else if(secretWord.find(rowWord[c]) != string::npos)
                        col = yellow;
                    else
                        col = grey;

                    if(r == animatingRow && c == animatingCol)
                    {
                        scale = flipScale;

                        if(isShrinking)
                            col = darkBox;
                    }
                }

                char letter =
                    (c < rowWord.length()) ? rowWord[c] : ' ';

                drawBox(startX + c * (boxS + gapS),
                        startY - r * (boxS + gapS),
                        boxS,
                        boxS,
                        col[0],
                        col[1],
                        col[2],
                        letter,
                        scale);
            }
        }

        /* ---------- KEYBOARD ---------- */

        float ky = -0.45f;
        float kw = 0.08f;
        float kh = 0.1f;
        float kGap = 0.015f;

        for(int i = 0; i < 3; i++)
        {
            int rowLen = keyboardRows[i].length();

            float rowW = rowLen * kw + (rowLen - 1) * kGap;

            float kx = -rowW / 2.0f;

            for(char c : keyboardRows[i])
            {
                float* col = defaultKey;

                if(keyStates[c] == 1)
                    col = grey;
                else if(keyStates[c] == 2)
                    col = yellow;
                else if(keyStates[c] == 3)
                    col = green;

                drawBox(kx,
                        ky - i * (kh + kGap),
                        kw,
                        kh,
                        col[0],
                        col[1],
                        col[2],
                        c,
                        1.0f,
                        true);

                kx += kw + kGap;
            }
        }

        /* ---------- GAME OVER ---------- */

        if(gameOver)
        {
            if(message.find("WINNER") != string::npos)
            {
                glPushMatrix();

                glTranslatef(0.0f, -0.9f, 0.0f);
                glScalef(winScale, winScale, 1.0f);

                glColor3f(0.3f, 1.0f, 0.3f);

                drawText(-0.35f,
                         0.0f,
                         message);

                glPopMatrix();
            }
            else
            {
                glColor3f(1.0f, 0.3f, 0.3f);

                drawText(-0.35f,
                         -0.9f,
                         message);
            }
        }
        else
        {
            glColor3f(1, 1, 1);

            drawText(-0.2f,
                     -0.9f,
                     message);
        }
    }

    glutSwapBuffers();
}

/* ---------------- KEYBOARD INPUT ---------------- */

void keyboard(unsigned char key, int x, int y)
{
    key = toupper(key);

    /* ---------- SECRET WORD INPUT ---------- */

    if(gameState == 0)
    {
        if(key == 13 && secretWord.length() == 5)
        {
            gameState = 1;
            message = "GUESS THE WORD!";
        }

        else if((key == 8 || key == 127) &&
                !secretWord.empty())
        {
            secretWord.pop_back();
        }

        else if(secretWord.length() < 5 &&
                isalpha(key))
        {
            secretWord += key;
        }
    }

    /* ---------- GAMEPLAY ---------- */

    else
    {
        if(gameOver || animatingCol != -1)
            return;

        if(key == 13 && currentGuess.length() == 5)
        {
            animatingRow = guesses.size();
            animatingCol = 0;

            flipScale = 1.0f;
            isShrinking = true;

            updateKeyStates(currentGuess);

            guesses.push_back(currentGuess);

            /* ---------- WIN ---------- */

            if(currentGuess == secretWord)
            {
                score = 100 - (guesses.size() - 1) * 10;

                if(score < 10)
                    score = 10;

                message = "WINNER! SCORE: "
                          + to_string(score);

                gameOver = true;
            }

            /* ---------- LOSE ---------- */

            else if(guesses.size() >= maxGuesses)
            {
                message = "YOU LOST! WORD: "
                          + secretWord;

                gameOver = true;
            }

            currentGuess = "";
        }

        else if((key == 8 || key == 127) &&
                !currentGuess.empty())
        {
            currentGuess.pop_back();
        }

        else if(currentGuess.length() < 5 &&
                isalpha(key))
        {
            currentGuess += key;
        }
    }

    glutPostRedisplay();
}

/* ---------------- OPENGL SETUP ---------------- */

void init()
{
    glClearColor(0, 0, 0, 1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluOrtho2D(-1, 1, -1, 1);
}

/* ---------------- MAIN ---------------- */

int main(int argc, char** argv)
{
    for(char c = 'A'; c <= 'Z'; c++)
        keyStates[c] = 0;

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    glutInitWindowSize(1000, 900);

    glutCreateWindow("Mini Wordle");

    init();

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutTimerFunc(16, timer, 0);

    glutMainLoop();

    return 0;
}