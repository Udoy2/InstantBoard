#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <sstream>
#include <vector>
#include <cstring>
#include <cmath>
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
void undoLastStroke() {
    if (!strokes.empty()) {
        strokes.pop_back();
        glutPostRedisplay();
    }
}

// Define buttons
Button buttons[] = {
    {10, 40, 100, 25, "PENCIL (P)", setPencilTool},
    {10, 80, 100, 25, "ERASER (E)", setEraserTool},
    {10, 120, 100, 25, "UNDO (U)", undoLastStroke}, // Undo button added here
    {10, 160, 100, 25, "CLEAR", clearScreen},
    {30, 200, 30, 25, "+", increasePointSize}, // Moved to the end
    {70, 200, 30, 25, "-", decreasePointSize}  // Moved to the end
};
int numButtons = 6;


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
    glBegin(GL_LINE);  // Fixed: Changed GL_LINE to GL_LINE
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    // Draw button label
    glColor3f(0.0, 0.0, 0.0);
    drawText(b->x + 10, b->y + 15, b->label);

}

// Helper function to check if a point is inside a circle
bool isInsideCircle(int x, int y, int centerX, int centerY, int radius) {
    int dx = x - centerX;
    int dy = y - centerY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

// Helper function to convert HSV to RGB
void HSVtoRGB(float h, float s, float v, float& r, float& g, float& b) {
    if (s == 0) {
        r = g = b = v;
        return;
    }

    h = h * 6.0f; // sector 0 to 5
    int i = floor(h);
    float f = h - i; // fractional part of h
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
}

// Modified color picker drawing function
void drawColorPicker() {
    int centerX = 60;  // Center of the color wheel
    int centerY = 450;
    int radius = 50;   // Radius of the color wheel
    int segments = 32; // Number of segments for smooth circle

    // Draw color wheel
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0, 1.0, 1.0);  // Center white
    glVertex2f(centerX, centerY);

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float hue = i / (float)segments;
        float r, g, b;
        HSVtoRGB(hue, 1.0f, 1.0f, r, g, b);
        glColor3f(r, g, b);
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // Draw border
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE);  // Fixed: Changed GL_LINE to GL_LINE
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // Draw current color preview
    glBegin(GL_QUADS);
    glColor3fv(currentColor);
    glVertex2i(10, 520);
    glVertex2i(110, 520);
    glVertex2i(110, 550);
    glVertex2i(10, 550);
    glEnd();
}

// Function to draw all strokes
void drawStrokes() {
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

void handleColorPickerClick(int x, int y) {
    int centerX = 60;
    int centerY = 450;
    int radius = 50;

    if (isInsideCircle(x, y, centerX, centerY, radius)) {
        // Calculate angle and distance from center
        float dx = x - centerX;
        float dy = y - centerY;
        float angle = atan2(dy, dx);
        if (angle < 0) angle += 2 * M_PI;

        float distance = sqrt(dx * dx + dy * dy);
        float saturation = std::min(distance / radius, 1.0f);

        // Convert angle to hue (0-1)
        float hue = angle / (2 * M_PI);

        // Convert HSV to RGB
        HSVtoRGB(hue, saturation, 1.0f,
                 currentColor[0],
                 currentColor[1],
                 currentColor[2]);
        glutPostRedisplay();
    }
}

// Keyboard callback function
void keyboard(unsigned char key, int x, int y) {
    switch (tolower(key)) {
        case 'p':
            setPencilTool();
            glutPostRedisplay();
            break;
        case 'e':
            setEraserTool();
            glutPostRedisplay();
            break;
        case 'u':  // Undo functionality
            undoLastStroke();
            break;
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
    drawText(10, 215, ss.str().c_str());

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
    glutKeyboardFunc(keyboard);  // Added keyboard callback
    glutMainLoop();
    return 0;
}
