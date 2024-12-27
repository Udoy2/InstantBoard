#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <sstream>
#include <vector>
#include <cstring>
// Structure to represent a line segment
struct Line {
    int x1, y1, x2, y2;
    float color[3];
    int size;
    bool isEraser;
};

// Structure to represent a stroke (collection of connected line segments)
struct Stroke {
    std::vector<Line> lines;
    float color[3];
    int size;
    bool isEraser;
};

// Variables for mouse state
int prevX = -1, prevY = -1;
int windowWidth = 800, windowHeight = 600;
int tool = 1; // 1 for pencil, 2 for eraser
float currentColor[3] = {0.0, 0.0, 0.0}; // Initial color black
int pointSize = 2;

// Vector to store all strokes
std::vector<Stroke> strokes;
Stroke currentStroke;

// Button structure
typedef struct {
    int x, y, w, h;
    const char *label;
    void (*callbackFunction)();
} Button;

// Button callback functions
void setPencilTool() { tool = 1; }
void setEraserTool() { tool = 2; }
void clearScreen() {
    strokes.clear();
    glutPostRedisplay();
}
void increasePointSize() { if (pointSize < 50) pointSize++; glutPostRedisplay(); }
void decreasePointSize() { if (pointSize > 1) pointSize--; glutPostRedisplay(); }

// Define buttons
Button buttons[] = {
    {10, 40, 100, 25, "PENCIL", setPencilTool},
    {10, 80, 100, 25, "ERASER", setEraserTool},
    {10, 120, 100, 25, "CLEAR", clearScreen},
    {30, 180, 30, 25, "+", increasePointSize},
    {70, 180, 30, 25, "-", decreasePointSize}
};
int numButtons = 5;

// Function to draw text
void drawText(int x, int y, const char *text) {
    glRasterPos2i(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *text++);
    }
}

// Function to draw buttons
void drawButton(Button *b) {
    // Draw button background with #FFD7C4
    glColor3f(1.0, 0.84, 0.77);
    glBegin(GL_QUADS);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    // Draw button border
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    // Draw button label
    glColor3f(0.0, 0.0, 0.0);
    drawText(b->x + 10, b->y + 15, b->label);
}

// Function to draw color picker
void drawColorPicker() {
    glBegin(GL_QUADS);
    glColor3f(1.0, 0.0, 0.0); glVertex2i(10, 500);
    glColor3f(0.0, 1.0, 0.0); glVertex2i(110, 500);
    glColor3f(0.0, 0.0, 1.0); glVertex2i(110, 400);
    glColor3f(1.0, 1.0, 0.0); glVertex2i(10, 400);
    glEnd();
}

// Function to draw all strokes
void drawStrokes() {
    // Replace range-based for loops with traditional for loops
    for (size_t i = 0; i < strokes.size(); i++) {
        const Stroke& stroke = strokes[i];
        for (size_t j = 0; j < stroke.lines.size(); j++) {
            const Line& line = stroke.lines[j];
            if (line.isEraser) {
                glColor3f(1.0, 1.0, 1.0);
            } else {
                glColor3fv(line.color);
            }
            glLineWidth(line.size);
            glBegin(GL_LINES);
            glVertex2i(line.x1, line.y1);
            glVertex2i(line.x2, line.y2);
            glEnd();
        }
    }
}

// Function to check button click
void handleButtonClick(int x, int y) {
    for (int i = 0; i < numButtons; i++) {
        Button *b = &buttons[i];
        if (x > b->x && x < b->x + b->w && y > b->y && y < b->y + b->h) {
            if (b->callbackFunction) {
                b->callbackFunction();
            }
        }
    }
}

// Function to handle color picker clicks
void handleColorPickerClick(int x, int y) {
    if (x > 10 && x < 110 && y > 400 && y < 500) {
        float dx = (float)(x - 10) / 100.0;
        float dy = (float)(y - 400) / 100.0;
        currentColor[0] = dx;
        currentColor[1] = dy;
        currentColor[2] = 1.0 - dx;
    }
}

// Mouse button callback
void mouseButton(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        if (x < 120) {
            handleButtonClick(x, y);
            handleColorPickerClick(x, y);
        } else {
            prevX = x;
            prevY = y;
            // Start new stroke
            currentStroke = Stroke();
            currentStroke.isEraser = (tool == 2);
            memcpy(currentStroke.color, currentColor, sizeof(float) * 3);
            currentStroke.size = pointSize;
        }
    }
    if (state == GLUT_UP) {
        if (prevX != -1 && prevY != -1) {
            // End stroke
            strokes.push_back(currentStroke);
        }
        prevX = -1;
        prevY = -1;
    }
}

// Mouse motion callback
void mouseMotion(int x, int y) {
    if (prevX != -1 && prevY != -1 && x > 120) {
        Line line;
        line.x1 = prevX;
        line.y1 = prevY;
        line.x2 = x;
        line.y2 = y;
        line.isEraser = (tool == 2);
        memcpy(line.color, currentColor, sizeof(float) * 3);
        line.size = pointSize;

        // Add line to current stroke
        currentStroke.lines.push_back(line);

        // Draw the line immediately
        if (line.isEraser) {
            glColor3f(1.0, 1.0, 1.0);
        } else {
            glColor3fv(line.color);
        }
        glLineWidth(line.size);
        glBegin(GL_LINES);
        glVertex2i(line.x1, line.y1);
        glVertex2i(line.x2, line.y2);
        glEnd();
        glFlush();

        prevX = x;
        prevY = y;
    }
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw sidebar background
    glColor3f(0.09, 0.08, 0.23);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(120, 0);
    glVertex2i(120, windowHeight);
    glVertex2i(0, windowHeight);
    glEnd();

    // Draw UI elements
    glColor3f(1.0, 1.0, 1.0);
    drawText(10, 170, "SIZE");

    std::stringstream ss;
    ss << pointSize;
    drawText(10, 195, ss.str().c_str());

    for (int i = 0; i < numButtons; i++) {
        drawButton(&buttons[i]);
    }
    drawColorPicker();

    // Draw all strokes
    drawStrokes();

    glFlush();
}

// Initialize OpenGL settings
void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(2.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, windowWidth, windowHeight, 0.0);
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Whiteboard with Vector Storage");
    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutMainLoop();
    return 0;
}
