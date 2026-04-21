#include <GLUT/glut.h>
#include <cmath>
#include <vector>
#include <sstream>
#include <string>

// -------- SOUND FUNCTION (MAC) --------
void playSound(const char* file){
    std::string cmd = "afplay ";
    cmd += file;
    cmd += " &";
    system(cmd.c_str());
}

// Window
const int WIDTH = 800, HEIGHT = 600;

// Ball
float ballX, ballY, ballVX = 0, ballVY = 0;
float radius = 10;

// Game
int strokes = 0;
int level = 1;
int timeLeft = 30;
int frameCount = 0;

// Mouse
bool aiming = false;
float aimX, aimY;

// State
enum State { MENU, PLAY, WIN };
State gameState = MENU;

// Hole
float holeX, holeY, holeR = 15;

// Obstacles
struct Rect { float x,y,w,h; };
std::vector<Rect> walls;
Rect sand, water, ice, lava;

// -------- DRAW --------
void drawCircle(float cx,float cy,float r){
    glBegin(GL_TRIANGLE_FAN);
    for(int i=0;i<50;i++){
        float a=2*M_PI*i/50;
        glVertex2f(cx+r*cos(a),cy+r*sin(a));
    }
    glEnd();
}

void drawText(float x,float y,std::string s){
    glRasterPos2f(x,y);
    for(char c:s)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,c);
}

// -------- LEVELS --------
void loadLevel(int lvl){
    walls.clear();
    sand = water = ice = lava = {0,0,0,0};

    strokes = 0;
    timeLeft = 30;
    frameCount = 0;

    if(lvl == 1){
        ballX=100; ballY=100;
        holeX=600; holeY=400;
        walls.push_back({300,300,200,20});
        sand = {200,100,150,100};
    }
    else if(lvl == 2){
        ballX=100; ballY=100;
        holeX=700; holeY=500;
        water = {400,200,200,200};
        walls.push_back({300,400,300,20});
    }
    else if(lvl == 3){
        ballX=100; ballY=500;
        holeX=700; holeY=200;
        ice = {200,200,400,200};
    }
    else if(lvl == 4){
        ballX=50; ballY=50;
        holeX=700; holeY=500;
        walls.push_back({200,0,20,500});
        walls.push_back({400,100,20,500});
        walls.push_back({600,0,20,500});
    }
    else if(lvl == 5){
        ballX=100; ballY=100;
        holeX=700; holeY=500;
        lava = {300,200,300,200};
        walls.push_back({300,400,200,20});
    }

    ballVX = ballVY = 0;
}

// -------- COLLISION --------
void handleCollisions(){

    for(auto &w:walls){
        if(ballX>w.x && ballX<w.x+w.w &&
           ballY>w.y && ballY<w.y+w.h){
            ballVX *= -0.8;
            ballVY *= -0.8;
            playSound("/System/Library/Sounds/Pop.aiff");
        }
    }

    if(ballX>sand.x && ballX<sand.x+sand.w &&
       ballY>sand.y && ballY<sand.y+sand.h){
        ballVX *= 0.9;
        ballVY *= 0.9;
    }

    if(ballX>ice.x && ballX<ice.x+ice.w &&
       ballY>ice.y && ballY<ice.y+ice.h){
        ballVX *= 0.995;
        ballVY *= 0.995;
    }

    if(ballX>water.x && ballX<water.x+water.w &&
       ballY>water.y && ballY<water.y+water.h){
        playSound("/System/Library/Sounds/Sosumi.aiff");
        loadLevel(level);
    }

    if(ballX>lava.x && ballX<lava.x+lava.w &&
       ballY>lava.y && ballY<lava.y+lava.h){
        playSound("/System/Library/Sounds/Sosumi.aiff");
        loadLevel(level);
    }
}

