#include <GL/glut.h>
#include <cmath>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <vector>

float bus1 = -200, bus2 = 1000;
float cloudMove = 0;
float birdMove = 0;

float studentEntry1 = -100;
float studentEntry2 = -300;

float studentExit1 = 750;
float studentExit2 = 900;

float studentEntrySpeed1 = 0.0f;
float studentEntrySpeed2 = 0.0f;
float studentExitSpeed1 = 0.0f;
float studentExitSpeed2 = 0.0f;

float studentPhaseEntry1 = 0.0f;
float studentPhaseEntry2 = 1.6f;
float studentPhaseExit1 = 3.1f;
float studentPhaseExit2 = 4.5f;
float walkTime = 0.0f;
float rainOffset = 0.0f;
float rainTranslateY = 0.0f;
float thunderFlash = 0.0f;
int thunderCooldown = 120;

struct RainPoint {
    float x;
    float y;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

std::vector<RainPoint> rainPoints;

enum SceneMode {
    MODE_DAY,
    MODE_NIGHT,
    MODE_RAIN
};

SceneMode currentMode = MODE_DAY;

struct BusDropStudent {
    float x;
    float y;
    float phase;
    float scale;
    float walkSpeed;
    bool backpack;
    int state; // 0 hidden, 1 dropping, 2 walking, 3 reached gate
};

BusDropStudent busDropStudents[4];
bool busPaused = false;
bool busDropActive = false;
bool busAutoResumePending = false;
int busResumeDelayFrames = 0;

void circle(float x, float y, float r) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 100; i++) {
        float a = i * 2 * 3.1416 / 100;
        glVertex2f(x + cos(a) * r, y + sin(a) * r);
    }
    glEnd();
}

void ellipse(float x, float y, float rx, float ry) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 100; i++) {
        float a = i * 2 * 3.1416f / 100;
        glVertex2f(x + cos(a) * rx, y + sin(a) * ry);
    }
    glEnd();
}

void drawText(float x, float y, const char* text, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (int i = 0; i < (int)strlen(text); i++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, text[i]);
}

void drawCloud(float x, float y) {
    glColor3f(0.85, 0.88, 0.9);
    circle(x, y, 25);
    circle(x + 20, y + 5, 30);
    circle(x + 45, y, 25);
    glColor3f(1.0, 1.0, 1.0);
    circle(x + 2, y + 3, 22);
    circle(x + 20, y + 10, 28);
    circle(x + 43, y + 5, 23);
}

