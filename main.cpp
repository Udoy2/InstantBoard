#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <sstream>
#include <vector>
#include <cstring>
#include <cmath>

// Function prototypes
void toggleSidebar();
void animateSidebar();

bool isSidebarVisible = true; // Track the sidebar visibility state
float sidebarPosition = 0.0f; // Animation state



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
int circleCenterX = -1, circleCenterY = -1;
int windowWidth = 800, windowHeight = 600;
int tool = 1; // 1 for pencil, 2 for eraser
float currentColor[3] = {0.0, 0.0, 0.0}; // Initial color black
int pointSize = 2;
// square starting point variables
int squareStartX = -1, squareStartY = -1;

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
void setCircleTool() { tool = 3; }
void setSquareTool() { tool = 4; }

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

Button buttons[] = {
    {10, 10, 100, 25, "CLOSE", toggleSidebar},
    {10, 40, 100, 25, "PENCIL (P)", setPencilTool},
    {10, 80, 100, 25, "ERASER (E)", setEraserTool},
    {10, 120, 100, 25, "CIRCLE (C)", setCircleTool},
    {10, 160, 100, 25, "SQUARE (S)", setSquareTool},
    {10, 200, 100, 25, "UNDO (U)", undoLastStroke},
    {10, 240, 100, 25, "CLEAR", clearScreen},
    {30, 280, 30, 25, "+", increasePointSize},
    {70, 280, 30, 25, "-", decreasePointSize}
};
int numButtons = 9;

Button smallToggleButton = {5, 10, 30, 25, ">", toggleSidebar};

// Function to toggle the sidebar visibility
void toggleSidebar() {
    isSidebarVisible = !isSidebarVisible;
    glutIdleFunc(animateSidebar);
}

void animateSidebar() {
    float targetPosition = isSidebarVisible ? 0.0f : -120.0f;
    float epsilon = 0.1f;

    if (std::abs(sidebarPosition - targetPosition) > epsilon) {
        sidebarPosition += (targetPosition - sidebarPosition) * 0.02f; // Reduce step size for slower animation
        glutPostRedisplay();
    } else {
        sidebarPosition = targetPosition;
        glutIdleFunc(NULL); // Stop the animation once it reaches the target
    }
}
// Function to draw text
void drawText(int x, int y, const char *text) {
    glRasterPos2i(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *text++);
    }
}

