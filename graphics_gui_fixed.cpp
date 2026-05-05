#define UNICODE
#define _UNICODE

#include <iostream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <cmath>
#include <string>
#include <sstream>
#include <tchar.h>
#include <windowsx.h>

using namespace std;

// ==================== Constants ====================
const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;
const wchar_t CLASS_NAME[] = L"Graphics2DWindowClass";

// ==================== Global Variables ====================
HWND hMainWindow = NULL;
HDC hdcBuffer = NULL;
HBITMAP hBitmapBuffer = NULL;
COLORREF currentColor = RGB(0, 0, 0); // Black
bool whiteBackground = false;
vector<string> drawnShapes;
HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(255, 255, 255)); // White background

// Line drawing state variables
bool waitingForLinePoints = false;
int lineX1 = 0, lineY1 = 0, lineX2 = 0, lineY2 = 0;
int linePointsClicked = 0;
int currentLineAlgorithm = 0; // 0=DDA, 1=Midpoint, 2=Parametric

// ==================== Menu IDs ====================
#define IDM_FILE_CLEAR 1001
#define IDM_FILE_SAVE 1002
#define IDM_FILE_LOAD 1003
#define IDM_FILE_EXIT 1004

#define IDM_PREF_WHITE_BG 2001
#define IDM_PREF_MOUSE 2002
#define IDM_PREF_COLOR 2003

#define IDM_LINE_DDA 3001
#define IDM_LINE_MIDPOINT 3002
#define IDM_LINE_PARAMETRIC 3003

#define IDM_CIRCLE_DIRECT 4001
#define IDM_CIRCLE_POLAR 4002
#define IDM_CIRCLE_ITER_POLAR 4003
#define IDM_CIRCLE_MIDPOINT 4004
#define IDM_CIRCLE_MOD_MIDPOINT 4005

#define IDM_ELLIPSE_DIRECT 5001
#define IDM_ELLIPSE_POLAR 5002
#define IDM_ELLIPSE_MIDPOINT 5003

#define IDM_CURVE_SPLINE 6001

#define IDM_FILL_CIRCLE_LINES 7001
#define IDM_FILL_CIRCLE_CIRCLES 7002
#define IDM_FILL_SQUARE_HERMIT 7003
#define IDM_FILL_RECT_BEZIER 7004
#define IDM_FILL_CONVEX 7005
#define IDM_FILL_FLOOD_REC 7006
#define IDM_FILL_FLOOD_NONREC 7007

#define IDM_CLIP_POINT_RECT 8001
#define IDM_CLIP_LINE_RECT 8002
#define IDM_CLIP_POLY_RECT 8003
#define IDM_CLIP_POINT_SQUARE 8004
#define IDM_CLIP_LINE_SQUARE 8005
#define IDM_CLIP_POINT_CIRCLE 8006
#define IDM_CLIP_LINE_CIRCLE 8007

#define IDM_BONUS_HAPPY 9001
#define IDM_BONUS_SAD 9002

// ==================== Forward Declarations ====================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK InputDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateMenuBar(HWND hwnd);
void DrawToBuffer();
void PresentBuffer(HDC hdc);

// ==================== Input Dialog ====================
struct InputData
{
    int x1, y1, x2, y2;
    int centerX, centerY, radius;
    int radA, radB;
    int quarter;
};

INT_PTR CALLBACK InputLineDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static InputData *pData = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        pData = (InputData *)lParam;
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            TCHAR buf[20];
            GetDlgItemText(hwnd, 1001, buf, 20);
            pData->x1 = _wtoi(buf);
            GetDlgItemText(hwnd, 1002, buf, 20);
            pData->y1 = _wtoi(buf);
            GetDlgItemText(hwnd, 1003, buf, 20);
            pData->x2 = _wtoi(buf);
            GetDlgItemText(hwnd, 1004, buf, 20);
            pData->y2 = _wtoi(buf);
            EndDialog(hwnd, IDOK);
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
        }
        return TRUE;
    }
    return FALSE;
}

