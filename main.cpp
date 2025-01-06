#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <sstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <math.h>

template <typename T>
std::string toString(T value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}

const int BOTTOM_TOOLBAR_HEIGHT = 60;
const int BOTTOM_BUTTON_WIDTH = 80;
const int BOTTOM_BUTTON_HEIGHT = 40;
const int BOTTOM_BUTTON_SPACING = 20;
const int BOTTOM_MARGIN = 20;
const int BOTTOM_BUTTON_SIZE = 40; // Add this line

// Update the sidebar and thumbnail dimensions
const int RIGHT_SIDEBAR_WIDTH = 180; // Increased from 120 to 180
const int THUMBNAIL_HEIGHT = 100;    // Increased from 80 to 100
const int THUMBNAIL_WIDTH = 180;     // Increased from 120 to 160

// Add to global variables
float scrollOffset = 0.0f;
const float SCROLL_SPEED = 20.0f;
int totalContentHeight = 0;
int selectedBottomTool = 1; // Track currently selected bottom tool (1 for pencil by default)

// Variables for mouse state
int prevX = -1, prevY = -1;
int circleCenterX = -1, circleCenterY = -1;
int windowWidth = 800, windowHeight = 600;
int tool = 1;                            // 1 for pencil, 2 for eraser
float currentColor[3] = {0.0, 0.0, 0.0}; // Initial color black
int pointSize = 2;
// square starting point variables
int squareStartX = -1, squareStartY = -1;

// Function prototypes
void toggleSidebar();
void animateSidebar();
void mouseWheel(int wheel, int direction, int x, int y);

bool isSidebarVisible = true; // Track the sidebar visibility state
float sidebarPosition = 0.0f; // Animation state

bool isRightSidebarVisible = false;               // Right sidebar is closed by default
float rightSidebarPosition = RIGHT_SIDEBAR_WIDTH; // Initial position for the closed state

void toggleRightSidebar();
void animateRightSidebar();

// Structure to represent a line segment
struct Line
{
    int x1, y1, x2, y2;
    float color[3];
    int size;
    bool isEraser;
};

// Structure to represent a stroke (collection of connected line segments)
struct Stroke
{
    std::vector<Line> lines;
    float color[3];
    int size;
    bool isEraser;
};

struct Board
{
    std::string name;
    std::vector<Stroke> strokes;
    float currentColor[3];
    int pointSize;
    int tool;
};

std::vector<Board> boards;
int currentBoardIndex = 0;

// Vector to store all strokes
std::vector<Stroke> strokes;
Stroke currentStroke;

// Button structure
typedef struct
{
    int x, y, w, h;
    const char *label;
    void (*callbackFunction)();
} Button;

// Button callback functions
void setPencilTool()
{
    tool = 1;
    selectedBottomTool = 0;
    glutPostRedisplay();
}

void setEraserTool()
{
    tool = 2;
    selectedBottomTool = 1;
    glutPostRedisplay();
}

void setCircleTool()
{
    tool = 3;
}
void setSquareTool()
{
    tool = 4;
}

void clearScreen()
{
    strokes.clear();
    glutPostRedisplay();
}

// Add these global variables
static bool canAdjustSize = true;
const int COOLDOWN_MS = 200; // 200ms cooldown

void resetSizeAdjustFlag(int value)
{
    canAdjustSize = true;
}

void increasePointSize()
{
    if (canAdjustSize && pointSize < 50)
    {
        pointSize++;
        canAdjustSize = false;
        glutTimerFunc(COOLDOWN_MS, resetSizeAdjustFlag, 0);
        glutPostRedisplay();
    }
}

void decreasePointSize()
{
    if (canAdjustSize && pointSize > 1)
    {
        pointSize--;
        canAdjustSize = false;
        glutTimerFunc(COOLDOWN_MS, resetSizeAdjustFlag, 0);
        glutPostRedisplay();
    }
}

void undoLastStroke()
{
    if (!strokes.empty())
    {
        strokes.pop_back();
        glutPostRedisplay();
    }
}

