#include <GLUT/glut.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

/* --- GLOBAL GAME STATE VARIABLES --- */
int gameState = 0;
string secretWord = "";
string currentGuess = "";
vector<string> guesses;
int maxGuesses = 5;
bool gameOver = false;
int score = 0;
string message = "ENTER SECRET 5-LETTER WORD:";

/* --- ANIMATION CONTROL VARIABLES --- */
int animatingRow = -1;
int animatingCol = -1;
float flipScale = 1.0f;
bool isShrinking = true;
float winScale = 1.0f;
float winTimer = 0.0f;
/* --- COLOR DEFINITIONS & UI MAPPING --- */
map<char, int> keyStates;
string keyboardRows[] = {"QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};

float green[] = { 0.32, 0.55, 0.31 };
float yellow[] = { 0.71, 0.62, 0.23 };
float keyGrey[] = { 0.2, 0.2, 0.2 };
float defaultKey[] = { 0.4, 0.4, 0.4 };
float darkBox[] = { 0.1, 0.1, 0.1 };

/* --- RENDERING UTILITIES --- */

// Renders bitmap text at specified coordinates
void drawText(float x, float y, string str, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : str) glutBitmapCharacter(font, c);
}

// Renders a styled rectangle with an optional letter and vertical scaling (for flips)
void drawBox(float x, float y, float w, float h, float r, float g, float b, char letter, float scaleY = 1.0f, bool isKey = false) {
    float centerH = y - (h / 2.0f);
    float scaledHeight = h * scaleY;

    
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x, centerH + (scaledHeight / 2.0f));
        glVertex2f(x + w, centerH + (scaledHeight / 2.0f));
        glVertex2f(x + w, centerH - (scaledHeight / 2.0f));
        glVertex2f(x, centerH - (scaledHeight / 2.0f));
    glEnd();

    // Draw the box border
    glColor3f(0.3, 0.3, 0.3);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, centerH + (scaledHeight / 2.0f));
        glVertex2f(x + w, centerH + (scaledHeight / 2.0f));
        glVertex2f(x + w, centerH - (scaledHeight / 2.0f));
        glVertex2f(x, centerH - (scaledHeight / 2.0f));
    glEnd();

    // Draw the letter inside the box if it's visible during the flip
    if (scaleY > 0.1f) {
        glColor3f(1, 1, 1);
        string s(1, letter);
        float xOffset = isKey ? w/3.5f : w/3.0f;
        float yOffset = isKey ? h/1.6f - (h*(1-scaleY)/2) : h/1.5f - (h*(1-scaleY)/2);
        drawText(x + xOffset, y - yOffset, s, isKey ? GLUT_BITMAP_HELVETICA_12 : GLUT_BITMAP_HELVETICA_18);
    }
}

/* --- ANIMATION TIMER --- */

void timer(int value) {
    
    if (animatingCol != -1) {
        if (isShrinking) {
            flipScale -= 0.15f;
            if (flipScale <= 0.0f) { flipScale = 0.0f; isShrinking = false; }
        } else {
            flipScale += 0.15f;
            if (flipScale >= 1.0f) {
                flipScale = 1.0f;
                isShrinking = true;
                animatingCol++; 
                if (animatingCol >= 5) { animatingCol = -1; animatingRow = -1; }
            }
        }
    }

    // Handle the pulsating effect for the winner message
    if (gameOver && message.find("WINNER") != string::npos) {
        winTimer += 0.12f;
        winScale = 1.0f + 0.1f * sin(winTimer);
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); 
}