void drawBoldText(int x, int y, const char *text, int boldness) {
    // Draw text multiple times with slight offsets
    for (int dx = -boldness; dx <= boldness; ++dx) {
        for (int dy = -boldness; dy <= boldness; ++dy) {
            glRasterPos2i(x + dx, y + dy);
            const char *p;
            for (p = text; *p; p++) {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
            }
        }
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

    // Save current line width
    GLfloat currentWidth;
    glGetFloatv(GL_LINE_WIDTH, &currentWidth);

    // Set line width for border
    glLineWidth(1.0);

    // Draw button border
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    // Restore previous line width
    glLineWidth(currentWidth);

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
    int centerY = windowHeight - 150;
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
    glBegin(GL_LINE_LOOP);  // Changed from GL_LINE to GL_LINE_LOOP
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
    glVertex2i(10, windowHeight-80);
    glVertex2i(110, windowHeight-80);
    glVertex2i(110, windowHeight-50);
    glVertex2i(10, windowHeight-50);
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
    if (isSidebarVisible) {
        for (int i = 0; i < numButtons; i++) {
            Button *b = &buttons[i];
            if (x > b->x + sidebarPosition &&
                x < b->x + b->w + sidebarPosition &&
                y > b->y &&
                y < b->y + b->h) {
                if (b->callbackFunction) {
                    b->callbackFunction();
                }
                return;
            }
        }
    } else {
        float adjustedX = sidebarPosition + 120;
        if (x > adjustedX &&
            x < adjustedX + smallToggleButton.w &&
            y > smallToggleButton.y &&
            y < smallToggleButton.y + smallToggleButton.h) {
            toggleSidebar();
        }
    }
}

void handleColorPickerClick(int x, int y) {
    int centerX = 60;
    int centerY = windowHeight - 150;
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
        case 'c':
            setCircleTool();
            glutPostRedisplay();
            break;
        case 's':
            setSquareTool();
            glutPostRedisplay();
            break;
        case 'u':
            undoLastStroke();
            break;
        case '[':
            decreasePointSize();
            break;
        case ']':
            increasePointSize();
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
            // Start new stroke, circle, or square
            if (tool == 3) {
                circleCenterX = x;
                circleCenterY = y;
            } else if (tool == 4) {
                squareStartX = x;
                squareStartY = y;
            } else {
                currentStroke = Stroke();
                currentStroke.isEraser = (tool == 2);
                memcpy(currentStroke.color, currentColor, sizeof(float) * 3);
                currentStroke.size = pointSize;
            }
        }
    }
    if (state == GLUT_UP) {
        if (prevX != -1 && prevY != -1) {
            // End stroke or circle
            if (tool == 3 && circleCenterX != -1 && circleCenterY != -1) {
                int radius = std::sqrt(std::pow(x - circleCenterX, 2) + std::pow(y - circleCenterY, 2));
                Stroke circleStroke;
                circleStroke.isEraser = false;
                memcpy(circleStroke.color, currentColor, sizeof(float) * 3);
                circleStroke.size = pointSize;

                // Draw the final circle using line segments
                int segments = 200; // Increase segments for smoother circle
                for (int i = 0; i < segments; ++i) {
                    float theta1 = 2.0f * M_PI * float(i) / float(segments);
                    float theta2 = 2.0f * M_PI * float(i + 1) / float(segments);
                    Line line;
                    line.x1 = circleCenterX + radius * std::cos(theta1);
                    line.y1 = circleCenterY + radius * std::sin(theta1);
                    line.x2 = circleCenterX + radius * std::cos(theta2);
                    line.y2 = circleCenterY + radius * std::sin(theta2);
                    line.size = pointSize;
                    memcpy(line.color, currentColor, sizeof(float) * 3);
                    circleStroke.lines.push_back(line);
                }

                strokes.push_back(circleStroke);
                circleCenterX = -1;
                circleCenterY = -1;
                glutPostRedisplay();
            }else if (tool == 4 && squareStartX != -1 && squareStartY != -1) {
                // Create square stroke
                Stroke squareStroke;
                squareStroke.isEraser = false;
                memcpy(squareStroke.color, currentColor, sizeof(float) * 3);
                squareStroke.size = pointSize;

                // Calculate square corners
                Line line;
                line.size = pointSize;
                memcpy(line.color, currentColor, sizeof(float) * 3);

                // Draw four sides of the square
                // Top line
                line.x1 = squareStartX;
                line.y1 = squareStartY;
                line.x2 = x;
                line.y2 = squareStartY;
                squareStroke.lines.push_back(line);

                // Right line
                line.x1 = x;
                line.y1 = squareStartY;
                line.x2 = x;
                line.y2 = y;
                squareStroke.lines.push_back(line);

                // Bottom line
                line.x1 = x;
                line.y1 = y;
                line.x2 = squareStartX;
                line.y2 = y;
                squareStroke.lines.push_back(line);

                // Left line
                line.x1 = squareStartX;
                line.y1 = y;
                line.x2 = squareStartX;
                line.y2 = squareStartY;
                squareStroke.lines.push_back(line);

                strokes.push_back(squareStroke);
                squareStartX = -1;
                squareStartY = -1;
                glutPostRedisplay();
            } else if (tool != 3 && tool != 4) {
                strokes.push_back(currentStroke);
            }
        }
        prevX = -1;
        prevY = -1;
    }
}

void display();

