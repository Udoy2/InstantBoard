#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <sstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <math.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

#define _WIN32_WINNT 0x0601 // Windows 7 or later
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void toggleSidebar();
void animateSidebar();
void animateRightSidebar();
void toggleRightSidebar();

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
const int BOTTOM_BUTTON_SIZE = 40;

const int RIGHT_SIDEBAR_WIDTH = 180;
const int THUMBNAIL_HEIGHT = 100;
const int THUMBNAIL_WIDTH = 180;

float scrollOffset = 0.0f;
const float SCROLL_SPEED = 20.0f;
int totalContentHeight = 0;
int selectedBottomTool = 1;

int prevX = -1, prevY = -1;
int circleCenterX = -1, circleCenterY = -1;
int windowWidth = 800, windowHeight = 600;
int tool = 1;
float currentColor[3] = {0.0, 0.0, 0.0};
int pointSize = 2;
int squareStartX = -1, squareStartY = -1;

bool isSidebarVisible = true;
float sidebarPosition = 0.0f;

bool isRightSidebarVisible = false;
float rightSidebarPosition = RIGHT_SIDEBAR_WIDTH;

SOCKET hostSocket = INVALID_SOCKET;
SOCKET clientSocket = INVALID_SOCKET;
bool isHost = false;
bool isClient = false;

std::mutex strokesMutex; // Mutex for synchronizing access to strokes

struct Line
{
    int x1, y1, x2, y2;
    float color[3];
    int size;
    bool isEraser;
};

struct Stroke
{
    std::vector<Line> lines;
    float color[3];
    int size;
    bool isEraser;
};
void sendStroke(const Stroke &stroke);
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
std::vector<Stroke> strokes;
Stroke currentStroke;

typedef struct
{
    int x, y, w, h;
    const char *label;
    void (*callbackFunction)();
} Button;

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

static bool canAdjustSize = true;
const int COOLDOWN_MS = 200;

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
    if (boards.size() == 1)
    {
        clearScreen();
        return;
    }

    std::vector<Board> tempBoards;
    for (int i = 0; i < static_cast<int>(boards.size()); i++) // Cast to int
    {
        if (i != currentBoardIndex)
        {
            tempBoards.push_back(boards[i]);
        }
    }

    boards = tempBoards;
    currentBoardIndex = std::max(0, currentBoardIndex - 1);
    strokes = boards[currentBoardIndex].strokes;
    pointSize = boards[currentBoardIndex].pointSize;
    memcpy(currentColor, boards[currentBoardIndex].currentColor, sizeof(float) * 3);
    tool = boards[currentBoardIndex].tool;
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
        {windowWidth - RIGHT_SIDEBAR_WIDTH + 10, 10, 100, 25, "<", toggleRightSidebar}};
int numButtons = 10;

Button bottomButtons[] =
    {
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", setPencilTool},
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", setEraserTool},
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", clearScreen},
        {0, 0, BOTTOM_BUTTON_SIZE, BOTTOM_BUTTON_SIZE, "", deleteCurrentBoard}};
const int NUM_BOTTOM_BUTTONS = 4;

void drawRotatedClearButtonIcon(Button *b)
{
    glPushMatrix();
    glTranslatef(b->x + 20, b->y + 20, 0);
    glRotatef(45.0f, 0, 0, 1);
    glTranslatef(-(b->x + 20), -(b->y + 20), 0);

    glBegin(GL_TRIANGLES);
    glVertex2i(b->x + 10, b->y + 30);
    glVertex2i(b->x + 30, b->y + 30);
    glVertex2i(b->x + 20, b->y + 10);
    glEnd();

    glBegin(GL_LINES);
    glVertex2i(b->x + 16, b->y + 10);
    glVertex2i(b->x + 24, b->y + 10);
    glEnd();

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_LINES);
    glVertex2i(b->x + 14, b->y + 20);
    glVertex2i(b->x + 18, b->y + 20);
    glVertex2i(b->x + 22, b->y + 20);
    glVertex2i(b->x + 26, b->y + 20);
    glEnd();

    glPopMatrix();
}