void deleteCurrentBoard()
{
    // If only one board exists, act like clear button
    if (boards.size() == 1)
    {
        clearScreen();
        return;
    }

    // For multiple boards:
    // Save states of all boards except current one
    std::vector<Board> tempBoards;
    for (size_t i = 0; i < boards.size(); i++)
    {
        if (i != currentBoardIndex)
        {
            tempBoards.push_back(boards[i]);
        }
    }

    // Update boards vector
    boards = tempBoards;

    // Switch to previous board
    currentBoardIndex = std::max(0, currentBoardIndex - 1);

    // Load the previous board's state
    strokes = boards[currentBoardIndex].strokes;
    pointSize = boards[currentBoardIndex].pointSize;
    memcpy(currentColor, boards[currentBoardIndex].currentColor, sizeof(float) * 3);
    tool = boards[currentBoardIndex].tool;

    // Update scroll and content height
    totalContentHeight = (boards.size() * (THUMBNAIL_HEIGHT + 30)) + 35;

    glutPostRedisplay();
}

void drawText(int x, int y, const char *text)
{
    glRasterPos2i(x, y);
    while (*text)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *text++);
    }
}

Button buttons[] =
    {
        {10, 10, 100, 25, "<<", toggleSidebar},
        {10, 40, 100, 25, "PENCIL (P)", setPencilTool},
        {10, 80, 100, 25, "ERASER (E)", setEraserTool},
        {10, 120, 100, 25, "CIRCLE (C)", setCircleTool},
        {10, 160, 100, 25, "SQUARE (S)", setSquareTool},
        {10, 200, 100, 25, "UNDO (U)", undoLastStroke},
        {10, 240, 100, 25, "CLEAR", clearScreen},
        {30, 280, 30, 25, "+", increasePointSize},
        {70, 280, 30, 25, "-", decreasePointSize},
        {windowWidth - RIGHT_SIDEBAR_WIDTH + 10, 10, 100, 25, "<", toggleRightSidebar}

};
int numButtons = 10;