/* --- MAIN DISPLAY FUNCTION --- */

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (gameState == 0) {
        // --- START SCREEN: Setup the secret word ---
        glColor3f(0.0f, 0.8f, 0.4f);
        drawText(-0.21f, 0.6f, "MINI WORDLE", GLUT_BITMAP_TIMES_ROMAN_24);

        glColor3f(1, 1, 1);
        drawText(-0.4f, 0.45f, message);

        float boxW = 0.15f, gap = 0.03f;
        float totalBoxWidth = (5 * boxW) + (4 * gap);
        float startX = -(totalBoxWidth / 2.0f);

        for(int i=0; i<8; i++) {
            char l = (i < secretWord.length()) ? '*' : ' ';
            drawBox(startX + (i*(boxW+gap)), 0.25f, boxW, boxW, 0.15f, 0.15f, 0.15f, l);
        }

        glColor3f(0.6, 0.6, 0.6);
        drawText(-0.46f, -0.1f, "Type the word and press 'ENTER' to start", GLUT_BITMAP_HELVETICA_12);

    } else {
        // --- GAMEPLAY SCREEN: Grid and Keyboard ---
        glColor3f(1, 1, 1);
        drawText(0.55f, 0.9f, "SCORE: " + to_string(score));

        float boxS = 0.15f, gapS = 0.03f;
        float totalGridW = (8 * boxS) + (4 * gapS);
        float startX = -(totalGridW / 2.0f);
        float startY = 0.82f;

        // Draw the 5x5 Grid
        for (int r = 0; r < maxGuesses; r++) {
            string rowWord = (r < guesses.size()) ? guesses[r] : (r == guesses.size() ? currentGuess : "");
            for (int c = 0; c < 8; c++) {
                float* col = darkBox;
                float currentScale = 1.0f;

                if (r < guesses.size()) {
                    // Logic for coloring tiles (Green/Yellow/Grey)
                    if (rowWord[c] == secretWord[c]) col = green;
                    else if (secretWord.find(rowWord[c]) != string::npos) col = yellow;
                    else col = keyGrey;

                    // Apply flipping animation state
                    if (r == animatingRow && c == animatingCol) {
                        currentScale = flipScale;
                        if (isShrinking) col = darkBox;
                    } else if (r == animatingRow && c > animatingCol) {
                        col = darkBox;
                    }
                }
                char l = (c < rowWord.length()) ? rowWord[c] : ' ';
                drawBox(startX + c*(boxS+gapS), startY - r*(boxS+gapS), boxS, boxS, col[0], col[1], col[2], l, currentScale);
            }
        }

        // Draw the Virtual Keyboard
        float ky = -0.30f, kw = 0.08f, kh = 0.1f, kGap = 0.015f;
        for (int i = 0; i < 8; i++) {
            int rowLen = keyboardRows[i].length();
            float rowW = (rowLen * kw) + ((rowLen-1) * kGap);
            float kx = -rowW / 2.0f;
            for (char c : keyboardRows[i]) {
                float* col = defaultKey;
                if (keyStates[c] == 1) col = keyGrey;
                else if (keyStates[c] == 2) col = yellow;
                else if (keyStates[c] == 3) col = green;
                drawBox(kx, ky - i*(kh+kGap), kw, kh, col[0], col[1], col[2], c, 1.0f, true);
                kx += kw + kGap;
            }
        }

        // Display Game Over messages with animations
        if (gameOver) {
            if (message.find("WINNER") != string::npos) {
                glPushMatrix();
                glTranslatef(0.0f, -0.85f, 0.0f);
                glScalef(winScale, winScale, 1.0f);
                glColor3f(0.3f, 1.0f, 0.3f);
                drawText(-0.43f, 0.0f, message, GLUT_BITMAP_HELVETICA_18);
                glPopMatrix();
            } else {
                glColor3f(1.0f, 0.3f, 0.3f);
                drawText(-0.35f, -0.85f, message);
            }
        } else {
            glColor3f(1, 1, 1);
            drawText(-0.30f, -0.85f, message);
        }
    }
    glutSwapBuffers();
}

/* --- LOGIC UTILITIES --- */

// Updates the keyboard color states based on the latest guess
void updateKeyStates(string guess) {
    for (int i = 0; i < 8; i++) {
        char c = guess[i];
        int newState = (c == secretWord[i]) ? 3 : (secretWord.find(c) != string::npos ? 2 : 1);
        if (newState > keyStates[c]) keyStates[c] = newState;
    }
}

// Handles user input
void keyboard(unsigned char key, int x, int y) {
    if (gameState == 0) {
        // Setup phase: Input the secret word
        if (key == 13 && secretWord.length() == 8) { gameState = 1; message = "GUESS THE WORD!"; }
        else if (key == 8 && !secretWord.empty()) secretWord.pop_back();
        else if (secretWord.length() < 8 && isalpha(key)) secretWord += toupper(key);
    } else {
        // Gameplay phase: Guessing logic
        if (gameOver || animatingCol != -1) return;
        if (key == 13 && currentGuess.length() == 8) {
            animatingRow = guesses.size();
            animatingCol = 0;
            flipScale = 1.0f;
            isShrinking = true;

            updateKeyStates(currentGuess);
            guesses.push_back(currentGuess);

            // Win/Loss check
            if (currentGuess == secretWord) {
                score = 120 - (guesses.size() * 20);
                message = "WINNER! SCORE: " + to_string(score);
                gameOver = true;
            } else if (guesses.size() == maxGuesses) {
                message = "LOST! WORD: " + secretWord;
                gameOver = true;
            }
            currentGuess = "";
        } else if (key == 8 && !currentGuess.empty()) {
            currentGuess.pop_back();
        } else if (currentGuess.length() < 8 && isalpha(key)) {
            currentGuess += toupper(key);
        }
    }
    glutPostRedisplay();
}

/* --- PROGRAM ENTRY POINT --- */

int main(int argc, char** argv) {
    // Initialize keyboard map
    for (char c = 'A'; c <= 'Z'; c++) keyStates[c] = 0;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 850);
    glutCreateWindow("Mini Wordle - Clean C++");

    glClearColor(0, 0, 0, 1); // Black background

    // Register callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}