void drawIcon(Button *b, int iconType)
{
    glColor3f(0.0, 0.0, 0.0);
    glLineWidth(2.0);

    switch (iconType)
    {
    case 0:
        glBegin(GL_QUADS);
        glVertex2i(b->x + 16, b->y + 12);
        glVertex2i(b->x + 24, b->y + 12);
        glVertex2i(b->x + 24, b->y + 28);
        glVertex2i(b->x + 16, b->y + 28);
        glEnd();

        glBegin(GL_TRIANGLES);
        glVertex2i(b->x + 16, b->y + 12);
        glVertex2i(b->x + 20, b->y + 4);
        glVertex2i(b->x + 24, b->y + 12);
        glEnd();
        break;

    case 1:
        glBegin(GL_QUADS);
        glVertex2i(b->x + 12, b->y + 15);
        glVertex2i(b->x + 28, b->y + 15);
        glVertex2i(b->x + 28, b->y + 25);
        glVertex2i(b->x + 12, b->y + 25);
        glEnd();

        glBegin(GL_POLYGON);
        for (int i = 0; i <= 360; i++)
        {
            float theta = i * 3.14159 / 180;
            glVertex2f(b->x + 10 + 2 * cos(theta), b->y + 20 + 5 * sin(theta));
        }
        glEnd();
        break;

    case 2:
        drawRotatedClearButtonIcon(b);
        break;

    case 3:
        glBegin(GL_LINE_STRIP);
        glVertex2i(b->x + 12, b->y + 15);
        glVertex2i(b->x + 28, b->y + 15);
        glVertex2i(b->x + 26, b->y + 30);
        glVertex2i(b->x + 14, b->y + 30);
        glVertex2i(b->x + 12, b->y + 15);
        glEnd();

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

void drawBottomToolbar()
{
    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++)
    {
        bool isSelected = false;

        if (i == 0)
            isSelected = (tool == 1);
        if (i == 1)
            isSelected = (tool == 2);

        if (isSelected)
        {
            glColor3f(1.0, 0.7, 0.6);
        }
        else
        {
            glColor3f(1.0, 0.84, 0.77);
        }

        glBegin(GL_QUADS);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y + bottomButtons[i].h);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y + bottomButtons[i].h);
        glEnd();

        glLineWidth(isSelected ? 2.0 : 1.0);
        glColor3f(0.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y);
        glVertex2i(bottomButtons[i].x + bottomButtons[i].w, bottomButtons[i].y + bottomButtons[i].h);
        glVertex2i(bottomButtons[i].x, bottomButtons[i].y + bottomButtons[i].h);
        glEnd();
        glLineWidth(1.0);

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

    if (!boards.empty())
    {
        boards[currentBoardIndex].strokes = strokes;
        boards[currentBoardIndex].pointSize = pointSize;
        memcpy(boards[currentBoardIndex].currentColor, currentColor, sizeof(float) * 3);
        boards[currentBoardIndex].tool = tool;
    }

    Board newBoard;
    newBoard.name = "Board " + toString(boards.size() + 1) + "/5";
    newBoard.pointSize = 2;
    newBoard.tool = 1;
    newBoard.strokes.clear();
    memcpy(newBoard.currentColor, currentColor, sizeof(float) * 3);

    boards.push_back(newBoard);
    currentBoardIndex = boards.size() - 1;
    strokes.clear();
    glutPostRedisplay();
}