// Use the existing Button struct for bottom buttons
Button bottomButtons[] =
    {
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", setPencilTool},
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", setEraserTool},
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", clearScreen},
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", deleteCurrentBoard}};
const int NUM_BOTTOM_BUTTONS = 4;

// Function to draw the rotated clear button icon
void drawRotatedClearButtonIcon(Button *b)
{
    glPushMatrix();
    glTranslatef(b->x + 20, b->y + 20, 0);
    glRotatef(45.0f, 0, 0, 1);
    glTranslatef(-(b->x + 20), -(b->y + 20), 0);

    // Draw larger triangle (flipped vertically)
    glBegin(GL_TRIANGLES);
    glVertex2i(b->x + 10, b->y + 30); // Bottom left
    glVertex2i(b->x + 30, b->y + 30); // Bottom right
    glVertex2i(b->x + 20, b->y + 10); // Top center
    glEnd();

    // Draw larger line upon the triangle
    glBegin(GL_LINES);
    glVertex2i(b->x + 16, b->y + 10); // Line start
    glVertex2i(b->x + 24, b->y + 10); // Line end
    glEnd();

    // Draw 2 larger white lines inside the triangle
    glColor3f(1.0, 1.0, 1.0); // White color
    glBegin(GL_LINES);
    glVertex2i(b->x + 14, b->y + 20); // First line start
    glVertex2i(b->x + 18, b->y + 20); // First line end

    glVertex2i(b->x + 22, b->y + 20); // Second line start
    glVertex2i(b->x + 26, b->y + 20); // Second line end
    glEnd();

    glPopMatrix();
}

void drawIcon(Button *b, int iconType)
{
    glColor3f(0.0, 0.0, 0.0);
    glLineWidth(2.0);

    switch (iconType)
    {
    case 0: // Pencil (head downwards and more pointed)
        // Pencil body
        glBegin(GL_QUADS);
        glVertex2i(b->x + 16, b->y + 12);
        glVertex2i(b->x + 24, b->y + 12);
        glVertex2i(b->x + 24, b->y + 28);
        glVertex2i(b->x + 16, b->y + 28);
        glEnd();

        // Pencil tip
        glBegin(GL_TRIANGLES);
        glVertex2i(b->x + 16, b->y + 12);
        glVertex2i(b->x + 20, b->y + 4);
        glVertex2i(b->x + 24, b->y + 12);
        glEnd();
        break;

    case 1: // Eraser
        // Draw rectangle part
        glBegin(GL_QUADS);
        glVertex2i(b->x + 12, b->y + 15);
        glVertex2i(b->x + 28, b->y + 15);
        glVertex2i(b->x + 28, b->y + 25);
        glVertex2i(b->x + 12, b->y + 25);
        glEnd();
        // Draw rounded part
        glBegin(GL_POLYGON);
        for (int i = 0; i <= 360; i++)
        {
            float theta = i * 3.14159 / 180;
            glVertex2f(b->x + 10 + 2 * cos(theta), b->y + 20 + 5 * sin(theta));
        }
        glEnd();
        break;

    case 2:                            // Clear (cleaning brush icon)
        drawRotatedClearButtonIcon(b); // Draw the larger clear button icon at a fixed 45 degree angle with white lines
        break;

    case 3: // Delete (trash can with lid)
        // Lid
        glBegin(GL_LINE_STRIP);
        glVertex2i(b->x + 12, b->y + 15);
        glVertex2i(b->x + 28, b->y + 15);
        // Can
        glVertex2i(b->x + 26, b->y + 30);
        glVertex2i(b->x + 14, b->y + 30);
        glVertex2i(b->x + 12, b->y + 15);
        glEnd();

        // Vertical lines inside can
        glBegin(GL_LINES);
        glVertex2i(b->x + 17, b->y + 18);
        glVertex2i(b->x + 17, b->y + 27);
        glVertex2i(b->x + 20, b->y + 18);
        glVertex2i(b->x + 20, b->y + 27);
        glVertex2i(b->x + 23, b->y + 18);
        glVertex2i(b->x + 23, b->y + 27);
        glEnd();
        break;
    }
}

void drawButton(Button *b);

// Modify drawBottomToolbar function
void drawBottomToolbar()
{
    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++)
    {
        // Draw button background
        bool isSelected = false;

        // Check if button should be highlighted
        if (i == 0)
            isSelected = (tool == 1); // Pencil
        if (i == 1)
            isSelected = (tool == 2); // Eraser

        // Set background color based on selection state
        if (isSelected)
        {
            glColor3f(1.0, 0.7, 0.6); // Darker highlight color
        }
        else
        {
            glColor3f(1.0, 0.84, 0.77); // Original color
        }

        // Draw button background
        glBegin(GL_QUADS);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y + bottomButtons[i].h);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y + bottomButtons[i].h);
        glEnd();

        // Draw border with thicker line for selected state
        glLineWidth(isSelected ? 2.0 : 1.0);
        glColor3f(0.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y + bottomButtons[i].h);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y + bottomButtons[i].h);
        glEnd();
        glLineWidth(1.0);

        // Draw the icon
        drawIcon(&bottomButtons[i], i);
    }
}

void updateBottomButtonPositions()
{
    int totalWidth = NUM_BOTTOM_BUTTONS * BOTTOM_BUTTON_SIZE +
                     (NUM_BOTTOM_BUTTONS - 1) * BOTTOM_BUTTON_SPACING;
    int startX = (windowWidth - totalWidth) / 2;

    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++)
    {
        bottomButtons[i].x = startX + i * (BOTTOM_BUTTON_SIZE + BOTTOM_BUTTON_SPACING);
        bottomButtons[i].y = windowHeight - BOTTOM_MARGIN - BOTTOM_BUTTON_SIZE;
    }
}

Button smallToggleButton = {5, 10, 30, 25, ">", toggleSidebar};

void createNewBoard()
{
    if (boards.size() >= 5)
    {
        return;
    }

    // Save current board state
    if (!boards.empty())
    {
        boards[currentBoardIndex].strokes = strokes;
        boards[currentBoardIndex].pointSize = pointSize;
        memcpy(boards[currentBoardIndex].currentColor, currentColor, sizeof(float) * 3);
        boards[currentBoardIndex].tool = tool;
    }

    // Create new blank board
    Board newBoard;
    newBoard.name = "Board " + toString(boards.size() + 1) + "/5";
    newBoard.pointSize = 2;
    newBoard.tool = 1;
    newBoard.strokes.clear(); // Ensure strokes are empty
    memcpy(newBoard.currentColor, currentColor, sizeof(float) * 3);

    // Add new board and switch to it
    boards.push_back(newBoard);
    currentBoardIndex = boards.size() - 1;

    // Clear current drawing area
    strokes.clear();

    glutPostRedisplay();
}