void drawBirdV(float x, float y, float size) {
    // Swept filled wings matching the reference style
    glColor3f(0.98f, 0.98f, 0.95f);
    glBegin(GL_POLYGON);
    glVertex2f(x + size * 0.00f, y + size * 0.10f);
    glVertex2f(x - size * 0.28f, y + size * 0.20f);
    glVertex2f(x - size * 1.10f, y + size * 0.52f);
    glVertex2f(x - size * 0.72f, y + size * 0.30f);
    glVertex2f(x - size * 0.34f, y + size * 0.08f);
    glVertex2f(x - size * 0.10f, y + size * 0.06f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex2f(x + size * 0.00f, y + size * 0.10f);
    glVertex2f(x + size * 0.28f, y + size * 0.20f);
    glVertex2f(x + size * 1.10f, y + size * 0.52f);
    glVertex2f(x + size * 0.72f, y + size * 0.30f);
    glVertex2f(x + size * 0.34f, y + size * 0.08f);
    glVertex2f(x + size * 0.10f, y + size * 0.06f);
    glEnd();

    // Subtle inner feather tone
    glColor3f(0.90f, 0.90f, 0.88f);
    glBegin(GL_TRIANGLES);
    glVertex2f(x - size * 0.08f, y + size * 0.11f);
    glVertex2f(x - size * 0.46f, y + size * 0.20f);
    glVertex2f(x - size * 0.24f, y + size * 0.10f);

    glVertex2f(x + size * 0.08f, y + size * 0.11f);
    glVertex2f(x + size * 0.46f, y + size * 0.20f);
    glVertex2f(x + size * 0.24f, y + size * 0.10f);

    // tiny center body
    glColor3f(0.95f, 0.95f, 0.92f);
    glVertex2f(x - size * 0.09f, y + size * 0.08f);
    glVertex2f(x + size * 0.09f, y + size * 0.08f);
    glVertex2f(x + size * 0.00f, y + size * 0.18f);
    glEnd();
}

void drawBirdFlock(float offsetX) {
    glLineWidth(2.0f);

    float x = -120.0f + offsetX;
    drawBirdV(x + 0, 520, 10);
    drawBirdV(x + 30, 535, 8);
    drawBirdV(x + 60, 518, 9);
    drawBirdV(x + 92, 532, 7);
    drawBirdV(x + 125, 522, 8);

    glLineWidth(1.0f);
}

void drawDIUBus(float x, float y, int direction) {
    // Bus Main Body
    glColor3f(0.0, 0.4, 0.2); // Darker Green
    glBegin(GL_QUADS);
    glVertex2f(x, y + 15); glVertex2f(x + 160, y + 15);
    glVertex2f(x + 160, y + 75); glVertex2f(x, y + 75);
    glEnd();

    // Windows with reflection effect
    for (int i = 0; i < 4; i++) {
        glColor3f(0.2, 0.3, 0.4); // Glass Base
        glBegin(GL_QUADS);
        glVertex2f(x + 10 + (i * 38), y + 42); glVertex2f(x + 45 + (i * 38), y + 42);
        glVertex2f(x + 45 + (i * 38), y + 68); glVertex2f(x + 10 + (i * 38), y + 68);
        glEnd();
        // Reflection Line
        glColor3f(0.5, 0.6, 0.7);
        glBegin(GL_LINES);
        glVertex2f(x + 15 + (i * 38), y + 45); glVertex2f(x + 40 + (i * 38), y + 65);
        glEnd();
    }

    // Front Windshield (Based on direction)
    glColor3f(0.1, 0.2, 0.3);
    glBegin(GL_QUADS);
    if (direction == 1) {
        glVertex2f(x + 150, y + 42); glVertex2f(x + 160, y + 42);
        glVertex2f(x + 160, y + 70); glVertex2f(x + 150, y + 70);
    }
    else {
        glVertex2f(x, y + 42); glVertex2f(x + 10, y + 42);
        glVertex2f(x + 10, y + 70); glVertex2f(x, y + 70);
    }
    glEnd();

    // Wheels with Rims
    glColor3f(0.1, 0.1, 0.1); // Tire
    circle(x + 35, y + 15, 15); circle(x + 125, y + 15, 15);
    glColor3f(0.7, 0.7, 0.7); // Rim
    circle(x + 35, y + 15, 7); circle(x + 125, y + 15, 7);

    // Lights
    if (direction == 1) {
        glColor3f(1.0, 1.0, 0.0); circle(x + 160, y + 25, 4); // Headlight
        glColor3f(1.0, 0.0, 0.0); circle(x, y + 25, 4);      // Backlight
    }
    else {
        glColor3f(1.0, 1.0, 0.0); circle(x, y + 25, 4);      // Headlight
        glColor3f(1.0, 0.0, 0.0); circle(x + 160, y + 25, 4); // Backlight
    }

    drawText(x + 55, y + 25, "DIU BUS", 1, 1, 1);
}

void drawRoad() {
    // Road Main Surface (Dark Charcoal)
    glColor3f(0.12, 0.12, 0.12);
    glBegin(GL_QUADS);
    glVertex2f(0, 80); glVertex2f(1000, 80);
    glVertex2f(1000, 250); glVertex2f(0, 250);
    glEnd();

    // Road Shoulders / Sidewalk edge
    glColor3f(0.4, 0.4, 0.4);
    glBegin(GL_QUADS);
    glVertex2f(0, 80); glVertex2f(1000, 80); glVertex2f(1000, 90); glVertex2f(0, 90);
    glVertex2f(0, 240); glVertex2f(1000, 240); glVertex2f(1000, 250); glVertex2f(0, 250);
    glEnd();

    // Lane Markers (White Strips)
    glColor3f(1, 1, 1);
    for (int i = 0; i < 1000; i += 80) {
        glBegin(GL_QUADS);
        glVertex2f(i + 30, 163); glVertex2f(i + 70, 163);
        glVertex2f(i + 70, 167); glVertex2f(i + 30, 167);
        glEnd();
    }

    // Premium walking path with curb and tile bands.
    glColor3f(0.66f, 0.68f, 0.70f); // curb wall
    glRectf(0, 250, 1000, 258);
    glColor3f(0.86f, 0.88f, 0.90f); // curb top
    glRectf(0, 258, 1000, 261);

    glColor3f(0.80f, 0.74f, 0.66f); // path base
    glRectf(0, 261, 1000, 287);

    glColor3f(0.88f, 0.83f, 0.75f); // warm tile stripe
    glRectf(0, 268, 1000, 274);

    glColor3f(0.74f, 0.67f, 0.58f); // inner accent stripe
    glRectf(0, 275, 1000, 279);

    glColor3f(0.58f, 0.53f, 0.47f); // top boundary
    glRectf(0, 286, 1000, 287);

    glColor3f(0.68f, 0.62f, 0.55f); // repeating paver joints
    for (int i = 0; i < 1000; i += 42) {
        glRectf(i + 3, 261, i + 6, 287);
    }

    glColor3f(0.92f, 0.89f, 0.83f); // center paver blocks
    for (int i = 0; i < 1000; i += 70) {
        glRectf(i + 9, 269, i + 34, 273);
        glRectf(i + 39, 269, i + 64, 273);
    }
}

void drawDIUGate() {
    // Side pillars
    glColor3f(0.80f, 0.82f, 0.86f);
    glRectf(280, 260, 340, 380);
    glRectf(660, 260, 720, 380);
    glColor3f(0.68f, 0.72f, 0.78f);
    glRectf(280, 260, 292, 380);
    glRectf(660, 260, 672, 380);
    glColor3f(0.92f, 0.94f, 0.97f);
    glRectf(332, 260, 340, 380);
    glRectf(712, 260, 720, 380);

    // Pillar decorative bands
    glColor3f(0.04f, 0.45f, 0.82f);
    glRectf(280, 320, 340, 334);
    glRectf(660, 320, 720, 334);
    glColor3f(0.58f, 0.84f, 0.98f);
    glRectf(280, 334, 340, 339);
    glRectf(660, 334, 720, 339);

    // Central block and frame
    glColor3f(0.88f, 0.92f, 0.97f);
    glRectf(396, 258, 604, 424);
    glColor3f(0.77f, 0.84f, 0.91f);
    glRectf(406, 268, 594, 414);

    // Glass windows
    glColor3f(0.45f, 0.76f, 0.96f);
    for (int i = 0; i < 4; i++) {
        glRectf(416 + i * 44, 276, 448 + i * 44, 402);
    }
    glColor3f(0.82f, 0.93f, 1.0f);
    for (int i = 0; i < 4; i++) {
        glRectf(420 + i * 44, 362, 444 + i * 44, 398);
    }

    // Curved roof wings
    glColor3f(0.02f, 0.42f, 0.24f);
    glBegin(GL_POLYGON);
    for (int j = 0; j <= 50; j++) {
        float ang = j * 3.1416f / 50;
        glVertex2f(340 + cos(ang) * 112, 380 + sin(ang) * 46);
    }
    glEnd();
    glBegin(GL_POLYGON);
    for (int j = 0; j <= 50; j++) {
        float ang = j * 3.1416f / 50;
        glVertex2f(660 + cos(ang) * 112, 380 + sin(ang) * 46);
    }
    glEnd();

    // Roof highlights
    glColor3f(0.19f, 0.62f, 0.38f);
    glBegin(GL_POLYGON);
    for (int j = 0; j <= 50; j++) {
        float ang = j * 3.1416f / 50;
        glVertex2f(340 + cos(ang) * 96, 383 + sin(ang) * 30);
    }
    glEnd();
    glBegin(GL_POLYGON);
    for (int j = 0; j <= 50; j++) {
        float ang = j * 3.1416f / 50;
        glVertex2f(660 + cos(ang) * 96, 383 + sin(ang) * 30);
    }
    glEnd();

    // Entry / Exit boards
    glColor3f(0.03f, 0.36f, 0.78f);
    glRectf(276, 385, 344, 406);
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(293, 393, "ENTRY", 1, 1, 1);

    glColor3f(0.20f, 0.24f, 0.30f);
    glRectf(656, 385, 724, 406);
    drawText(676, 393, "EXIT", 1, 1, 1);

    // Center DIU badge
    glColor3f(0.98f, 0.78f, 0.08f);
    glRectf(452, 416, 548, 468);
    glColor3f(0.01f, 0.40f, 0.22f);
    glRectf(458, 422, 542, 462);
    drawText(486, 438, "DIU", 1, 1, 1);

    // Round shrubs at both sides
    glColor3f(0.0, 0.6, 0.1);
    // বাম সাইডে ঝোপ
    for (int i = 0; i < 5; i++) {
        circle(50 + i * 40, 270, 15);
    }
    // ডান সাইডে ঝোপ
    for (int i = 0; i < 5; i++) {
        circle(780 + i * 40, 270, 15);
    }
}

// ১. বড় গাছের ডিজাইন (ঘন ও রিয়েলিস্টিক লুক)
void drawTree(float x, float y)
{
    // Trunk (slim, straight)
    glColor3f(0.45f, 0.25f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(x + 4, y);
    glVertex2f(x + 18, y);
    glVertex2f(x + 18, y + 105);
    glVertex2f(x + 4, y + 105);
    glEnd();

    // Branches
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex2f(x + 11, y + 42); glVertex2f(x - 8, y + 70);
    glVertex2f(x + 11, y + 56); glVertex2f(x + 31, y + 82);
    glVertex2f(x + 11, y + 68); glVertex2f(x - 2, y + 93);
    glVertex2f(x + 11, y + 74); glVertex2f(x + 24, y + 98);
    glEnd();
    glLineWidth(1.0f);

    // Dark foliage back layer
    glColor3f(0.02f, 0.45f, 0.18f);
    circle(x - 10, y + 102, 24);
    circle(x + 20, y + 116, 28);
    circle(x + 44, y + 108, 23);
    circle(x - 2, y + 130, 25);
    circle(x + 30, y + 134, 27);
    circle(x + 58, y + 128, 22);

    // Mid layer
    glColor3f(0.05f, 0.53f, 0.21f);
    circle(x - 18, y + 82, 16);
    circle(x + 52, y + 88, 18);
    circle(x + 8, y + 144, 22);
    circle(x + 42, y + 146, 20);

    // Front highlight layer
    glColor3f(0.16f, 0.66f, 0.24f);
    circle(x - 1, y + 112, 20);
    circle(x + 26, y + 112, 21);
    circle(x + 14, y + 92, 16);
    circle(x + 42, y + 90, 15);
    circle(x - 15, y + 70, 13);
    circle(x + 38, y + 68, 13);
}

// ২. ছোট গোল গাছ/ঝোপঝাড়ের ডিজাইন (আরও সুন্দর)
void drawSmallTree(float x, float y) {
    glColor3f(0.0, 0.4, 0.1); // গভীর সবুজ
    circle(x, y, 18);
    glColor3f(0.0, 0.6, 0.2); // হালকা সবুজ শেড (হাইলাইট)
    circle(x, y + 5, 12);
}
void drawDaySun() {
    glColor3f(1.0f, 0.92f, 0.25f);
    circle(860, 525, 34);
    glColor3f(1.0f, 0.98f, 0.55f);
    circle(860, 525, 23);
}

void drawNightMoonAndStars() {
    glColor3f(0.94f, 0.96f, 1.0f);
    circle(850, 520, 28);
    glColor3f(0.05f, 0.08f, 0.17f);
    circle(862, 526, 22);

    glColor3f(0.95f, 0.96f, 0.85f);
    for (int i = 0; i < 24; i++) {
        float x = 30.0f + fmod(i * 89.0f, 940.0f);
        float y = 430.0f + fmod(i * 53.0f, 150.0f);
        circle(x, y, 1.4f + (i % 3) * 0.4f);
    }
}

void drawRain() {
    if (rainPoints.empty()) return;

    // Borrowed from the provided village rain logic:
    // a fixed random point cloud translated down over time.
    glPushMatrix();
    glTranslatef(0.0f, -rainTranslateY, 0.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(RainPoint), &rainPoints[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(RainPoint), &rainPoints[0].r);
    glPointSize(2.5f + thunderFlash * 0.6f);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(rainPoints.size()));
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glPopMatrix();
}

void drawPuddleReflections() {
    float shimmer = sin(rainOffset * 0.075f) * 1.6f;
    float pulse = 0.10f + thunderFlash * 0.35f;

    // Puddle bases.
    glColor4f(0.10f + pulse, 0.14f + pulse, 0.18f + pulse, 0.62f);
    ellipse(160, 271, 68, 11);
    ellipse(355, 270, 78, 12);
    ellipse(548, 271, 72, 11);
    ellipse(760, 270, 80, 12);

    // Soft reflected streaks from gate/light.
    glLineWidth(1.4f);
    glColor4f(0.36f + pulse, 0.48f + pulse, 0.60f + pulse, 0.58f);
    glBegin(GL_LINES);
    for (int i = 0; i < 26; i++) {
        float px = 95.0f + i * 34.0f + shimmer;
        float yTop = 278.0f + (i % 3) * 0.9f;
        float yBot = 265.0f - (i % 2) * 0.6f;
        glVertex2f(px, yTop);
        glVertex2f(px + 4.0f, yBot);
    }
    glEnd();
    glLineWidth(1.0f);
}

void drawThunderFlashOverlay() {
    if (thunderFlash <= 0.01f) return;

    glColor4f(0.86f, 0.90f, 1.0f, 0.17f * thunderFlash);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(1000, 0);
    glVertex2f(1000, 600);
    glVertex2f(0, 600);
    glEnd();

    glLineWidth(2.0f + thunderFlash * 2.6f);
    glColor4f(0.96f, 0.98f, 1.0f, 0.74f * thunderFlash);
    glBegin(GL_LINE_STRIP);
    glVertex2f(700, 590);
    glVertex2f(676, 540);
    glVertex2f(704, 500);
    glVertex2f(675, 458);
    glVertex2f(694, 420);
    glVertex2f(670, 386);
    glEnd();
    glLineWidth(1.0f);
}

void drawModeHint() {
    if (currentMode == MODE_DAY) {
        drawText(18, 575, "Mode: DAY (d/n/r, e=stop+drop auto run, s=manual start)", 0.08f, 0.18f, 0.45f);
    }
    else if (currentMode == MODE_NIGHT) {
        drawText(18, 575, "Mode: NIGHT (press d/n/r)", 0.88f, 0.90f, 0.98f);
    }
    else {
        drawText(18, 575, "Mode: RAINING (d/n/r, e=stop+drop auto run, s=manual start)", 0.92f, 0.96f, 1.0f);
    }
}

void keyboard(unsigned char key, int x, int y) {
    (void)x;
    (void)y;

    unsigned char lower = static_cast<unsigned char>(std::tolower(key));
    if (lower == 'd') currentMode = MODE_DAY;
    else if (lower == 'n') currentMode = MODE_NIGHT;
    else if (lower == 'r') currentMode = MODE_RAIN;
    else if (lower == 'e') {
        if (currentMode != MODE_NIGHT) {
            busPaused = true;
            busAutoResumePending = true;
            busResumeDelayFrames = 20;
            if (!busDropActive) {
                if (bus1 < 140.0f || bus1 > 360.0f) bus1 = 220.0f;
                for (int i = 0; i < 4; i++) {
                    busDropStudents[i].x = bus1 + 120.0f - i * 10.0f;
                    busDropStudents[i].y = 171.0f + (i % 2) * 4.0f;
                    busDropStudents[i].phase = i * 0.7f;
                    busDropStudents[i].scale = 0.90f + (i % 3) * 0.05f;
                    busDropStudents[i].walkSpeed = 1.0f + i * 0.14f;
                    busDropStudents[i].backpack = (i % 2 == 0);
                    busDropStudents[i].state = 1;
                }
                busDropActive = true;
            }
        }
    }
    else if (lower == 's') {
        busPaused = false;
        busAutoResumePending = false;
        busResumeDelayFrames = 0;
    }

    glutPostRedisplay();
}

void drawStudent(float x, float y, float phase, float scale, bool backpack)
{
    float stride = sin(phase);
    float swing = stride * 5.4f * scale;
    float bob = (0.35f + fabs(stride) * 0.95f) * scale;
    float torsoSway = sin(phase * 0.5f) * 0.85f * scale;
    float torsoLean = stride * 0.75f * scale;
    x += torsoSway;
    float groundY = y;
    float baseY = groundY + bob;
    float hipY = baseY + 12.0f * scale;
    float shoulderY = baseY + 31.0f * scale;
    float headY = baseY + 41.5f * scale;

    if (backpack) {
        glColor3f(0.25f, 0.18f, 0.1f);
        glBegin(GL_QUADS);
        glVertex2f(x - 9.0f * scale, shoulderY - 2.0f * scale);
        glVertex2f(x - 3.5f * scale, shoulderY - 2.0f * scale);
        glVertex2f(x - 3.5f * scale, hipY + 7.0f * scale);
        glVertex2f(x - 9.0f * scale, hipY + 7.0f * scale);
        glEnd();
    }

    // head
    glColor3f(1.0, 0.8, 0.6);
    circle(x + torsoLean * 0.25f, headY, 6.2f * scale);
    glColor3f(0.2f, 0.13f, 0.07f);
    circle(x + torsoLean * 0.25f, headY + 2.5f * scale, 4.8f * scale);

    // neck + torso
    glColor3f(0.95f, 0.77f, 0.58f);
    glBegin(GL_LINES);
    glVertex2f(x + torsoLean * 0.20f, headY - 6.0f * scale);
    glVertex2f(x + torsoLean * 0.12f, shoulderY + 1.5f * scale);
    glEnd();

    glColor3f(0.1f, 0.22f, 0.78f);
    glBegin(GL_QUADS);
    glVertex2f(x - 6.0f * scale + torsoLean * 0.22f, shoulderY);
    glVertex2f(x + 6.0f * scale + torsoLean * 0.22f, shoulderY);
    glVertex2f(x + 4.5f * scale - torsoLean * 0.18f, hipY);
    glVertex2f(x - 4.5f * scale - torsoLean * 0.18f, hipY);
    glEnd();

    glColor3f(0.15f, 0.15f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(x - 4.5f * scale - torsoLean * 0.18f, hipY);
    glVertex2f(x + 4.5f * scale - torsoLean * 0.18f, hipY);
    glVertex2f(x + 5.0f * scale - torsoLean * 0.18f, hipY - 3.8f * scale);
    glVertex2f(x - 5.0f * scale - torsoLean * 0.18f, hipY - 3.8f * scale);
    glEnd();

    float leftElbowX = x - 7.0f * scale - swing * 0.25f;
    float rightElbowX = x + 7.0f * scale + swing * 0.25f;
    float leftHandX = x - 10.5f * scale - swing * 0.55f;
    float rightHandX = x + 10.5f * scale + swing * 0.55f;
    float elbowY = shoulderY - 6.5f * scale;
    float handY = shoulderY - 12.0f * scale;

    glLineWidth(2.2f);
    glColor3f(0.1f, 0.22f, 0.78f);
    glBegin(GL_LINES);
    glVertex2f(x - 5.0f * scale, shoulderY - 1.5f * scale);
    glVertex2f(leftElbowX, elbowY);
    glVertex2f(leftElbowX, elbowY);
    glVertex2f(leftHandX, handY);

    glVertex2f(x + 5.0f * scale, shoulderY - 1.5f * scale);
    glVertex2f(rightElbowX, elbowY);
    glVertex2f(rightElbowX, elbowY);
    glVertex2f(rightHandX, handY);
    glEnd();

    float leftKneeX = x - 3.0f * scale + swing * 0.34f;
    float rightKneeX = x + 3.0f * scale - swing * 0.34f;
    float kneeY = baseY + 6.0f * scale;
    float leftFootX = x - 5.5f * scale + swing * 0.75f;
    float rightFootX = x + 5.5f * scale - swing * 0.75f;
    float leftLift = (stride > 0.0f ? stride : 0.0f) * 1.2f * scale;
    float rightLift = (stride < 0.0f ? -stride : 0.0f) * 1.2f * scale;
    float leftFootY = groundY + 1.0f * scale + leftLift;
    float rightFootY = groundY + 1.0f * scale + rightLift;

    glColor3f(0.05f, 0.05f, 0.08f);
    glBegin(GL_LINES);
    glVertex2f(x - 2.0f * scale, hipY - 3.8f * scale);
    glVertex2f(leftKneeX, kneeY);
    glVertex2f(leftKneeX, kneeY);
    glVertex2f(leftFootX, leftFootY);

    glVertex2f(x + 2.0f * scale, hipY - 3.8f * scale);
    glVertex2f(rightKneeX, kneeY);
    glVertex2f(rightKneeX, kneeY);
    glVertex2f(rightFootX, rightFootY);
    glEnd();

    glColor3f(0.2f, 0.2f, 0.24f);
    glRectf(leftFootX - 2.0f * scale, leftFootY - 1.0f * scale, leftFootX + 3.0f * scale, leftFootY + 0.3f * scale);
    glRectf(rightFootX - 2.0f * scale, rightFootY - 1.0f * scale, rightFootX + 3.0f * scale, rightFootY + 0.3f * scale);
    glLineWidth(1.0f);
}

void drawUmbrella(float x, float y, float scale, float phase) {
    float sway = sin(phase * 0.45f) * 0.8f * scale;
    float centerX = x + 10.0f * scale + sway;
    float handleTopY = y + 52.0f * scale;
    float canopyY = y + 58.0f * scale;

    // Handle
    glLineWidth(2.2f);
    glColor3f(0.22f, 0.12f, 0.05f);
    glBegin(GL_LINES);
    glVertex2f(centerX, y + 36.0f * scale);
    glVertex2f(centerX, handleTopY);
    glEnd();

    // Hook
    glBegin(GL_LINE_STRIP);
    glVertex2f(centerX, y + 36.0f * scale);
    glVertex2f(centerX + 3.0f * scale, y + 34.5f * scale);
    glVertex2f(centerX + 4.0f * scale, y + 31.8f * scale);
    glEnd();

    // Canopy
    glColor3f(0.86f, 0.14f, 0.18f);
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 28; i++) {
        float a = 3.1415926f * i / 28.0f;
        glVertex2f(centerX + cos(a) * 13.0f * scale, canopyY + sin(a) * 8.0f * scale);
    }
    glEnd();

    // Canopy edge scallops
    glColor3f(0.74f, 0.09f, 0.14f);
    glBegin(GL_TRIANGLES);
    glVertex2f(centerX - 11.0f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX - 8.0f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX - 9.5f * scale, canopyY - 2.0f * scale);

    glVertex2f(centerX - 4.0f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX - 1.0f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX - 2.5f * scale, canopyY - 2.1f * scale);

    glVertex2f(centerX + 3.0f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX + 6.0f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX + 4.5f * scale, canopyY - 2.0f * scale);

    glVertex2f(centerX + 10.0f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX + 12.6f * scale, canopyY + 1.0f * scale);
    glVertex2f(centerX + 11.3f * scale, canopyY - 1.8f * scale);
    glEnd();

    glLineWidth(1.0f);
}

void drawStudentCharacter(float x, float y, float phase, float scale, bool backpack, bool withUmbrella) {
    drawStudent(x, y, phase, scale, backpack);
    if (withUmbrella) drawUmbrella(x, y, scale, phase);
}

void display() {
    if (currentMode == MODE_DAY) glClearColor(0.5f, 0.8f, 1.0f, 1.0f);
    else if (currentMode == MODE_NIGHT) glClearColor(0.05f, 0.08f, 0.17f, 1.0f);
    else glClearColor(0.38f, 0.44f, 0.52f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (currentMode == MODE_DAY) drawDaySun();
    if (currentMode == MODE_NIGHT) drawNightMoonAndStars();

    drawCloud(100 + cloudMove, 520);
    drawCloud(500 + cloudMove, 540);
    drawCloud(850 + cloudMove, 510);
    if (currentMode == MODE_DAY) {
        drawBirdFlock(birdMove);
        drawBirdFlock(birdMove - 520);
    }

    glColor3f(0.2f, 0.7f, 0.2f);
    glRectf(0, 80, 1000, 260);

    drawRoad();
    if (currentMode == MODE_RAIN) drawPuddleReflections();
    drawDIUGate();
    drawTree(80, 260);
    drawTree(920, 260);

    for (int i = 0; i < 3; i++) drawSmallTree(180 + i * 40, 265);
    for (int i = 0; i < 3; i++) drawSmallTree(760 + i * 40, 265);

    drawDIUBus(bus1, 95, 1);
    drawDIUBus(bus2, 170, -1);

    if (currentMode != MODE_NIGHT) {
        bool rainUmbrella = (currentMode == MODE_RAIN);

        drawStudentCharacter(studentEntry1, 261, studentPhaseEntry1, 1.0f, true, rainUmbrella);
        drawStudentCharacter(studentEntry2, 261, studentPhaseEntry2, 0.96f, false, rainUmbrella);
        drawStudentCharacter(studentEntry1 - 26, 261, studentPhaseEntry1 + 0.7f, 0.94f, false, rainUmbrella);
        drawStudentCharacter(studentEntry1 - 52, 261, studentPhaseEntry1 + 1.2f, 1.02f, true, rainUmbrella);
        drawStudentCharacter(studentEntry2 - 24, 261, studentPhaseEntry2 + 0.9f, 0.92f, false, rainUmbrella);

        drawStudentCharacter(studentExit1, 261, studentPhaseExit1, 1.0f, true, rainUmbrella);
        drawStudentCharacter(studentExit2, 261, studentPhaseExit2, 1.04f, false, rainUmbrella);
        drawStudentCharacter(studentExit1 - 28, 261, studentPhaseExit1 + 0.8f, 0.93f, false, rainUmbrella);
        drawStudentCharacter(studentExit1 - 54, 261, studentPhaseExit1 + 1.4f, 0.98f, true, rainUmbrella);
        drawStudentCharacter(studentExit2 - 24, 261, studentPhaseExit2 + 1.1f, 0.95f, false, rainUmbrella);

        if (busDropActive) {
            for (int i = 0; i < 4; i++) {
                if (busDropStudents[i].state == 1 || busDropStudents[i].state == 2) {
                    drawStudentCharacter(
                        busDropStudents[i].x,
                        busDropStudents[i].y,
                        busDropStudents[i].phase,
                        busDropStudents[i].scale,
                        busDropStudents[i].backpack,
                        rainUmbrella
                    );
                }
            }
        }
    }

    if (currentMode == MODE_RAIN) drawRain();
    if (currentMode == MODE_RAIN) drawThunderFlashOverlay();
    drawModeHint();
    glutSwapBuffers();
}
void update(int v) {

    if (!busPaused) {
        bus1 += 2.2;
        bus2 -= 1.8;
    }
    cloudMove += 0.4;
    birdMove += 0.9f;
    if (currentMode == MODE_RAIN) {
        rainTranslateY += 7.5f;
        if (rainTranslateY > 700.0f) rainTranslateY = 0.0f;
        rainOffset += 0.25f;

        if (thunderCooldown > 0) thunderCooldown--;
        else if (thunderFlash <= 0.01f && (rand() % 1000) < 14) {
            thunderFlash = 1.0f;
            thunderCooldown = 90 + rand() % 170;
        }
    }

    thunderFlash *= 0.86f;
    if (thunderFlash < 0.01f) thunderFlash = 0.0f;

    if (busDropActive && currentMode != MODE_NIGHT) {
        bool anyActive = false;
        for (int i = 0; i < 4; i++) {
            if (busDropStudents[i].state == 1) {
                busDropStudents[i].y += 2.6f;
                busDropStudents[i].phase += 0.08f;
                if (busDropStudents[i].y >= 261.0f) {
                    busDropStudents[i].y = 261.0f;
                    busDropStudents[i].state = 2;
                }
                anyActive = true;
            }
            else if (busDropStudents[i].state == 2) {
                busDropStudents[i].x += busDropStudents[i].walkSpeed;
                busDropStudents[i].phase += 0.14f + busDropStudents[i].walkSpeed * 0.08f;
                if (busDropStudents[i].x >= 410.0f + (i % 2) * 10.0f) {
                    busDropStudents[i].state = 3;
                }
                else {
                    anyActive = true;
                }
            }
        }
        busDropActive = anyActive;
    }

    if (busAutoResumePending && !busDropActive) {
        if (busResumeDelayFrames > 0) busResumeDelayFrames--;
        else {
            busPaused = false;
            busAutoResumePending = false;
        }
    }

    walkTime += 0.03f;
    float entryTarget1 = 0.96f + 0.10f * sin(walkTime * 0.95f);
    float entryTarget2 = 1.12f + 0.14f * sin(walkTime * 0.78f + 1.2f);
    float exitTarget1 = 1.30f + 0.12f * sin(walkTime * 0.88f + 0.8f);
    float exitTarget2 = 1.06f + 0.10f * sin(walkTime * 1.06f + 2.0f);

    // Slightly slow down near the gate for a more natural flow.
    if (studentEntry1 > 250.0f && studentEntry1 < 365.0f) entryTarget1 *= 0.86f;
    if (studentEntry2 > 250.0f && studentEntry2 < 365.0f) entryTarget2 *= 0.88f;

    // Speed eases toward changing targets to avoid robotic constant pace.
    studentEntrySpeed1 += (entryTarget1 - studentEntrySpeed1) * 0.06f;
    studentEntrySpeed2 += (entryTarget2 - studentEntrySpeed2) * 0.06f;
    studentExitSpeed1 += (exitTarget1 - studentExitSpeed1) * 0.06f;
    studentExitSpeed2 += (exitTarget2 - studentExitSpeed2) * 0.06f;

    studentEntry1 += studentEntrySpeed1;
    studentEntry2 += studentEntrySpeed2;

    if (studentEntry1 > 400) {
        studentEntry1 = -100;
        studentEntrySpeed1 = 0.0f;
        studentPhaseEntry1 = 0.0f;
    }
    if (studentEntry2 > 400) {
        studentEntry2 = -300;
        studentEntrySpeed2 = 0.0f;
        studentPhaseEntry2 = 1.6f;
    }

    studentExit1 += studentExitSpeed1;
    studentExit2 += studentExitSpeed2;

    if (studentExit1 > 1100) {
        studentExit1 = 740;
        studentExitSpeed1 = 0.0f;
        studentPhaseExit1 = 3.1f;
    }
    if (studentExit2 > 1100) {
        studentExit2 = 820;
        studentExitSpeed2 = 0.0f;
        studentPhaseExit2 = 4.5f;
    }

    studentPhaseEntry1 += 0.10f + studentEntrySpeed1 * 0.11f;
    studentPhaseEntry2 += 0.10f + studentEntrySpeed2 * 0.11f;
    studentPhaseExit1 += 0.10f + studentExitSpeed1 * 0.11f;
    studentPhaseExit2 += 0.10f + studentExitSpeed2 * 0.11f;

    if (bus1 > 1000) bus1 = -200;
    if (bus2 < -200) bus2 = 1000;
    if (cloudMove > 1100) cloudMove = -400;
    if (birdMove > 1400) birdMove = -120;

    glutPostRedisplay();
    glutTimerFunc(20, update, 0);
}

void init() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1000, 0, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    rainPoints.clear();
    rainPoints.reserve(1200);
    for (int i = 0; i < 1200; i++) {
        RainPoint pt;
        pt.x = static_cast<float>(rand() % 1000);
        pt.y = static_cast<float>(rand() % 700);
        unsigned char base = static_cast<unsigned char>(215 + rand() % 40);
        pt.r = base;
        pt.g = base;
        pt.b = 255;
        pt.a = 255;
        rainPoints.push_back(pt);
    }

    for (int i = 0; i < 4; i++) {
        busDropStudents[i].state = 0;
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1000, 600);
    glutCreateWindow("Realistic DIU Road Scene");
    srand((unsigned int)time(0));
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(20, update, 0);
    glutMainLoop();
    return 0;
}