void switchToBoard(int index)
{
    if (index >= 0 && index < static_cast<int>(boards.size())) // Cast to int
    {
        if (!boards.empty())
        {
            boards[currentBoardIndex].strokes = strokes;
            boards[currentBoardIndex].pointSize = pointSize;
            memcpy(boards[currentBoardIndex].currentColor, currentColor, sizeof(float) * 3);
            boards[currentBoardIndex].tool = tool;
        }

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
    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + THUMBNAIL_WIDTH - 20, y);
    glVertex2i(x + THUMBNAIL_WIDTH - 20, y + THUMBNAIL_HEIGHT);
    glVertex2i(x, y + THUMBNAIL_HEIGHT);
    glEnd();

    if (index == currentBoardIndex)
    {
        glColor3f(0.0, 0.7, 1.0);
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

    glColor3f(1.0, 1.0, 1.0);
    std::string boardName = "Board " + toString(index + 1) + "/5";
    drawText(x + 5, y + THUMBNAIL_HEIGHT + 15, boardName.c_str());
}

void handleBoardClick(int x, int y)
{
    float yOffset = 50 + scrollOffset;

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

    if (boards.size() < 5)
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
        sidebarPosition += (targetPosition - sidebarPosition) * 0.02f;
        glutPostRedisplay();
    }
    else
    {
        sidebarPosition = targetPosition;
        glutIdleFunc(NULL);
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
        rightSidebarPosition += (targetPosition - rightSidebarPosition) * 0.2f;
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

void drawButton(Button *b)
{
    glColor3f(1.0, 0.84, 0.77);
    glBegin(GL_QUADS);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    GLfloat currentWidth;
    glGetFloatv(GL_LINE_WIDTH, &currentWidth);

    glLineWidth(1.0);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2i(b->x, b->y);
    glVertex2i(b->x + b->w, b->y);
    glVertex2i(b->x + b->w, b->y + b->h);
    glVertex2i(b->x, b->y + b->h);
    glEnd();

    glLineWidth(currentWidth);
    glColor3f(0.0, 0.0, 0.0);
    drawText(b->x + 10, b->y + 15, b->label);
}

bool isInsideCircle(int x, int y, int centerX, int centerY, int radius)
{
    int dx = x - centerX;
    int dy = y - centerY;
    return (dx * dx + dy * dy) <= (radius * radius);
}

void HSVtoRGB(float h, float s, float v, float &r, float &g, float &b)
{
    if (s == 0)
    {
        r = g = b = v;
        return;
    }

    h = h * 6.0f;
    int i = floor(h);
    float f = h - i;
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

void drawColorPicker()
{
    int centerX = 60;
    int centerY = windowHeight - 150;
    int radius = 50;
    int segments = 32;

    glBegin(GL_TRIANGLE_FAN);
    glColor3f(1.0, 1.0, 1.0);
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

    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * M_PI * i / segments;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);
        glVertex2f(x, y);
    }
    glEnd();

    glBegin(GL_QUADS);
    glColor3fv(currentColor);
    glVertex2i(10, windowHeight - 80);
    glVertex2i(110, windowHeight - 80);
    glVertex2i(110, windowHeight - 50);
    glVertex2i(10, windowHeight - 50);
    glEnd();
}

void drawStrokes()
{
    std::lock_guard<std::mutex> lock(strokesMutex); // Lock the strokes vector
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

    if (!isRightSidebarVisible)
    {
        if (x >= windowWidth - 40 && x <= windowWidth - 10 &&
            y >= 10 && y <= 35)
        {
            toggleRightSidebar();
            return;
        }
    }
    else
    {
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
        float dx = x - centerX;
        float dy = y - centerY;
        float angle = atan2(dy, dx);
        if (angle < 0)
            angle += 2 * M_PI;

        float distance = sqrt(dx * dx + dy * dy);
        float saturation = std::min(distance / radius, 1.0f);

        float hue = angle / (2 * M_PI);

        HSVtoRGB(hue, saturation, 1.0f,
                 currentColor[0],
                 currentColor[1],
                 currentColor[2]);
        glutPostRedisplay();
    }
}

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
        if (direction > 0)
        {
            scrollOffset += SCROLL_SPEED;
            if (scrollOffset > 0)
                scrollOffset = 0;
        }
        else
        {
            float maxScroll = -(totalContentHeight - windowHeight + 20);
            scrollOffset -= SCROLL_SPEED;
            if (scrollOffset < maxScroll)
                scrollOffset = maxScroll;
        }
        glutPostRedisplay();
    }
}

void getDrawingArea(int &drawX, int &drawWidth)
{
    drawX = 0;
    drawWidth = windowWidth;

    if (isSidebarVisible == true)
    {
        drawX += 120;
        drawWidth -= 120;
    }

    if (isRightSidebarVisible == true)
    {
        drawWidth -= RIGHT_SIDEBAR_WIDTH;
    }
}