void switchToBoard(int index)
{
    if (index >= 0 && index < boards.size())
    {
        // Save current board state
        if (!boards.empty())
        {
            boards[currentBoardIndex].strokes = strokes;
            boards[currentBoardIndex].pointSize = pointSize;
            memcpy(boards[currentBoardIndex].currentColor, currentColor, sizeof(float) * 3);
            boards[currentBoardIndex].tool = tool;
        }

        // Load new board state
        currentBoardIndex = index;
        strokes = boards[index].strokes;
        pointSize = boards[index].pointSize;
        memcpy(currentColor, boards[index].currentColor, sizeof(float) * 3);
        tool = boards[index].tool;
        glutPostRedisplay();
    }
}

void drawBoardThumbnail(int index, int x, int y)
{
    // Draw thumbnail background
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + THUMBNAIL_WIDTH - 20, y);
    glVertex2i(x + THUMBNAIL_WIDTH - 20, y + THUMBNAIL_HEIGHT);
    glVertex2i(x, y + THUMBNAIL_HEIGHT);
    glEnd();

    // Draw border
    if (index == currentBoardIndex)
    {
        glColor3f(0.0, 0.7, 1.0); // Highlight current board
        glLineWidth(2.0);
    }
    else
    {
        glColor3f(0.0, 0.0, 0.0);
        glLineWidth(1.0);
    }

    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + THUMBNAIL_WIDTH - 20, y);
    glVertex2i(x + THUMBNAIL_WIDTH - 20, y + THUMBNAIL_HEIGHT);
    glVertex2i(x, y + THUMBNAIL_HEIGHT);
    glEnd();

    // Draw board name
    glColor3f(1.0, 1.0, 1.0);
    std::string boardName = "Board " + toString(index + 1) + "/5";
    drawText(x + 5, y + THUMBNAIL_HEIGHT + 15, boardName.c_str());
}

void handleBoardClick(int x, int y)
{
    float yOffset = 50 + scrollOffset; // Start from the adjusted offset

    // Check existing board clicks
    for (size_t i = 0; i < boards.size(); i++)
    {
        if (y >= yOffset && y <= yOffset + THUMBNAIL_HEIGHT &&
            x >= windowWidth - RIGHT_SIDEBAR_WIDTH + 10 &&
            x <= windowWidth - RIGHT_SIDEBAR_WIDTH + THUMBNAIL_WIDTH - 10)
        {
            switchToBoard(i);
            return;
        }
        yOffset += THUMBNAIL_HEIGHT + 30;
    }

    // Check "+ New Board" button click
    if (boards.size() < 5) // Only if we can add more boards
    {
        if (y >= yOffset && y <= yOffset + 25 &&
            x >= windowWidth - RIGHT_SIDEBAR_WIDTH + 10 + rightSidebarPosition &&
            x <= windowWidth - RIGHT_SIDEBAR_WIDTH + 90 + rightSidebarPosition)
        {
            createNewBoard();
            return;
        }
    }
}

// Function to toggle the sidebar visibility
void toggleSidebar()
{
    isSidebarVisible = !isSidebarVisible;
    glutIdleFunc(animateSidebar);
}

void animateSidebar()
{
    float targetPosition = isSidebarVisible ? 0.0f : -120.0f;
    float epsilon = 0.1f;

    if (std::abs(sidebarPosition - targetPosition) > epsilon)
    {
        sidebarPosition += (targetPosition - sidebarPosition) * 0.02f; // Reduce step size for slower animation
        glutPostRedisplay();
    }
    else
    {
        sidebarPosition = targetPosition;
        glutIdleFunc(NULL); // Stop the animation once it reaches the target
    }
}

void toggleRightSidebar()
{
    isRightSidebarVisible = !isRightSidebarVisible;
    glutIdleFunc(animateRightSidebar);
}