// Mouse motion callback
void mouseMotion(int x, int y) {
    if (prevX != -1 && prevY != -1 && x > 120) {
        if (tool == 3 && circleCenterX != -1 && circleCenterY != -1) {
            // Calculate the radius of the circle
            int radius = std::sqrt(std::pow(x - circleCenterX, 2) + std::pow(y - circleCenterY, 2));

            // Clear the window before drawing the new circle
            glClear(GL_COLOR_BUFFER_BIT);
            display(); // Redraw the display without the temporary circle

            // Draw the new circle for feedback
            glColor3fv(currentColor);
            glLineWidth(pointSize);
            glBegin(GL_LINE_LOOP);
            int segments = 200; // Increase segments for smoother circle
            for (int i = 0; i < segments; ++i) {
                float theta = 2.0f * M_PI * float(i) / float(segments);
                float dx = radius * std::cos(theta);
                float dy = radius * std::sin(theta);
                glVertex2f(circleCenterX + dx, circleCenterY + dy);
            }
            glEnd();
            glFlush();
        } else if (tool == 4 && squareStartX != -1 && squareStartY != -1) {
            // Clear and redraw for square preview
            glClear(GL_COLOR_BUFFER_BIT);
            display();

            // Draw preview square
            glColor3fv(currentColor);
            glLineWidth(pointSize);
            glBegin(GL_LINE_LOOP);
            glVertex2i(squareStartX, squareStartY);
            glVertex2i(x, squareStartY);
            glVertex2i(x, y);
            glVertex2i(squareStartX, y);
            glEnd();
            glFlush();
        } else if (tool != 3 && tool != 4) {
            // Existing line drawing code...
            Line line;
            line.x1 = prevX;
            line.y1 = prevY;
            line.x2 = x;
            line.y2 = y;
            line.isEraser = (tool == 2);
            memcpy(line.color, currentColor, sizeof(float) * 3);
            line.size = pointSize;

            currentStroke.lines.push_back(line);

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
}

void reshape(int w, int h) {
    // Update the window size variables
    windowWidth = w;
    windowHeight = h;

    // Update the OpenGL viewport
    glViewport(0, 0, w, h);

    // Adjust the projection matrix to match the new window size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, windowWidth, windowHeight, 0.0);

    // Redraw the window with the updated size
    glutPostRedisplay();
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw sidebar background based on sidebarPosition
    glColor3f(0.09, 0.08, 0.23);
    glBegin(GL_QUADS);
    glVertex2i(sidebarPosition, 0);
    glVertex2i(120 + sidebarPosition, 0);
    glVertex2i(120 + sidebarPosition, windowHeight);
    glVertex2i(sidebarPosition, windowHeight);
    glEnd();

    // Calculate alpha value based on sidebarPosition for fade-in effect
    float alpha = (sidebarPosition + 120.0f) / 120.0f; // Interpolate from 0 to 1

    // Draw UI elements based on sidebarPosition only if sidebar is visible
    if (isSidebarVisible) {
        glColor4f(1.0, 1.0, 1.0, alpha);

        // Draw all buttons first
        for (int i = 0; i < numButtons; i++) {
            glColor4f(1.0, 1.0, 1.0, alpha); // Set button color with alpha
            drawButton(&buttons[i]);
        }

        // Draw point size text
        std::stringstream ss;
        ss  << pointSize;  // Added "Size:" label for clarity

        // Position the text above the +/- buttons
        glColor4f(1.0, 1.0, 1.0, alpha); // Set text color with alpha
        drawBoldText(10 + sidebarPosition, 298, ss.str().c_str(), 0.5);

        // Draw color picker
        drawColorPicker();
    } else {
        // Draw the small toggle button with fade-in effect
        glColor4f(1.0, 1.0, 1.0, alpha); // Set button color with alpha
        drawButton(&smallToggleButton);
    }

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
    glutReshapeFunc(reshape);
    init();
    glutDisplayFunc(display);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