void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        handleButtonClick(x, y);
    }
    if (state == GLUT_DOWN)
    {
        if (isRightSidebarVisible)
        {
            if (x >= windowWidth - RIGHT_SIDEBAR_WIDTH)
            {
                if (button == 3)
                {
                    scrollOffset += SCROLL_SPEED;
                    if (scrollOffset > 0)
                        scrollOffset = 0;
                    glutPostRedisplay();
                    return;
                }
                else if (button == 4)
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

        if (isSidebarVisible)
        {
            if (x < 120)
            {
                handleButtonClick(x, y);
                handleColorPickerClick(x, y);
                return;
            }
        }

        int drawX, drawWidth;
        getDrawingArea(drawX, drawWidth);
        if (x >= drawX && x <= (drawX + drawWidth))
        {
            prevX = x;
            prevY = y;

            switch (tool)
            {
            case 3:
                circleCenterX = x;
                circleCenterY = y;
                break;
            case 4:
                squareStartX = x;
                squareStartY = y;
                break;
            default:
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
                int radius = sqrt(pow(x - circleCenterX, 2) + pow(y - circleCenterY, 2));
                Stroke circleStroke;
                circleStroke.isEraser = false;
                memcpy(circleStroke.color, currentColor, sizeof(float) * 3);
                circleStroke.size = pointSize;

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

                // Send the stroke to the other user
                sendStroke(circleStroke);
            }
            else if (tool == 4 && squareStartX != -1 && squareStartY != -1)
            {
                Stroke squareStroke;
                squareStroke.isEraser = false;
                memcpy(squareStroke.color, currentColor, sizeof(float) * 3);
                squareStroke.size = pointSize;

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

                // Send the stroke to the other user
                sendStroke(squareStroke);
            }
            else if (tool != 3 && tool != 4)
            {
                strokes.push_back(currentStroke);

                // Send the stroke to the other user
                sendStroke(currentStroke);
            }

            prevX = -1;
            prevY = -1;
            glutPostRedisplay();
        }
    }
}

void display();

void mouseMotion(int x, int y)
{
    int drawX, drawWidth;
    getDrawingArea(drawX, drawWidth);

    if (prevX != -1 && prevY != -1 && x >= drawX && x <= (drawX + drawWidth))
    {

        if (tool == 3 && circleCenterX != -1 && circleCenterY != -1)
        {
            glClear(GL_COLOR_BUFFER_BIT);
            display();

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
            glClear(GL_COLOR_BUFFER_BIT);
            display();

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

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(windowWidth, 0);
    glVertex2i(windowWidth, windowHeight);
    glVertex2i(0, windowHeight);
    glEnd();

    drawStrokes();

    glColor3f(0.09, 0.08, 0.23);
    glBegin(GL_QUADS);
    glVertex2i(sidebarPosition, 0);
    glVertex2i(120 + sidebarPosition, 0);
    glVertex2i(120 + sidebarPosition, windowHeight);
    glVertex2i(sidebarPosition, windowHeight);
    glEnd();

    glColor3f(0.09, 0.08, 0.23);
    glBegin(GL_QUADS);
    glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition, 0);
    glVertex2i(windowWidth + rightSidebarPosition, 0);
    glVertex2i(windowWidth + rightSidebarPosition, windowHeight);
    glVertex2i(windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition, windowHeight);
    glEnd();

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
        glColor3f(1.0, 1.0, 1.0);
        drawBoldText(10 + sidebarPosition, 298, ss.str().c_str(), 0.5);
        drawColorPicker();
    }
    else
    {
        Button tempButton = smallToggleButton;
        tempButton.x = sidebarPosition + 120;
        drawButton(&tempButton);
    }

    glEnable(GL_SCISSOR_TEST);
    glScissor(windowWidth - RIGHT_SIDEBAR_WIDTH + rightSidebarPosition, 0,
              RIGHT_SIDEBAR_WIDTH, windowHeight);

    float yOffset = 50 + scrollOffset;
    totalContentHeight = 0;

    for (size_t i = 0; i < boards.size(); i++)
    {
        drawBoardThumbnail(i, windowWidth - RIGHT_SIDEBAR_WIDTH + 10 + rightSidebarPosition, yOffset);
        yOffset += THUMBNAIL_HEIGHT + 30;
        totalContentHeight += THUMBNAIL_HEIGHT + 30;
    }

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

void init()
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(2.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, windowWidth, windowHeight, 0.0);

    createNewBoard();
    isRightSidebarVisible = false;
    rightSidebarPosition = RIGHT_SIDEBAR_WIDTH;
}

void cleanup()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
    }
    if (hostSocket != INVALID_SOCKET)
    {
        closesocket(hostSocket);
    }
    WSACleanup();
}