// -------- UPDATE --------
void update(int v){

    if(gameState==PLAY){

        ballX+=ballVX;
        ballY+=ballVY;

        ballVX*=0.98;
        ballVY*=0.98;

        if(fabs(ballVX)<0.01) ballVX=0;
        if(fabs(ballVY)<0.01) ballVY=0;

        if(ballX < 0) ballX = 0;
        if(ballY < 0) ballY = 0;
        if(ballX > WIDTH) ballX = WIDTH;
        if(ballY > HEIGHT) ballY = HEIGHT;

        handleCollisions();

        frameCount++;
        if(frameCount % 60 == 0){
            timeLeft--;
        }

        if(timeLeft <= 0){
            loadLevel(level);
        }

        float dx=ballX-holeX, dy=ballY-holeY;
        if(sqrt(dx*dx+dy*dy)<holeR){
            playSound("/System/Library/Sounds/Glass.aiff");
            level++;

            if(level > 5){
                gameState = WIN;
            } else {
                loadLevel(level);
            }
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16,update,0);
}

// -------- INPUT --------
void mouse(int btn,int state,int x,int y){

    y = HEIGHT - y;

    if(gameState==MENU){
        level = 1;
        loadLevel(level);
        gameState=PLAY;
        return;
    }

    if(gameState==WIN){
        gameState=MENU;
        return;
    }

    if(btn==GLUT_LEFT_BUTTON && state==GLUT_DOWN){
        aiming = true;
        aimX = x;
        aimY = y;
    }

    if(btn==GLUT_LEFT_BUTTON && state==GLUT_UP){
        aiming = false;

        float dx=ballX-aimX;
        float dy=ballY-aimY;
        float power=sqrt(dx*dx+dy*dy);

        if(power < 5) return;
        if(power > 150) power = 150;

        ballVX=(dx/power)*(power*0.2);
        ballVY=(dy/power)*(power*0.2);

        strokes++;
        playSound("/System/Library/Sounds/Pop.aiff");
    }
}

void motion(int x,int y){
    y = HEIGHT - y;
    if(aiming){
        aimX = x;
        aimY = y;
    }
}

// -------- DISPLAY --------
void display(){

    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if(gameState==MENU){
        drawText(300,300,"MINI GOLF - 5 LEVELS");
        drawText(280,250,"Click to Start");
    }
    else if(gameState==WIN){
        drawText(300,300,"YOU WON!");
        drawText(280,250,"Click to Restart");
    }
    else{
        glColor3f(0.3,0.8,0.3);
        glRectf(0,0,WIDTH,HEIGHT);

        glColor3f(0.9,0.8,0.5);
        glRectf(sand.x,sand.y,sand.x+sand.w,sand.y+sand.h);

        glColor3f(0.2,0.4,1);
        glRectf(water.x,water.y,water.x+water.w,water.y+water.h);

        glColor3f(0.8,0.9,1);
        glRectf(ice.x,ice.y,ice.x+ice.w,ice.y+ice.h);

        glColor3f(1,0.2,0);
        glRectf(lava.x,lava.y,lava.x+lava.w,lava.y+lava.h);

        glColor3f(0.5,0.3,0.1);
        for(auto &w:walls)
            glRectf(w.x,w.y,w.x+w.w,w.y+w.h);

        glColor3f(0,0,0);
        drawCircle(holeX,holeY,holeR);

        glColor3f(0,0,1);
        drawCircle(ballX,ballY,radius);

        if(aiming){
            glColor3f(1,0,0);
            glBegin(GL_LINES);
            glVertex2f(ballX,ballY);
            glVertex2f(aimX,aimY);
            glEnd();
        }

        drawText(10,580,"Level: " + std::to_string(level) +
                           " Time: " + std::to_string(timeLeft));
    }

    glutSwapBuffers();
}

// -------- INIT --------
void init(){
    glClearColor(0.3,0.7,0.3,1);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

// -------- MAIN --------
int main(int argc,char** argv){

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIDTH,HEIGHT);
    glutCreateWindow("Mini Golf");

    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(16,update,0);

    glutMainLoop();
}