void animateRightSidebar()
{
    float targetPosition = isRightSidebarVisible ? 0.0f : RIGHT_SIDEBAR_WIDTH;
    float epsilon = 0.1f;

    if (std::abs(rightSidebarPosition - targetPosition) > epsilon)
    {
        rightSidebarPosition += (targetPosition - rightSidebarPosition) * 0.2f; // Increased animation speed
        glutPostRedisplay();
    }
    else
    {
        rightSidebarPosition = targetPosition;
        glutIdleFunc(NULL);
    }
}

void drawBoldText(int x, int y, const char *text, int boldness)
{
    // Draw text multiple times with slight offsets
    for (int dx = -boldness; dx <= boldness; ++dx)
    {
        for (int dy = -boldness; dy <= boldness; ++dy)
        {
            glRasterPos2i(x + dx, y + dy);
            const char *p;
            for (p = text; *p; p++)
            {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *p);
            }
        }
    }
}

// Function to draw buttons
void drawButton(Button *b)
{
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
bool isInsideCircle(int x, int y, int centerX, int centerY, int radius)
{
    int dx = x - centerX;
    int dy = y - centerY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

// Helper function to convert HSV to RGB
void HSVtoRGB(float h, float s, float v, float &r, float &g, float &b)
{
    if (s == 0)
    {
        r = g = b = v;
        return;
    }

    h = h * 6.0f; // sector 0 to 5
    int i = floor(h);
    float f = h - i; // fractional part of h
    float p = v * (1 - s);
    float q = v * (1 - s * f);
    float t = v * (1 - s * (1 - f));

    switch (i % 6)
    {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    }
}

// Modified color picker drawing function
void drawColorPicker()
{
    int centerX = 60; // Center of the color wheel
    int centerY = windowHeight - 150;
    int radius = 50;   // Radius of the color wheel
    int segments = 32; // Number of segments for smooth circle

    // Draw color wheel
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0, 1.0, 1.0); // Center white
    glVertex2f(centerX, centerY);

    for (int i = 0; i <= segments; i++)
    {
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
    glBegin(GL_LINE_LOOP); // Changed from GL_LINE to GL_LINE_LOOP
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    // Draw current color preview
    glBegin(GL_QUADS);
    glColor3fv(currentColor);
    glVertex2i(10, windowHeight - 80);
    glVertex2i(110, windowHeight - 80);
    glVertex2i(110, windowHeight - 50);
    glVertex2i(10, windowHeight - 50);
    glEnd();
}

// Function to draw all strokes
void drawStrokes()
{
    for (size_t i = 0; i < strokes.size(); i++)
    {
        const Stroke &stroke = strokes[i];
        for (size_t j = 0; j < stroke.lines.size(); j++)
        {
            const Line &line = stroke.lines[j];
            if (line.isEraser)
            {
                glColor3f(1.0, 1.0, 1.0);
            }
            else
            {
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
void handleButtonClick(int x, int y)
{
    if (y >= windowHeight - BOTTOM_MARGIN - BOTTOM_BUTTON_HEIGHT &&
        y <= windowHeight - BOTTOM_MARGIN)
    {
        for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++)
        {
            if (x >= bottomButtons[i].x && x <= bottomButtons[i].x + bottomButtons[i].w)
            {
                if (bottomButtons[i].callbackFunction)
                {
                    bottomButtons[i].callbackFunction();
                }
                return;
            }
        }
    }

    // Left sidebar handling
    if (isSidebarVisible)
    {
        for (int i = 0; i < numButtons - 1; i++)
        {
            Button *b = &buttons[i];
            if (x > b->x + sidebarPosition && x < b->x + b->w + sidebarPosition &&
                y > b->y && y < b->y + b->h)
            {
                if (b->callbackFunction)
                {
                    b->callbackFunction();
                }
                return;
            }
        }
    }
    else
    {
        float adjustedX = sidebarPosition + 120;
        if (x > adjustedX && x < adjustedX + smallToggleButton.w &&
            y > smallToggleButton.y && y < smallToggleButton.y + smallToggleButton.h)
        {
            toggleSidebar();
        }
    }

    // Right sidebar handling
    if (!isRightSidebarVisible)
    {
        // Check click area for the small toggle button
        if (x >= windowWidth - 40 && x <= windowWidth - 10 &&
            y >= 10 && y <= 35)
        {
            toggleRightSidebar();
            return;
        }
    }
    else
    {
        // Check click area for the expanded toggle button
        float adjustedX = windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition;
        if (x >= adjustedX + 10 && x <= adjustedX + 110 &&
            y >= 10 && y <= 35)
        {
            toggleRightSidebar();
            return;
        }
    }
}

void handleColorPickerClick(int x, int y)
{
    int centerX = 60;
    int centerY = windowHeight - 150;
    int radius = 50;

    if (isInsideCircle(x, y, centerX, centerY, radius))
    {
        // Calculate angle and distance from center
        float dx = x - centerX;
        float dy = y - centerY;
        float angle = atan2(dy, dx);
        if (angle < 0)
            angle += 2 * M_PI;

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
void keyboard(unsigned char key, int x, int y)
{
    switch (tolower(key))
    {
    case 'p':
        setPencilTool();
        glutPostRedisplay();
        break;
    case 'd':
        deleteCurrentBoard();
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

void mouseWheel(int wheel, int direction, int x, int y)
{
    if (x >= windowWidth - RIGHT_SIDEBAR_WIDTH)
    {
        if (direction > 0) // Scroll up
        {
            scrollOffset += SCROLL_SPEED;
            if (scrollOffset > 0)
                scrollOffset = 0;
        }
        else // Scroll down
        {
            float maxScroll = -(totalContentHeight - windowHeight + 20);
            scrollOffset -= SCROLL_SPEED;
            if (scrollOffset < maxScroll)
                scrollOffset = maxScroll;
        }
        glutPostRedisplay();
    }
}

// Function to get drawing area based on sidebar visibility
void getDrawingArea(int &drawX, int &drawWidth)
{
    // Default drawing area starts at (0, windowWidth)
    drawX = 0;
    drawWidth = windowWidth;

    // If left sidebar is hidden, adjust the drawing area
    if (isSidebarVisible == true)
    {
        drawX += 120;     // Assuming sidebar width is 120
        drawWidth -= 120; // Reduce drawing width
    }

    // If right sidebar is hidden, adjust the drawing area
    if (isRightSidebarVisible == true)
    {
        drawWidth -= RIGHT_SIDEBAR_WIDTH; // Reduce drawing width by right sidebar width
    }
}

// Mouse button callback
void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        handleButtonClick(x, y);
    }
    if (state == GLUT_DOWN)
    {
        // Handle right sidebar interactions
        if (isRightSidebarVisible)
        {
            if (x >= windowWidth - RIGHT_SIDEBAR_WIDTH)
            {
                // Mouse wheel scrolling
                if (button == 3) // Mouse wheel up
                {
                    scrollOffset += SCROLL_SPEED;
                    if (scrollOffset > 0)
                        scrollOffset = 0;
                    glutPostRedisplay();
                    return;
                }
                else if (button == 4) // Mouse wheel down
                {
                    float maxScroll = -(totalContentHeight - windowHeight + 20);
                    scrollOffset -= SCROLL_SPEED;
                    if (scrollOffset < maxScroll)
                        scrollOffset = maxScroll;
                    glutPostRedisplay();
                    return;
                }
                handleBoardClick(x, y);
                return;
            }
        }

        // Handle left sidebar interactions
        if (isSidebarVisible)
        {
            if (x < 120)
            {
                handleButtonClick(x, y);
                handleColorPickerClick(x, y);
                return;
            }
        }

        // calculate the drawing area
        int drawX, drawWidth;
        getDrawingArea(drawX, drawWidth);
        // Handle drawing area interactions
        if (x >= drawX && x <= (drawX + drawWidth))
        {
            prevX = x;
            prevY = y;

            switch (tool)
            {
            case 3: // Circle
                circleCenterX = x;
                circleCenterY = y;
                break;
            case 4: // Square
                squareStartX = x;
                squareStartY = y;
                break;
            default: // Pencil or Eraser
                currentStroke = Stroke();
                currentStroke.isEraser = (tool == 2);
                memcpy(currentStroke.color, currentColor, sizeof(float) * 3);
                currentStroke.size = pointSize;
                break;
            }
        }
    }

    if (state == GLUT_UP)
    {
        if (prevX != -1 && prevY != -1)
        {
            if (tool == 3 && circleCenterX != -1 && circleCenterY != -1)
            {
                // Complete circle drawing
                int radius = sqrt(pow(x - circleCenterX, 2) + pow(y - circleCenterY, 2));
                Stroke circleStroke;
                circleStroke.isEraser = false;
                memcpy(circleStroke.color, currentColor, sizeof(float) * 3);
                circleStroke.size = pointSize;

                // Create circle using line segments
                int segments = 200;
                for (int i = 0; i < segments; i++)
                {
                    Line line;
                    float theta1 = 2.0f * M_PI * float(i) / float(segments);
                    float theta2 = 2.0f * M_PI * float(i + 1) / float(segments);

                    line.x1 = circleCenterX + radius * cos(theta1);
                    line.y1 = circleCenterY + radius * sin(theta1);
                    line.x2 = circleCenterX + radius * cos(theta2);
                    line.y2 = circleCenterY + radius * sin(theta2);

                    line.size = pointSize;
                    memcpy(line.color, currentColor, sizeof(float) * 3);
                    line.isEraser = false;

                    circleStroke.lines.push_back(line);
                }
                strokes.push_back(circleStroke);
                circleCenterX = -1;
                circleCenterY = -1;
            }
            else if (tool == 4 && squareStartX != -1 && squareStartY != -1)
            {
                // Complete square drawing
                Stroke squareStroke;
                squareStroke.isEraser = false;
                memcpy(squareStroke.color, currentColor, sizeof(float) * 3);
                squareStroke.size = pointSize;

                // Create four lines for the square
                Line lines[4];
                lines[0] = {squareStartX, squareStartY, x, squareStartY};
                lines[1] = {x, squareStartY, x, y};
                lines[2] = {x, y, squareStartX, y};
                lines[3] = {squareStartX, y, squareStartX, squareStartY};

                for (int i = 0; i < 4; i++)
                {
                    lines[i].size = pointSize;
                    memcpy(lines[i].color, currentColor, sizeof(float) * 3);
                    lines[i].isEraser = false;
                    squareStroke.lines.push_back(lines[i]);
                }

                strokes.push_back(squareStroke);
                squareStartX = -1;
                squareStartY = -1;
            }
            else if (tool != 3 && tool != 4)
            {
                strokes.push_back(currentStroke);
            }

            prevX = -1;
            prevY = -1;
            glutPostRedisplay();
        }
    }
}

void display();

// Mouse motion callback
void mouseMotion(int x, int y)
{
    int drawX, drawWidth;
    getDrawingArea(drawX, drawWidth);

    // Check if x is within the drawing area
    if (prevX != -1 && prevY != -1 && x >= drawX && x <= (drawX + drawWidth))
    {

        if (tool == 3 && circleCenterX != -1 && circleCenterY != -1)
        {
            // Draw all existing content
            glClear(GL_COLOR_BUFFER_BIT);
            display();

            // Draw preview circle
            int radius = sqrt(pow(x - circleCenterX, 2) + pow(y - circleCenterY, 2));
            glColor3fv(currentColor);
            glLineWidth(pointSize);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 200; i++)
            {
                float theta = 2.0f * M_PI * i / 200;
                float px = circleCenterX + radius * cos(theta);
                float py = circleCenterY + radius * sin(theta);
                glVertex2f(px, py);
            }
            glEnd();
            glFlush();
        }
        else if (tool == 4 && squareStartX != -1 && squareStartY != -1)
        {
            // Draw all existing content
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
        }
        else if (tool != 3 && tool != 4)
        {
            // Existing pencil/eraser drawing code remains the same
            Line line;
            line.x1 = prevX;
            line.y1 = prevY;
            line.x2 = x;
            line.y2 = y;
            line.isEraser = (tool == 2);
            memcpy(line.color, currentColor, sizeof(float) * 3);
            line.size = pointSize;
            currentStroke.lines.push_back(line);

            if (line.isEraser)
            {
                glColor3f(1.0, 1.0, 1.0);
            }
            else
            {
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

// Add to reshape function
void reshape(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    updateBottomButtonPositions();

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, windowWidth, windowHeight, 0.0);

    glutPostRedisplay();
}
// Display callback
void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw main canvas background
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(windowWidth, 0);
    glVertex2i(windowWidth, windowHeight);
    glVertex2i(0, windowHeight);
    glEnd();

    // Draw all strokes first (canvas content)
    drawStrokes();

    // Draw left sidebar background
    glColor3f(0.09, 0.08, 0.23);
    glBegin(GL_QUADS);
    glVertex2i(sidebarPosition, 0);
    glVertex2i(120 + sidebarPosition, 0);
    glVertex2i(120 + sidebarPosition, windowHeight);
    glVertex2i(sidebarPosition, windowHeight);
    glEnd();

    // Draw right sidebar background
    glColor3f(0.09, 0.08, 0.23);
    glBegin(GL_QUADS);
    glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition, 0);
    glVertex2i(windowWidth + rightSidebarPosition, 0);
    glVertex2i(windowWidth + rightSidebarPosition, windowHeight);
    glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition, windowHeight);
    glEnd();

    // Draw left sidebar UI elements
    if (isSidebarVisible)
    {
        for (int i = 0; i < numButtons - 1; i++)
        {
            Button tempButton = buttons[i];
            tempButton.x += sidebarPosition;
            drawButton(&tempButton);
        }

        std::stringstream ss;
        ss << pointSize;
        glColor3f(1.0, 1.0, 1.0); // Set color to white
        drawBoldText(10 + sidebarPosition, 298, ss.str().c_str(), 0.5);
        drawColorPicker();
    }
    else
    {
        Button tempButton = smallToggleButton;
        tempButton.x = sidebarPosition + 120;
        drawButton(&tempButton);
    }

    // Handle right sidebar content with scrolling
    glEnable(GL_SCISSOR_TEST);
    glScissor(windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition, 0,
              RIGHT_SIDEBAR_WIDTH, windowHeight);

    // Draw board thumbnails
    float yOffset = 50 + scrollOffset; // Changed from 10 to 50
    totalContentHeight = 0;

    for (size_t i = 0; i < boards.size(); i++)
    {
        drawBoardThumbnail(i, windowWidth - RIGHT_SIDEBAR_WIDTH + 10 + rightSidebarPosition, yOffset);
        yOffset += THUMBNAIL_HEIGHT + 30;
        totalContentHeight += THUMBNAIL_HEIGHT + 30;
    }

    // Draw new board button if less than 5 boards
    if (boards.size() < 5)
    {
        glColor3f(1.0, 0.84, 0.77);
        glBegin(GL_QUADS);
        glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + 10 + rightSidebarPosition, yOffset);
        glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + 90 + rightSidebarPosition, yOffset);
        glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + 90 + rightSidebarPosition, yOffset + 25);
        glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + 10 + rightSidebarPosition, yOffset + 25);
        glEnd();

        glColor3f(0.0, 0.0, 0.0);
        drawText(windowWidth - RIGHT_SIDEBAR_WIDTH + 20 + rightSidebarPosition,
                 yOffset + 15, "+ New ");
        totalContentHeight += 35;
    }

    glDisable(GL_SCISSOR_TEST);

    // Draw right sidebar toggle button
    if (!isRightSidebarVisible)
    {
        Button toggleRightButton = {windowWidth - 30, 10, 30, 25, "<<", toggleRightSidebar};
        drawButton(&toggleRightButton);
    }
    else
    {
        Button toggleRightButton =
            {
                windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition + 10,
                10, 100, 25, ">>", toggleRightSidebar};
        drawButton(&toggleRightButton);
    }

    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++)
    {
        drawButton(&bottomButtons[i]);
    }

    drawBottomToolbar();
    glFlush();
}

// Initialize OpenGL settings
void init()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(2.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, windowWidth, windowHeight, 0.0);

    // Initialize with only one board
    createNewBoard();
    isRightSidebarVisible = false;              // Ensure right sidebar starts closed
    rightSidebarPosition = RIGHT_SIDEBAR_WIDTH; // Set initial position to closed state
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("InstantBoard");

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);

    init();
    glutMainLoop();
    return 0;
}