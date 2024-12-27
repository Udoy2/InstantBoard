#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

// Variables for mouse state
int prevX = -1, prevY = -1;
int windowWidth = 800, windowHeight = 600;
int tool = 1; // 1 for pencil, 2 for eraser
float currentColor[3] = {0.0, 0.0, 0.0}; // Initial color black
int pointSize = 2;

// Button structure
typedef struct {
    int x, y, w, h;
    const char *label; // Use const char* to fix warnings
    void (*callbackFunction)();
} Button;

// Button callback functions
void setPencilTool() { tool = 1; }
void setEraserTool() { tool = 2; }
void clearScreen() {
    glEnable(GL_SCISSOR_TEST);                // Enable scissor testing
    glScissor(120, 0, windowWidth - 120, windowHeight); // Restrict clearing to the drawing area
    glClear(GL_COLOR_BUFFER_BIT);            // Clear only the drawing area
    glDisable(GL_SCISSOR_TEST);              // Disable scissor testing
    glFlush();
}
void increasePointSize() { if (pointSize < 10) pointSize++; }
void decreasePointSize() { if (pointSize > 1) pointSize--; }

// Define buttons
Button buttons[] = {
    {10, 40, 100, 25, "PENCIL", setPencilTool},
    {10, 80, 100, 25, "ERASER", setEraserTool},
    {10, 120, 100, 25, "CLEAR", clearScreen},
    {30, 180, 30, 25, "+", increasePointSize}, // Adjusted position for +
    {70, 180, 30, 25, "-", decreasePointSize}  // Adjusted position for -
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
    glColor3f(1.0, 0.84, 0.77); // RGB for #FFD7C4
    glBegin(GL_QUADS);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    // Draw button border
    glColor3f(0.0, 0.0, 0.0); // Black border
    glBegin(GL_LINE_LOOP);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    // Draw button label
    glColor3f(0.0, 0.0, 0.0); // Black text
    drawText(b->x + 10, b->y + 15, b->label);
}

// Function to draw color picker
void drawColorPicker() {
    glBegin(GL_QUADS);
    glColor3f(1.0, 0.0, 0.0); glVertex2i(10, 500); // Red
    glColor3f(0.0, 1.0, 0.0); glVertex2i(110, 500); // Green
    glColor3f(0.0, 0.0, 1.0); glVertex2i(110, 400); // Blue
    glColor3f(1.0, 1.0, 0.0); glVertex2i(10, 400); // Yellow
    glEnd();
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
        currentColor[0] = dx; // Red
        currentColor[1] = dy; // Green
        currentColor[2] = 1.0 - dx; // Blue
    }
}

// Mouse button callback (start/stop drawing or handle clicks)
void mouseButton(int button, int state, int x, int y) {
    if (state == GLUT_DOWN) {
        if (x < 120) {
            handleButtonClick(x, y);
            handleColorPickerClick(x, y);
        } else {
            prevX = x;
            prevY = y;
        }
    }
    if (state == GLUT_UP) {
        prevX = -1;
        prevY = -1;
    }
}

// Mouse motion callback (handle drawing)
void mouseMotion(int x, int y) {
    if (prevX != -1 && prevY != -1 && x > 120) {
        if (tool == 2) {
            float eraserColor[] = {1.0, 1.0, 1.0};
            glColor3fv(eraserColor);  // Use eraser color
        } else {
            glColor3fv(currentColor);  // Use selected color
        }
        glLineWidth(pointSize);
        glBegin(GL_LINES);
        glVertex2i(prevX, prevY);
        glVertex2i(x, y);
        glEnd();
        glFlush();
        prevX = x;
        prevY = y;
    }
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Sidebar background with #17153B
    glColor3f(0.09, 0.08, 0.23); // RGB for #17153B
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(120, 0);
    glVertex2i(120, windowHeight);
    glVertex2i(0, windowHeight);
    glEnd();

    // Draw the "SIZE" label centered
    glColor3f(1.0, 1.0, 1.0); // White text for "SIZE"
    drawText(40, 170, "SIZE");

    for (int i = 0; i < numButtons; i++) {
        drawButton(&buttons[i]);
    }
    drawColorPicker();
    glFlush();
}

// Initialize OpenGL settings
void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0); // White background
    glColor3f(0.0, 0.0, 0.0);         // Black drawing color
    glPointSize(2.0);                 // Set point size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, windowWidth, windowHeight, 0.0); // Top-left origin
}

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Whiteboard with Sidebar");
    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutMainLoop();
    return 0;
}