void startHost()
{
    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "27015", &hints, &result) != 0)
    {
        std::cerr << "getaddrinfo failed.\n";
        WSACleanup();
        return;
    }

    hostSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (hostSocket == INVALID_SOCKET)
    {
        std::cerr << "Error at socket(): " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        WSACleanup();
        return;
    }

    if (bind(hostSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
    {
        std::cerr << "bind failed with error: " << WSAGetLastError() << "\n";
        freeaddrinfo(result);
        closesocket(hostSocket);
        WSACleanup();
        return;
    }

    freeaddrinfo(result);

    if (listen(hostSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(hostSocket);
        WSACleanup();
        return;
    }

    clientSocket = accept(hostSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "accept failed: " << WSAGetLastError() << "\n";
        closesocket(hostSocket);
        WSACleanup();
        return;
    }

    // Set the socket to non-blocking mode
    u_long mode = 1; // 1 to enable non-blocking socket
    ioctlsocket(clientSocket, FIONBIO, &mode);

    isHost = true;
    std::cout << "Client connected.\n";
}

void connectToHost(const char *hostname)
{
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, "27015", &hints, &result) != 0)
    {
        std::cerr << "getaddrinfo failed.\n";
        WSACleanup();
        return;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
    {
        clientSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (clientSocket == INVALID_SOCKET)
        {
            std::cerr << "Error at socket(): " << WSAGetLastError() << "\n";
            WSACleanup();
            return;
        }

        if (connect(clientSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
        {
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Unable to connect to server!\n";
        WSACleanup();
        return;
    }

    // Set the socket to non-blocking mode
    u_long mode = 1; // 1 to enable non-blocking socket
    ioctlsocket(clientSocket, FIONBIO, &mode);

    isClient = true;
    std::cout << "Connected to host.\n";
}

void sendData(const std::string &data)
{
    if (isHost || isClient)
    {
        int iResult = send(clientSocket, data.c_str(), data.length(), 0);
        if (iResult == SOCKET_ERROR)
        {
            std::cerr << "send failed: " << WSAGetLastError() << "\n";
            closesocket(clientSocket);
            WSACleanup();
        }
    }
}

std::string receiveData()
{
    char recvbuf[512];
    int iResult = recv(clientSocket, recvbuf, 512, 0);
    if (iResult > 0)
    {
        recvbuf[iResult] = '\0';
        return std::string(recvbuf);
    }
    else if (iResult == 0)
    {
        std::cout << "Connection closed\n";
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    else
    {
        if (WSAGetLastError() != WSAEWOULDBLOCK)
        {
            std::cerr << "recv failed: " << WSAGetLastError() << "\n";
            closesocket(clientSocket);
            clientSocket = INVALID_SOCKET;
        }
    }
    return "";
}

void sendStroke(const Stroke &stroke)
{
    std::stringstream ss;
    ss << stroke.isEraser << " " << stroke.size << " " << stroke.color[0] << " " << stroke.color[1] << " " << stroke.color[2] << " ";
    for (size_t i = 0; i < stroke.lines.size(); i++)
    {
        const Line &line = stroke.lines[i];
        ss << line.x1 << " " << line.y1 << " " << line.x2 << " " << line.y2 << " ";
    }
    ss << "\n"; // Add a delimiter to mark the end of the stroke
    sendData(ss.str());
}

void receiveStrokes()
{
    std::string data = receiveData();
    if (!data.empty())
    {
        std::stringstream ss(data);
        Stroke stroke;
        ss >> stroke.isEraser >> stroke.size >> stroke.color[0] >> stroke.color[1] >> stroke.color[2];
        while (ss)
        {
            Line line;
            ss >> line.x1 >> line.y1 >> line.x2 >> line.y2;
            if (ss)
            {
                line.isEraser = stroke.isEraser;
                line.size = stroke.size;
                memcpy(line.color, stroke.color, sizeof(float) * 3);
                stroke.lines.push_back(line);
            }
        }
        std::lock_guard<std::mutex> lock(strokesMutex); // Lock the strokes vector
        strokes.push_back(stroke);
        glutPostRedisplay();
    }
}

void networkThread()
{
    while (true)
    {
        receiveStrokes();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("InstantBoard");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "-host") == 0)
    {
        startHost();
    }
    else if (argc > 1 && strcmp(argv[1], "-connect") == 0)
    {
        connectToHost(argv[2]);
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);

    // Start the network thread
    std::thread t(networkThread);
    t.detach(); // Detach the thread to run independently

    init();
    glutMainLoop();

    cleanup();
    return 0;
}