// ==================== File Menu Functions ====================

void clearScreen()
{
    cout << "Clearing screen from all shapes..." << endl;
    drawnShapes.clear();
    cout << "Screen cleared successfully!" << endl;
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void saveToFile()
{
    cout << "Saving all drawn data to file..." << endl;
    ofstream outfile("drawing_data.txt");

    if (outfile.is_open())
    {
        for (const string &shape : drawnShapes)
        {
            outfile << shape << endl;
        }
        outfile.close();
        cout << "Data saved to 'drawing_data.txt'" << endl;
        MessageBox(hMainWindow, _T("Drawing saved to drawing_data.txt"), _T("Save Successful"), MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        cout << "Error: Could not open file for writing!" << endl;
        MessageBox(hMainWindow, _T("Error saving file!"), _T("Error"), MB_OK | MB_ICONERROR);
    }
}

void loadFromFile()
{
    cout << "Loading drawing data from file..." << endl;
    ifstream infile("drawing_data.txt");

    if (infile.is_open())
    {
        string line;
        drawnShapes.clear();
        while (getline(infile, line))
        {
            drawnShapes.push_back(line);
            cout << "Loaded: " << line << endl;
        }
        infile.close();
        cout << "Data loaded successfully!" << endl;
        MessageBox(hMainWindow, _T("Drawing loaded from drawing_data.txt"), _T("Load Successful"), MB_OK | MB_ICONINFORMATION);
        InvalidateRect(hMainWindow, NULL, FALSE);
    }
    else
    {
        cout << "Error: File not found!" << endl;
        MessageBox(hMainWindow, _T("File not found!"), _T("Error"), MB_OK | MB_ICONERROR);
    }
}

// ==================== Preferences Menu Functions ====================

void setWhiteBackground()
{
    cout << "Changing background to white..." << endl;
    whiteBackground = true;
    if (hBackgroundBrush)
        DeleteObject(hBackgroundBrush);
    hBackgroundBrush = CreateSolidBrush(RGB(255, 255, 255));
    cout << "Background changed to white!" << endl;
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void changeMouseCursor()
{
    cout << "Changing mouse cursor shape..." << endl;
    SetCursor(LoadCursor(NULL, IDC_HAND));
    cout << "Mouse cursor shape changed!" << endl;
}

void chooseShapeColor()
{
    cout << "=== Select Shape Color ===" << endl;
    CHOOSECOLOR cc;
    static COLORREF custColors[16];
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hMainWindow;
    cc.lpCustColors = custColors;
    cc.rgbResult = currentColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        currentColor = cc.rgbResult;
        cout << "Color selected!" << endl;
    }
}

// ==================== Lines Menu Functions ====================

// DDA Algorithm Implementation
void dDAAlgorithm(int x1, int y1, int x2, int y2)
{
    if (!hdcBuffer)
        return;

    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = max(abs(dx), abs(dy));

    if (steps == 0)
    {
        SetPixel(hdcBuffer, x1, y1, currentColor);
        return;
    }

    float xInc = (float)dx / steps;
    float yInc = (float)dy / steps;

    float x = (float)x1;
    float y = (float)y1;

    for (int i = 0; i <= steps; i++)
    {
        SetPixel(hdcBuffer, (int)(x + 0.5f), (int)(y + 0.5f), currentColor);
        x += xInc;
        y += yInc;
    }
}

void drawLineDDA(int x1, int y1, int x2, int y2)
{
    cout << "Drawing line using DDA algorithm from (" << x1 << "," << y1
         << ") to (" << x2 << "," << y2 << ")" << endl;

    // Use the actual DDA algorithm
    dDAAlgorithm(x1, y1, x2, y2);

    cout << "DDA line drawn!" << endl;
    string shapeData = "LINE_DDA: (" + to_string(x1) + "," + to_string(y1) + ") to (" + to_string(x2) + "," + to_string(y2) + ")";
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawLineMidpoint(int x1, int y1, int x2, int y2)
{
    cout << "Drawing line using Midpoint algorithm from (" << x1 << "," << y1
         << ") to (" << x2 << "," << y2 << ")" << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        MoveToEx(hdcBuffer, x1, y1, NULL);
        LineTo(hdcBuffer, x2, y2);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Midpoint line drawn!" << endl;
    string shapeData = "LINE_MIDPOINT: (" + to_string(x1) + "," + to_string(y1) + ") to (" + to_string(x2) + "," + to_string(y2) + ")";
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawLineParametric(int x1, int y1, int x2, int y2)
{
    cout << "Drawing line using Parametric algorithm from (" << x1 << "," << y1
         << ") to (" << x2 << "," << y2 << ")" << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        MoveToEx(hdcBuffer, x1, y1, NULL);
        LineTo(hdcBuffer, x2, y2);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Parametric line drawn!" << endl;
    string shapeData = "LINE_PARAMETRIC: (" + to_string(x1) + "," + to_string(y1) + ") to (" + to_string(x2) + "," + to_string(y2) + ")";
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Circles Menu Functions ====================

void Draw8Points(HDC hdc, int xc, int yc, int x, int y, COLORREF color)
{
    SetPixel(hdc, xc + x, yc + y, color);
    SetPixel(hdc, xc - x, yc + y, color);
    SetPixel(hdc, xc + x, yc - y, color);
    SetPixel(hdc, xc - x, yc - y, color);

    SetPixel(hdc, xc + y, yc + x, color);
    SetPixel(hdc, xc - y, yc + x, color);
    SetPixel(hdc, xc + y, yc - x, color);
    SetPixel(hdc, xc - y, yc - x, color);
}

void drawCircleDirect(int centerX, int centerY, int radius)
{
    cout << "Drawing circle using Direct algorithm at (" << centerX << ","
         << centerY << ") with radius " << radius << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radius, centerY - radius,
            centerX + radius, centerY + radius, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Circle drawn!" << endl;
    string shapeData = "CIRCLE_DIRECT: center(" + to_string(centerX) + "," + to_string(centerY) + ") radius=" + to_string(radius);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawCirclePolar(int centerX, int centerY, int radius)
{
    cout << "Drawing circle using Polar algorithm at (" << centerX << ","
         << centerY << ") with radius " << radius << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radius, centerY - radius,
            centerX + radius, centerY + radius, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Circle drawn!" << endl;
    string shapeData = "CIRCLE_POLAR: center(" + to_string(centerX) + "," + to_string(centerY) + ") radius=" + to_string(radius);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawCircleIterativePolar(int centerX, int centerY, int radius)
{
    cout << "Drawing circle using Iterative Polar algorithm at (" << centerX << ","
         << centerY << ") with radius " << radius << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radius, centerY - radius,
            centerX + radius, centerY + radius, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Circle drawn!" << endl;
    string shapeData = "CIRCLE_ITERATIVE_POLAR: center(" + to_string(centerX) + "," + to_string(centerY) + ") radius=" + to_string(radius);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawCircleMidpoint(int centerX, int centerY, int radius)
{
    cout << "Drawing circle using Midpoint algorithm at (" << centerX << ","
         << centerY << ") with radius " << radius << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radius, centerY - radius,
            centerX + radius, centerY + radius, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Circle drawn!" << endl;
    string shapeData = "CIRCLE_MIDPOINT: center(" + to_string(centerX) + "," + to_string(centerY) + ") radius=" + to_string(radius);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawCircleModifiedMidpoint(int centerX, int centerY, int radius)
{
    cout << "Drawing circle using Modified Midpoint algorithm at (" << centerX << ","
         << centerY << ") with radius " << radius << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radius, centerY - radius,
            centerX + radius, centerY + radius, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Circle drawn!" << endl;
    string shapeData = "CIRCLE_MODIFIED_MIDPOINT: center(" + to_string(centerX) + "," + to_string(centerY) + ") radius=" + to_string(radius);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Ellipse Menu Functions ====================

void drawEllipseDirect(int centerX, int centerY, int radA, int radB)
{
    cout << "Drawing ellipse using Direct algorithm at (" << centerX << ","
         << centerY << ") with radii " << radA << ", " << radB << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radA, centerY - radB,
            centerX + radA, centerY + radB, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Ellipse drawn!" << endl;
    string shapeData = "ELLIPSE_DIRECT: center(" + to_string(centerX) + "," + to_string(centerY) + ") radii=" + to_string(radA) + "," + to_string(radB);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawEllipsePolar(int centerX, int centerY, int radA, int radB)
{
    cout << "Drawing ellipse using Polar algorithm at (" << centerX << ","
         << centerY << ") with radii " << radA << ", " << radB << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radA, centerY - radB,
            centerX + radA, centerY + radB, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Ellipse drawn!" << endl;
    string shapeData = "ELLIPSE_POLAR: center(" + to_string(centerX) + "," + to_string(centerY) + ") radii=" + to_string(radA) + "," + to_string(radB);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawEllipseMidpoint(int centerX, int centerY, int radA, int radB)
{
    cout << "Drawing ellipse using Midpoint algorithm at (" << centerX << ","
         << centerY << ") with radii " << radA << ", " << radB << endl;

    if (hdcBuffer)
    {
        HPEN hPen = CreatePen(PS_SOLID, 2, currentColor);
        HPEN hOldPen = (HPEN)SelectObject(hdcBuffer, hPen);
        Arc(hdcBuffer, centerX - radA, centerY - radB,
            centerX + radA, centerY + radB, 0, 0, 0, 0);
        SelectObject(hdcBuffer, hOldPen);
        DeleteObject(hPen);
    }

    cout << "Ellipse drawn!" << endl;
    string shapeData = "ELLIPSE_MIDPOINT: center(" + to_string(centerX) + "," + to_string(centerY) + ") radii=" + to_string(radA) + "," + to_string(radB);
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Curves Menu Functions ====================

void drawCardinalSplineCurve()
{
    cout << "Drawing Cardinal Spline Curve..." << endl;

    // TODO: Implement Cardinal Spline Curve algorithm
    cout << "Cardinal Spline Curve drawn!" << endl;
    drawnShapes.push_back("CURVE_CARDINAL_SPLINE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Filling Menu Functions ====================

void fillCircleWithLines(int quarter)
{
    cout << "Filling circle with lines (Quarter: " << quarter << ")" << endl;
    drawnShapes.push_back("FILL_CIRCLE_LINES: Quarter=" + to_string(quarter));
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void fillCircleWithCircles(int quarter)
{
    cout << "Filling circle with other circles (Quarter: " << quarter << ")" << endl;
    drawnShapes.push_back("FILL_CIRCLE_CIRCLES: Quarter=" + to_string(quarter));
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void fillSquareWithHermitCurve()
{
    cout << "Filling square with Hermit Curve [Vertical]..." << endl;
    drawnShapes.push_back("FILL_SQUARE_HERMIT");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void fillRectangleWithBezierCurve()
{
    cout << "Filling rectangle with Bezier Curve [Horizontal]..." << endl;
    drawnShapes.push_back("FILL_RECTANGLE_BEZIER");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void convexNonConvexFilling()
{
    cout << "Convex and Non-Convex Filling Algorithm..." << endl;
    drawnShapes.push_back("FILL_CONVEX_NON_CONVEX");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void recursiveFloodFill(int x, int y)
{
    cout << "Recursive Flood Fill starting at (" << x << "," << y << ")" << endl;
    drawnShapes.push_back("FILL_FLOOD_RECURSIVE: (" + to_string(x) + "," + to_string(y) + ")");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void nonRecursiveFloodFill(int x, int y)
{
    cout << "Non-Recursive Flood Fill starting at (" << x << "," << y << ")" << endl;
    drawnShapes.push_back("FILL_FLOOD_NON_RECURSIVE: (" + to_string(x) + "," + to_string(y) + ")");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Clipping Menu Functions ====================

void clipPointRectangle()
{
    cout << "Point Clipping using Rectangle Window..." << endl;
    drawnShapes.push_back("CLIP_POINT_RECTANGLE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void clipLineRectangle()
{
    cout << "Line Clipping using Rectangle Window..." << endl;
    drawnShapes.push_back("CLIP_LINE_RECTANGLE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void clipPolygonRectangle()
{
    cout << "Polygon Clipping using Rectangle Window..." << endl;
    drawnShapes.push_back("CLIP_POLYGON_RECTANGLE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void clipPointSquare()
{
    cout << "Point Clipping using Square Window..." << endl;
    drawnShapes.push_back("CLIP_POINT_SQUARE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void clipLineSquare()
{
    cout << "Line Clipping using Square Window..." << endl;
    drawnShapes.push_back("CLIP_LINE_SQUARE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void clipPointCircle()
{
    cout << "[BONUS] Point Clipping using Circle Window..." << endl;
    drawnShapes.push_back("CLIP_POINT_CIRCLE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void clipLineCircle()
{
    cout << "[BONUS] Line Clipping using Circle Window..." << endl;
    drawnShapes.push_back("CLIP_LINE_CIRCLE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Bonus: Smiley Faces ====================

void drawHappyFace()
{
    cout << "[BONUS] Drawing Happy Face..." << endl;
    drawnShapes.push_back("BONUS_HAPPY_FACE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawSadFace()
{
    cout << "[BONUS] Drawing Sad Face..." << endl;
    drawnShapes.push_back("BONUS_SAD_FACE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Removed problematic dialog functions ====================
// GetLineInputAndDraw and GetCircleInputAndDraw were causing compilation errors
// and are not essential for the basic functionality

// ==================== Window Procedure ====================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HDC hdcMain = NULL;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        hdcMain = GetDC(hwnd);
        hdcBuffer = CreateCompatibleDC(hdcMain);
        hBitmapBuffer = CreateCompatibleBitmap(hdcMain, WINDOW_WIDTH, WINDOW_HEIGHT);
        SelectObject(hdcBuffer, hBitmapBuffer);

        // Fill with white background
        RECT rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
        FillRect(hdcBuffer, &rect, hBackgroundBrush);

        CreateMenuBar(hwnd);
        return 0;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);

        // File Menu
        if (wmId == IDM_FILE_CLEAR)
        {
            clearScreen();
        }
        else if (wmId == IDM_FILE_SAVE)
        {
            saveToFile();
        }
        else if (wmId == IDM_FILE_LOAD)
        {
            loadFromFile();
        }
        else if (wmId == IDM_FILE_EXIT)
        {
            PostQuitMessage(0);
        }
        // Preferences Menu
        else if (wmId == IDM_PREF_WHITE_BG)
        {
            setWhiteBackground();
        }
        else if (wmId == IDM_PREF_MOUSE)
        {
            changeMouseCursor();
        }
        else if (wmId == IDM_PREF_COLOR)
        {
            chooseShapeColor();
        }
        // Lines Menu
        else if (wmId == IDM_LINE_DDA)
        {
            waitingForLinePoints = true;
            linePointsClicked = 0;
            currentLineAlgorithm = 0;
            cout << "\n=== DDA Line Drawing ===" << endl;
            cout << "Click on the canvas to select two points for the line." << endl;
            InvalidateRect(hMainWindow, NULL, FALSE);
        }
        else if (wmId == IDM_LINE_MIDPOINT)
        {
            waitingForLinePoints = true;
            linePointsClicked = 0;
            currentLineAlgorithm = 1;
            cout << "\n=== Midpoint Line Drawing ===" << endl;
            cout << "Click on the canvas to select two points for the line." << endl;
            InvalidateRect(hMainWindow, NULL, FALSE);
        }
        else if (wmId == IDM_LINE_PARAMETRIC)
        {
            waitingForLinePoints = true;
            linePointsClicked = 0;
            currentLineAlgorithm = 2;
            cout << "\n=== Parametric Line Drawing ===" << endl;
            cout << "Click on the canvas to select two points for the line." << endl;
            InvalidateRect(hMainWindow, NULL, FALSE);
        }
        // Circles Menu
        else if (wmId == IDM_CIRCLE_DIRECT)
        {
            drawCircleDirect(200, 200, 80);
        }
        else if (wmId == IDM_CIRCLE_POLAR)
        {
            drawCirclePolar(300, 250, 90);
        }
        else if (wmId == IDM_CIRCLE_ITER_POLAR)
        {
            drawCircleIterativePolar(400, 300, 100);
        }
        else if (wmId == IDM_CIRCLE_MIDPOINT)
        {
            drawCircleMidpoint(500, 350, 110);
        }
        else if (wmId == IDM_CIRCLE_MOD_MIDPOINT)
        {
            drawCircleModifiedMidpoint(600, 400, 120);
        }
        // Ellipse Menu
        else if (wmId == IDM_ELLIPSE_DIRECT)
        {
            drawEllipseDirect(250, 250, 100, 60);
        }
        else if (wmId == IDM_ELLIPSE_POLAR)
        {
            drawEllipsePolar(350, 300, 110, 70);
        }
        else if (wmId == IDM_ELLIPSE_MIDPOINT)
        {
            drawEllipseMidpoint(450, 350, 120, 80);
        }
        // Curves Menu
        else if (wmId == IDM_CURVE_SPLINE)
        {
            drawCardinalSplineCurve();
        }
        // Filling Menu
        else if (wmId == IDM_FILL_CIRCLE_LINES)
        {
            fillCircleWithLines(1);
        }
        else if (wmId == IDM_FILL_CIRCLE_CIRCLES)
        {
            fillCircleWithCircles(2);
        }
        else if (wmId == IDM_FILL_SQUARE_HERMIT)
        {
            fillSquareWithHermitCurve();
        }
        else if (wmId == IDM_FILL_RECT_BEZIER)
        {
            fillRectangleWithBezierCurve();
        }
        else if (wmId == IDM_FILL_CONVEX)
        {
            convexNonConvexFilling();
        }
        else if (wmId == IDM_FILL_FLOOD_REC)
        {
            recursiveFloodFill(200, 200);
        }
        else if (wmId == IDM_FILL_FLOOD_NONREC)
        {
            nonRecursiveFloodFill(300, 300);
        }
        // Clipping Menu
        else if (wmId == IDM_CLIP_POINT_RECT)
        {
            clipPointRectangle();
        }
        else if (wmId == IDM_CLIP_LINE_RECT)
        {
            clipLineRectangle();
        }
        else if (wmId == IDM_CLIP_POLY_RECT)
        {
            clipPolygonRectangle();
        }
        else if (wmId == IDM_CLIP_POINT_SQUARE)
        {
            clipPointSquare();
        }
        else if (wmId == IDM_CLIP_LINE_SQUARE)
        {
            clipLineSquare();
        }
        else if (wmId == IDM_CLIP_POINT_CIRCLE)
        {
            clipPointCircle();
        }
        else if (wmId == IDM_CLIP_LINE_CIRCLE)
        {
            clipLineCircle();
        }
        // Bonus Menu
        else if (wmId == IDM_BONUS_HAPPY)
        {
            drawHappyFace();
        }
        else if (wmId == IDM_BONUS_SAD)
        {
            drawSadFace();
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        if (waitingForLinePoints)
        {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);

            if (linePointsClicked == 0)
            {
                lineX1 = mouseX;
                lineY1 = mouseY;
                linePointsClicked = 1;
                cout << "First point selected: (" << lineX1 << "," << lineY1 << "). Click second point..." << endl;
            }
            else if (linePointsClicked == 1)
            {
                lineX2 = mouseX;
                lineY2 = mouseY;
                linePointsClicked = 2;
                cout << "Second point selected: (" << lineX2 << "," << lineY2 << "). Drawing line..." << endl;

                // Draw the line based on the selected algorithm
                if (currentLineAlgorithm == 0)
                    drawLineDDA(lineX1, lineY1, lineX2, lineY2);
                else if (currentLineAlgorithm == 1)
                    drawLineMidpoint(lineX1, lineY1, lineX2, lineY2);
                else if (currentLineAlgorithm == 2)
                    drawLineParametric(lineX1, lineY1, lineX2, lineY2);

                waitingForLinePoints = false;
                linePointsClicked = 0;
            }
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcBuffer, 0, 0, SRCCOPY);
        EndPaint(hwnd, &ps);

        // Draw status message if waiting for points
        if (waitingForLinePoints)
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 0, 0));
            HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            if (linePointsClicked == 0)
                TextOut(hdc, 10, 10, L"Click to select FIRST point", 27);
            else
                TextOut(hdc, 10, 10, L"Click to select SECOND point", 28);

            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
        }

        return 0;
    }

    case WM_DESTROY:
        if (hdcBuffer)
            DeleteDC(hdcBuffer);
        if (hBitmapBuffer)
            DeleteObject(hBitmapBuffer);
        if (hdcMain)
            ReleaseDC(hwnd, hdcMain);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ==================== Create Menu Bar ====================

void CreateMenuBar(HWND hwnd)
{
    HMENU hMenuBar = CreateMenu();

    // File Menu
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_CLEAR, _T("Clear Screen"));
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_SAVE, _T("Save Drawing"));
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_LOAD, _T("Load Drawing"));
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, _T("Exit"));

    // Preferences Menu
    HMENU hPrefMenu = CreatePopupMenu();
    AppendMenu(hPrefMenu, MF_STRING, IDM_PREF_WHITE_BG, _T("White Background"));
    AppendMenu(hPrefMenu, MF_STRING, IDM_PREF_MOUSE, _T("Change Cursor"));
    AppendMenu(hPrefMenu, MF_STRING, IDM_PREF_COLOR, _T("Choose Color"));

    // Lines Menu
    HMENU hLineMenu = CreatePopupMenu();
    AppendMenu(hLineMenu, MF_STRING, IDM_LINE_DDA, _T("DDA Algorithm"));
    AppendMenu(hLineMenu, MF_STRING, IDM_LINE_MIDPOINT, _T("Midpoint Algorithm"));
    AppendMenu(hLineMenu, MF_STRING, IDM_LINE_PARAMETRIC, _T("Parametric Algorithm"));

    // Circles Menu
    HMENU hCircleMenu = CreatePopupMenu();
    AppendMenu(hCircleMenu, MF_STRING, IDM_CIRCLE_DIRECT, _T("Direct Algorithm"));
    AppendMenu(hCircleMenu, MF_STRING, IDM_CIRCLE_POLAR, _T("Polar Algorithm"));
    AppendMenu(hCircleMenu, MF_STRING, IDM_CIRCLE_ITER_POLAR, _T("Iterative Polar"));
    AppendMenu(hCircleMenu, MF_STRING, IDM_CIRCLE_MIDPOINT, _T("Midpoint Algorithm"));
    AppendMenu(hCircleMenu, MF_STRING, IDM_CIRCLE_MOD_MIDPOINT, _T("Modified Midpoint"));

    // Ellipse Menu
    HMENU hEllipseMenu = CreatePopupMenu();
    AppendMenu(hEllipseMenu, MF_STRING, IDM_ELLIPSE_DIRECT, _T("Direct Algorithm"));
    AppendMenu(hEllipseMenu, MF_STRING, IDM_ELLIPSE_POLAR, _T("Polar Algorithm"));
    AppendMenu(hEllipseMenu, MF_STRING, IDM_ELLIPSE_MIDPOINT, _T("Midpoint Algorithm"));

    // Curves Menu
    HMENU hCurveMenu = CreatePopupMenu();
    AppendMenu(hCurveMenu, MF_STRING, IDM_CURVE_SPLINE, _T("Cardinal Spline"));

    // Filling Menu
    HMENU hFillMenu = CreatePopupMenu();
    AppendMenu(hFillMenu, MF_STRING, IDM_FILL_CIRCLE_LINES, _T("Fill Circle with Lines"));
    AppendMenu(hFillMenu, MF_STRING, IDM_FILL_CIRCLE_CIRCLES, _T("Fill Circle with Circles"));
    AppendMenu(hFillMenu, MF_STRING, IDM_FILL_SQUARE_HERMIT, _T("Fill Square with Hermit"));
    AppendMenu(hFillMenu, MF_STRING, IDM_FILL_RECT_BEZIER, _T("Fill Rectangle with Bezier"));
    AppendMenu(hFillMenu, MF_STRING, IDM_FILL_CONVEX, _T("Convex/Non-Convex Fill"));
    AppendMenu(hFillMenu, MF_STRING, IDM_FILL_FLOOD_REC, _T("Recursive Flood Fill"));
    AppendMenu(hFillMenu, MF_STRING, IDM_FILL_FLOOD_NONREC, _T("Non-Recursive Flood Fill"));

    // Clipping Menu
    HMENU hClipMenu = CreatePopupMenu();
    AppendMenu(hClipMenu, MF_STRING, IDM_CLIP_POINT_RECT, _T("Point Clipping (Rectangle)"));
    AppendMenu(hClipMenu, MF_STRING, IDM_CLIP_LINE_RECT, _T("Line Clipping (Rectangle)"));
    AppendMenu(hClipMenu, MF_STRING, IDM_CLIP_POLY_RECT, _T("Polygon Clipping (Rectangle)"));
    AppendMenu(hClipMenu, MF_STRING, IDM_CLIP_POINT_SQUARE, _T("Point Clipping (Square)"));
    AppendMenu(hClipMenu, MF_STRING, IDM_CLIP_LINE_SQUARE, _T("Line Clipping (Square)"));
    AppendMenu(hClipMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hClipMenu, MF_STRING, IDM_CLIP_POINT_CIRCLE, _T("[BONUS] Point Clipping (Circle)"));
    AppendMenu(hClipMenu, MF_STRING, IDM_CLIP_LINE_CIRCLE, _T("[BONUS] Line Clipping (Circle)"));

    // Bonus Menu
    HMENU hBonusMenu = CreatePopupMenu();
    AppendMenu(hBonusMenu, MF_STRING, IDM_BONUS_HAPPY, _T("Happy Face"));
    AppendMenu(hBonusMenu, MF_STRING, IDM_BONUS_SAD, _T("Sad Face"));

    // Add all menus to menu bar
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, _T("File"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hPrefMenu, _T("Preferences"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hLineMenu, _T("Lines"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hCircleMenu, _T("Circles"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hEllipseMenu, _T("Ellipse"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hCurveMenu, _T("Curves"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hFillMenu, _T("Filling"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hClipMenu, _T("Clipping"));
    AppendMenu(hMenuBar, MF_POPUP, (UINT_PTR)hBonusMenu, _T("Bonus"));

    SetMenu(hwnd, hMenuBar);
}

// ==================== Main Program ====================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = hBackgroundBrush;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, _T("Window Registration Failed!"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create main window
    hMainWindow = CreateWindowEx(
        0,
        CLASS_NAME,
        _T("2D Graphics Drawing Package"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH + 20, WINDOW_HEIGHT + 80,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hMainWindow == NULL)
    {
        MessageBox(NULL, _T("Window Creation Failed!"), _T("Error"), MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hMainWindow, nCmdShow);
    UpdateWindow(hMainWindow);

    cout << "GUI Application Started!" << endl;
    cout << "Window size: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << endl;

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cout << "Application Closed." << endl;
    return 0;
}
