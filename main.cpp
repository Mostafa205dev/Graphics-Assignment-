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
#include <stack>
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

// Circle drawing state variables
bool waitingForCirclePoints = false;
int circleCenterX, circleCenterY;
int circlePointsClicked = 0;
int currentCircleAlgorithm = 0; // 0=Direct, 1=Polar, 2=Iterative Polar, 3=Midpoint, 4=Modified Midpoint

// fill cricles with lines
// bool waitingForFillCircleWithLines = false;

// flood fill state variable
bool waitingForFloodFill = false;
int currentFloodFillAlgorithm; // 0=Recursive, 1=Non-Recursive
// Ellipse drawing state variables
bool waitingForEllipsePoints = false;
int ellipseCenterX, ellipseCenterY;
int ellipseRadiusAX, ellipseRadiusAY;
int ellipseRadiusBX, ellipseRadiusBY;
int ellipsePointsClicked = 0;
int currentEllipseAlgorithm = 0; // 0=Direct, 1=Polar, 2=Midpoint

// Cursor state variable
int currentCursorType = 0; // 0=Arrow, 1=Hand, 2=Wait, 3=Cross, 4=IBeam, 5=No, 6=SizeNS, 7=SizeWE
LPCTSTR cursorTypeNames[] = {_T("Arrow"), _T("Hand"), _T("Wait (Hourglass)"), _T("Cross"), _T("IBeam"), _T("No/Prohibited"), _T("Size NS"), _T("Size WE")};

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

// Cursor selection dialog
INT_PTR CALLBACK CursorSelectionDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static int *pCursorType = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        pCursorType = (int *)lParam;

        // Create combo box with cursor options
        HWND hCombo = GetDlgItem(hwnd, 1001);
        for (int i = 0; i < 8; i++)
        {
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)cursorTypeNames[i]);
        }
        SendMessage(hCombo, CB_SETCURSEL, *pCursorType, 0);
        return TRUE;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDOK)
        {
            HWND hCombo = GetDlgItem(hwnd, 1001);
            *pCursorType = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            EndDialog(hwnd, IDOK);
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwnd, IDCANCEL);
        }
        return TRUE;
    }
    }
    return FALSE;
}

// ==================== File Menu Functions ====================
// The parseAndDrawShape function is used to interpret the shape data loaded from the file
void parseAndDrawShape(const string &shapeData);

void clearScreen()
{
    cout << "Clearing screen from all shapes..." << endl;

    drawnShapes.clear();

    if (hdcBuffer)
    {
        RECT rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};

        FillRect(hdcBuffer, &rect, hBackgroundBrush);
    }

    cout << "Screen cleared successfully!" << endl;

    InvalidateRect(hMainWindow, NULL, FALSE);
    UpdateWindow(hMainWindow);
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

        // Clear the canvas first
        if (hdcBuffer)
        {
            RECT rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            FillRect(hdcBuffer, &rect, hBackgroundBrush);
        }

        // Load and redraw all shapes
        while (getline(infile, line))
        {
            if (!line.empty())
            {
                drawnShapes.push_back(line);
                cout << "Loaded: " << line << endl;

                // Parse and redraw the shape
                parseAndDrawShape(line);
            }
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

LPCWSTR GetCursorHandleByType(int cursorType)
{
    switch (cursorType)
    {
    case 0:
        return IDC_ARROW;
    case 1:
        return IDC_HAND;
    case 2:
        return IDC_WAIT;
    case 3:
        return IDC_CROSS;
    case 4:
        return IDC_IBEAM;
    case 5:
        return IDC_NO;
    case 6:
        return IDC_SIZENS;
    case 7:
        return IDC_SIZEWE;
    default:
        return IDC_ARROW;
    }
}

void changeMouseCursor()
{
    cout << "=== Select Mouse Cursor ===" << endl;
    cout << "Available cursor options:" << endl;
    cout << "0: Arrow" << endl;
    cout << "1: Hand (Pointer)" << endl;
    cout << "2: Wait (Hourglass)" << endl;
    cout << "3: Cross" << endl;
    cout << "4: IBeam (Text)" << endl;
    cout << "5: No/Prohibited" << endl;
    cout << "6: Size North-South" << endl;
    cout << "7: Size West-East" << endl;

    // Show information about available cursors
    int result = MessageBox(hMainWindow,
                            _T("Select a mouse cursor:\n\n")
                            _T("Click 'Yes' to cycle through cursor types:\n")
                            _T("Arrow -> Hand -> Wait -> Cross -> IBeam -> No -> SizeNS -> SizeWE -> Arrow...\n\n")
                            _T("Current cursor will change each time you click Yes"),
                            _T("Change Mouse Cursor"),
                            MB_YESNO | MB_ICONQUESTION);

    if (result == IDYES)
    {
        // Cycle through cursors
        currentCursorType = (currentCursorType + 1) % 8;
        SetCursor(LoadCursor(NULL, GetCursorHandleByType(currentCursorType)));
        cout << "Mouse cursor changed to type: " << currentCursorType << endl;
        cout << "Cursor: " << cursorTypeNames[currentCursorType] << endl;
    }
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

// Midpoint Algorithm Implementation
void midpointAlgorithm(int x1, int y1, int x2, int y2)
{
    if (!hdcBuffer)
        return;

    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int x = x1;
    int y = y1;

    if (dx > dy) // More horizontal
    {
        int d = 2 * dy - dx;
        int d_inc = 2 * dy;
        int d_dec = 2 * (dy - dx);

        SetPixel(hdcBuffer, x, y, currentColor);

        for (int i = 0; i < dx; i++)
        {
            if (d < 0)
            {
                d += d_inc;
            }
            else
            {
                d += d_dec;
                y += sy;
            }
            x += sx;
            SetPixel(hdcBuffer, x, y, currentColor);
        }
    }
    else // More vertical
    {
        int d = 2 * dx - dy;
        int d_inc = 2 * dx;
        int d_dec = 2 * (dx - dy);

        SetPixel(hdcBuffer, x, y, currentColor);

        for (int i = 0; i < dy; i++)
        {
            if (d < 0)
            {
                d += d_inc;
            }
            else
            {
                d += d_dec;
                x += sx;
            }
            y += sy;
            SetPixel(hdcBuffer, x, y, currentColor);
        }
    }
}

void drawLineMidpoint(int x1, int y1, int x2, int y2)
{
    cout << "Drawing line using Midpoint algorithm from (" << x1 << "," << y1
         << ") to (" << x2 << "," << y2 << ")" << endl;

    // Use the actual Midpoint algorithm
    midpointAlgorithm(x1, y1, x2, y2);

    cout << "Midpoint line drawn!" << endl;
    string shapeData = "LINE_MIDPOINT: (" + to_string(x1) + "," + to_string(y1) + ") to (" + to_string(x2) + "," + to_string(y2) + ")";
    drawnShapes.push_back(shapeData);
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// Parametric Algorithm Implementation
void parametricAlgorithm(int x1, int y1, int x2, int y2)
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

    for (int i = 0; i <= steps; i++)
    {
        float t = (float)i / steps;
        float x = x1 + t * dx;
        float y = y1 + t * dy;

        SetPixel(hdcBuffer, (int)(x + 0.5f), (int)(y + 0.5f), currentColor);
    }
}

void drawLineParametric(int x1, int y1, int x2, int y2)
{
    cout << "Drawing line using Parametric algorithm from (" << x1 << "," << y1
         << ") to (" << x2 << "," << y2 << ")" << endl;

    // Use the actual Parametric algorithm
    parametricAlgorithm(x1, y1, x2, y2);

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
    cout << "Drawing circle using Direct algorithm at ("
         << centerX << "," << centerY
         << ") radius = " << radius << endl;

    if (!hdcBuffer)
        return;

    int x = 0;
    int y = radius;

    Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);

    while (x < y)
    {
        x++;

        y = round(sqrt((double)radius * radius - x * x));

        Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);
    }

    cout << "Circle drawn!" << endl;

    string shapeData =
        "CIRCLE_DIRECT: center(" +
        to_string(centerX) + "," +
        to_string(centerY) +
        ") radius=" + to_string(radius);

    drawnShapes.push_back(shapeData);

    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawCirclePolar(int centerX, int centerY, int radius)
{
    cout << "Drawing circle using Polar algorithm at (" << centerX << ","
         << centerY << ") with radius " << radius << endl;

    if (!hdcBuffer)
        return;

    int x = radius, y = 0;
    double theta = 0, dtheta = 1.0 / radius;
    Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);
    while (x > y)
    {
        theta += dtheta;
        x = round(radius * cos(theta));
        y = round(radius * sin(theta));
        Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);
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

    if (!hdcBuffer)
        return;

    double x = radius, y = 0;
    double dtheta = 1.0 / radius;
    double cdtheta = cos(dtheta), sdtheta = sin(dtheta);
    Draw8Points(hdcBuffer, centerX, centerY, radius, 0, currentColor);
    while (x > y)
    {
        double x1 = x * cdtheta - y * sdtheta;
        y = x * sdtheta + y * cdtheta;
        x = x1;
        Draw8Points(hdcBuffer, centerX, centerY, round(x), round(y), currentColor);
    }

    cout << "Circle drawn!" << endl;
    string shapeData = "CIRCLE_ITER_POLAR: center(" + to_string(centerX) + "," + to_string(centerY) + ") radius=" + to_string(radius);
    drawnShapes.push_back(shapeData);

    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawCircleMidpoint(int centerX, int centerY, int radius)
{
    cout << "Drawing circle using Midpoint algorithm at (" << centerX << ","
         << centerY << ") with radius " << radius << endl;

    if (!hdcBuffer)
        return;

    int x = 0, y = radius;
    int d = 1 - radius;
    Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);
    while (x < y)
    {
        if (d < 0)
        {
            d += 2 * x + 3;
        }
        else
        {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
        Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);
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

    if (!hdcBuffer)
        return;

    int x = 0, y = radius;
    int d = 1 - radius;
    int c1 = 3, c2 = 5 - 2 * radius;
    Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);
    while (x < y)
    {
        if (d < 0)
        {
            d += c1;
            c2 += 2;
        }
        else
        {
            d += c2;
            c2 += 4;
            y--;
        }
        c1 += 2;
        x++;
        Draw8Points(hdcBuffer, centerX, centerY, x, y, currentColor);
    }

    cout << "Circle drawn!" << endl;
    string shapeData = "CIRCLE_MOD_MIDPOINT: center(" + to_string(centerX) + "," + to_string(centerY) + ") radius=" + to_string(radius);
    drawnShapes.push_back(shapeData);

    InvalidateRect(hMainWindow, NULL, FALSE);
}

// ==================== Ellipse Menu Functions ====================

// Helper function to draw 4 quadrant points for ellipse

void Draw4EllipsePoints(HDC hdc, int xc, int yc, int x, int y, COLORREF color)
{
    SetPixel(hdc, xc + x, yc + y, color);
    SetPixel(hdc, xc - x, yc + y, color);
    SetPixel(hdc, xc + x, yc - y, color);
    SetPixel(hdc, xc - x, yc - y, color);
}

void drawEllipseDirect(int centerX, int centerY, int radA, int radB)
{
    cout << "Drawing ellipse using Direct algorithm at (" << centerX << ","
         << centerY << ") with radii " << radA << ", " << radB << endl;

    if (!hdcBuffer)
        return;

    double a2 = (double)radA * radA;
    double b2 = (double)radB * radB;
    cout << "centerx and centerY are " << centerX << " and " << centerY << endl;
    float x = 0;
    float y = radB;

    Draw4EllipsePoints(hdcBuffer, centerX, centerY, x, y, currentColor);

    while (x < radA)
    {
        x += .25;
        y = round(sqrt(b2 * (1.0 - (x * x) / a2)));
        Draw4EllipsePoints(hdcBuffer, centerX, centerY, x, y, currentColor);
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

    if (!hdcBuffer)
        return;

    double theta = 0.0;
    double dtheta = 1.0 / max(radA, radB);
    int x = radA, y = 0;

    Draw4EllipsePoints(hdcBuffer, centerX, centerY, x, y, currentColor);

    while (theta < M_PI / 2.0)
    {
        theta += dtheta;
        x = round(radA * cos(theta));
        y = round(radB * sin(theta));
        Draw4EllipsePoints(hdcBuffer, centerX, centerY, x, y, currentColor);
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

    if (!hdcBuffer)
        return;

    int a = radA;
    int b = radB;
    int a2 = a * a;
    int b2 = b * b;
    int x = 0, y = b;
    double d = b2 - a2 * b + a2 / 4;

    Draw4EllipsePoints(hdcBuffer, centerX, centerY, x, y, currentColor);

    while (b2 * x <= a2 * y)
    {
        if (d < 0)
        {
            d += b2 * (2 * x + 3);
        }
        else
        {
            d += b2 * (2 * x + 3) - a2 * (2 * y - 2);
            y--;
        }
        x++;
        Draw4EllipsePoints(hdcBuffer, centerX, centerY, x, y, currentColor);
    }

    d = b2 * (x + 0.5) * (x + 0.5) + a2 * (y - 1) * (y - 1) - a2 * b2;
    while (y > 0)
    {
        if (d < 0)
        {
            d += b2 * (2 * x + 2) + a2 * (-2 * y + 3);
            x++;
        }
        else
        {
            d += a2 * (-2 * y + 3);
        }
        y--;
        Draw4EllipsePoints(hdcBuffer, centerX, centerY, x, y, currentColor);
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
bool getLastCircle(int &cx, int &cy, int &r)
{
    for (int i = (int)drawnShapes.size() - 1; i >= 0; i--)
    {
        string s = drawnShapes[i];

        if (s.find("CIRCLE_") == 0)
        {
            if (sscanf_s(s.c_str(),
                "%*[^:]: center(%d,%d) radius=%d",
                &cx, &cy, &r) == 3)
            {
                return true;
            }
        }
    }
    return false;
}
void fillCircleWithLinesQuarter(int xc, int yc, int r, int quarter)
{
    if (!hdcBuffer)
        return;

    double startAngle = 0, endAngle = 0;

    switch (quarter)
    {
    case 1:
        startAngle = 0;
        endAngle = 90;
        break;
    case 2:
        startAngle = 90;
        endAngle = 180;
        break;
    case 3:
        startAngle = 180;
        endAngle = 270;
        break;
    case 4:
        startAngle = 270;
        endAngle = 360;
        break;
    }

    for (double angle = startAngle; angle <= endAngle; angle += 1.0)
    {
        double rad = angle * 3.14159265 / 180.0;

        int x = xc + (int)(r * cos(rad));
        int y = yc + (int)(r * sin(rad));

        MoveToEx(hdcBuffer, xc, yc, NULL);
        LineTo(hdcBuffer, x, y);
    }

    drawnShapes.push_back("FILL_CIRCLE_LINES_QUARTER");

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

void floodFillRec(int x, int y, COLORREF oldColor, COLORREF newColor)
{
    if (!hdcBuffer)
        return;

    // boundary checks
    if (x < 0 || x >= WINDOW_WIDTH - 1 || y < 0 || y >= WINDOW_HEIGHT - 1)
        return;

    COLORREF currentPixel = GetPixel(hdcBuffer, x, y);

    // stop conditions
    if (currentPixel != oldColor || currentPixel == newColor)
        return;

    // fill current pixel
    SetPixel(hdcBuffer, x, y, newColor);

    // recursive calls
    floodFillRec(x + 1, y, oldColor, newColor);
    floodFillRec(x - 1, y, oldColor, newColor);
    floodFillRec(x, y + 1, oldColor, newColor);
    floodFillRec(x, y - 1, oldColor, newColor);
}

void recursiveFloodFill(int x, int y)
{
    cout << "Recursive Flood Fill starting at (" << x << "," << y << ")" << endl;

    COLORREF oldColor = GetPixel(hdcBuffer, x, y);
    COLORREF newColor = currentColor;

    if (oldColor == newColor)
        return;

    floodFillRec(x, y, oldColor, newColor);

    drawnShapes.push_back("FILL_FLOOD_RECURSIVE: (" + to_string(x) + "," + to_string(y) + ")");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

struct Point
{
    int x, y;
};

void nonRecursiveFloodFill(int x, int y)
{
    cout << "Non-Recursive Flood Fill starting at (" << x << "," << y << ")" << endl;

    COLORREF oldColor = GetPixel(hdcBuffer, x, y);
    COLORREF newColor = currentColor;

    if (oldColor == newColor)
        return;

    stack<Point> S;
    S.push({x, y});

    while (!S.empty())
    {
        Point p = S.top();
        S.pop();

        // boundary check
        if (p.x < 0 || p.x >= WINDOW_WIDTH || p.y < 0 || p.y >= WINDOW_HEIGHT)
            continue;

        COLORREF currentPixel = GetPixel(hdcBuffer, p.x, p.y);

        // stop condition
        if (currentPixel != oldColor || currentPixel == newColor)
            continue;

        // fill
        SetPixel(hdcBuffer, p.x, p.y, newColor);

        // neighbors
        S.push({p.x + 1, p.y});
        S.push({p.x - 1, p.y});
        S.push({p.x, p.y + 1});
        S.push({p.x, p.y - 1});
    }

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

    if (!hdcBuffer)
        return;

    int centerX = WINDOW_WIDTH / 2;
    int centerY = WINDOW_HEIGHT / 2;
    int headRadius = 100;
    int eyeRadius = 8;
    int mouthRadius = 40;

    // Draw head
    drawCircleMidpoint(centerX, centerY, headRadius);

    // Draw left eye
    drawCircleMidpoint(centerX - 30, centerY - 30, eyeRadius);

    // Draw right eye
    drawCircleMidpoint(centerX + 30, centerY - 30, eyeRadius);

    // Draw smile (curved line using small circles)
    for (int i = -35; i <= 35; i++)
    {
        int y = (int)(mouthRadius - sqrt(mouthRadius * mouthRadius - i * i) + centerY);
        SetPixel(hdcBuffer, centerX + i, y, currentColor);
    }

    drawnShapes.push_back("BONUS_HAPPY_FACE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

void drawSadFace()
{
    cout << "[BONUS] Drawing Sad Face..." << endl;

    if (!hdcBuffer)
        return;

    int centerX = WINDOW_WIDTH / 2;
    int centerY = WINDOW_HEIGHT / 2;
    int headRadius = 100;
    int eyeRadius = 8;
    int mouthRadius = 40;

    // Draw head
    drawCircleMidpoint(centerX, centerY, headRadius);

    // Draw left eye
    drawCircleMidpoint(centerX - 30, centerY - 30, eyeRadius);

    // Draw right eye
    drawCircleMidpoint(centerX + 30, centerY - 30, eyeRadius);

    // Draw frown (curved line using small circles - inverted from smile)
    for (int i = -35; i <= 35; i++)
    {
        int y = (int)(centerY + 50 - (mouthRadius - sqrt(mouthRadius * mouthRadius - i * i)));
        SetPixel(hdcBuffer, centerX + i, y, currentColor);
    }

    drawnShapes.push_back("BONUS_SAD_FACE");
    InvalidateRect(hMainWindow, NULL, FALSE);
}

// =============== Helper function to parse and redraw shapes from stored data ==================
void parseAndDrawShape(const string &shapeData)
{
    // Parse LINE_DDA
    if (shapeData.find("LINE_DDA:") == 0)
    {
        int x1, y1, x2, y2;
        sscanf_s(shapeData.c_str(), "LINE_DDA: (%d,%d) to (%d,%d)", &x1, &y1, &x2, &y2);
        dDAAlgorithm(x1, y1, x2, y2);
    }
    // Parse LINE_MIDPOINT
    else if (shapeData.find("LINE_MIDPOINT:") == 0)
    {
        int x1, y1, x2, y2;
        sscanf_s(shapeData.c_str(), "LINE_MIDPOINT: (%d,%d) to (%d,%d)", &x1, &y1, &x2, &y2);
        midpointAlgorithm(x1, y1, x2, y2);
    }
    // Parse LINE_PARAMETRIC
    else if (shapeData.find("LINE_PARAMETRIC:") == 0)
    {
        int x1, y1, x2, y2;
        sscanf_s(shapeData.c_str(), "LINE_PARAMETRIC: (%d,%d) to (%d,%d)", &x1, &y1, &x2, &y2);
        parametricAlgorithm(x1, y1, x2, y2);
    }
    // Parse CIRCLE_DIRECT
    else if (shapeData.find("CIRCLE_DIRECT:") == 0)
    {
        int cx, cy, radius;
        sscanf_s(shapeData.c_str(), "CIRCLE_DIRECT: center(%d,%d) radius=%d", &cx, &cy, &radius);
        drawCircleDirect(cx, cy, radius);
    }
    // Parse CIRCLE_POLAR
    else if (shapeData.find("CIRCLE_POLAR:") == 0)
    {
        int cx, cy, radius;
        sscanf_s(shapeData.c_str(), "CIRCLE_POLAR: center(%d,%d) radius=%d", &cx, &cy, &radius);
        drawCirclePolar(cx, cy, radius);
    }
    // Parse CIRCLE_ITER_POLAR
    else if (shapeData.find("CIRCLE_ITER_POLAR:") == 0)
    {
        int cx, cy, radius;
        sscanf_s(shapeData.c_str(), "CIRCLE_ITER_POLAR: center(%d,%d) radius=%d", &cx, &cy, &radius);
        drawCircleIterativePolar(cx, cy, radius);
    }
    // Parse CIRCLE_MIDPOINT
    else if (shapeData.find("CIRCLE_MIDPOINT:") == 0)
    {
        int cx, cy, radius;
        sscanf_s(shapeData.c_str(), "CIRCLE_MIDPOINT: center(%d,%d) radius=%d", &cx, &cy, &radius);
        drawCircleMidpoint(cx, cy, radius);
    }
    // Parse CIRCLE_MOD_MIDPOINT
    else if (shapeData.find("CIRCLE_MOD_MIDPOINT:") == 0)
    {
        int cx, cy, radius;
        sscanf_s(shapeData.c_str(), "CIRCLE_MOD_MIDPOINT: center(%d,%d) radius=%d", &cx, &cy, &radius);
        drawCircleModifiedMidpoint(cx, cy, radius);
    }
    // Parse ELLIPSE_DIRECT
    else if (shapeData.find("ELLIPSE_DIRECT:") == 0)
    {
        int cx, cy, radA, radB;
        sscanf_s(shapeData.c_str(), "ELLIPSE_DIRECT: center(%d,%d) radii=%d,%d", &cx, &cy, &radA, &radB);
        drawEllipseDirect(cx, cy, radA, radB);
    }
    // Parse ELLIPSE_POLAR
    else if (shapeData.find("ELLIPSE_POLAR:") == 0)
    {
        int cx, cy, radA, radB;
        sscanf_s(shapeData.c_str(), "ELLIPSE_POLAR: center(%d,%d) radii=%d,%d", &cx, &cy, &radA, &radB);
        drawEllipsePolar(cx, cy, radA, radB);
    }
    // Parse ELLIPSE_MIDPOINT
    else if (shapeData.find("ELLIPSE_MIDPOINT:") == 0)
    {
        int cx, cy, radA, radB;
        sscanf_s(shapeData.c_str(), "ELLIPSE_MIDPOINT: center(%d,%d) radii=%d,%d", &cx, &cy, &radA, &radB);
        drawEllipseMidpoint(cx, cy, radA, radB);
    }
    // Add more shape types here as needed in the future
}

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

            waitingForCirclePoints = true;
            circlePointsClicked = 0;
            currentCircleAlgorithm = 0;

            cout << "\n=== Direct Circle Drawing ===" << endl;
            cout << "Click center of circle, then click radius point." << endl;
        }
        else if (wmId == IDM_CIRCLE_POLAR)
        {
            waitingForCirclePoints = true;
            circlePointsClicked = 0;
            currentCircleAlgorithm = 1;

            cout << "\n=== Polar Circle Drawing ===" << endl;
            cout << "Click center of circle, then click radius point." << endl;
        }
        else if (wmId == IDM_CIRCLE_ITER_POLAR)
        {
            waitingForCirclePoints = true;
            circlePointsClicked = 0;
            currentCircleAlgorithm = 2;

            cout << "\n=== Iterative Polar Circle Drawing ===" << endl;
            cout << "Click center of circle, then click radius point." << endl;
        }
        else if (wmId == IDM_CIRCLE_MIDPOINT)
        {
            waitingForCirclePoints = true;
            circlePointsClicked = 0;
            currentCircleAlgorithm = 3;

            cout << "\n=== Midpoint Circle Drawing ===" << endl;
            cout << "Click center of circle, then click radius point." << endl;
        }
        else if (wmId == IDM_CIRCLE_MOD_MIDPOINT)
        {
            waitingForCirclePoints = true;
            circlePointsClicked = 0;
            currentCircleAlgorithm = 4;

            cout << "\n=== Modified Midpoint Circle Drawing ===" << endl;
            cout << "Click center of circle, then click radius point." << endl;
        }
        // Ellipse Menu
        else if (wmId == IDM_ELLIPSE_DIRECT)
        {
            waitingForEllipsePoints = true;
            ellipsePointsClicked = 0;
            currentEllipseAlgorithm = 0;

            cout << "\n=== Direct Ellipse Drawing ===" << endl;
            cout << "Click center of ellipse, then click X-radius point, then click Y-radius point." << endl;
        }
        else if (wmId == IDM_ELLIPSE_POLAR)
        {
            waitingForEllipsePoints = true;
            ellipsePointsClicked = 0;
            currentEllipseAlgorithm = 1;

            cout << "\n=== Polar Ellipse Drawing ===" << endl;
            cout << "Click center of ellipse, then click X-radius point, then click Y-radius point." << endl;
        }
        else if (wmId == IDM_ELLIPSE_MIDPOINT)
        {
            waitingForEllipsePoints = true;
            ellipsePointsClicked = 0;
            currentEllipseAlgorithm = 2;

            cout << "\n=== Midpoint Ellipse Drawing ===" << endl;
            cout << "Click center of ellipse, then click X-radius point, then click Y-radius point." << endl;
        }
        // Curves Menu
        else if (wmId == IDM_CURVE_SPLINE)
        {
            drawCardinalSplineCurve();
        }
        // Filling Menu
        else if (wmId == IDM_FILL_CIRCLE_LINES)
        {
            int cx, cy, r;

            if (getLastCircle(cx, cy, r))
            {
                cout << "Filling last drawn circle with lines..." << endl;

                fillCircleWithLinesQuarter(cx, cy, r, 1);

                InvalidateRect(hMainWindow, NULL, FALSE);
            }
            else
            {
                MessageBox(hMainWindow,
                           _T("No circle found! Draw a circle first."),
                           _T("Error"),
                           MB_OK | MB_ICONERROR);
            }
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
            currentFloodFillAlgorithm = 0;
            waitingForFloodFill = true;
            cout << "\n=== Recursive Flood Fill ===" << endl;
            cout << "Click on the canvas to select a point for flood fill." << endl;
        }
        else if (wmId == IDM_FILL_FLOOD_NONREC)
        {
            currentFloodFillAlgorithm = 1;
            waitingForFloodFill = true;
            cout << "\n=== Non-Recursive Flood Fill ===" << endl;
            cout << "Click on the canvas to select a point for flood fill." << endl;
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
        if (waitingForCirclePoints)
        {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);

            if (circlePointsClicked == 0)
            {
                circleCenterX = mouseX;
                circleCenterY = mouseY;
                circlePointsClicked = 1;

                cout << "Center selected: ("
                     << circleCenterX << ","
                     << circleCenterY << ")" << endl;
                cout << "Click radius point..." << endl;
            }
            else if (circlePointsClicked == 1)
            {
                int dx = mouseX - circleCenterX;
                int dy = mouseY - circleCenterY;

                int radius = round(sqrt(dx * dx + dy * dy));

                if (currentCircleAlgorithm == 0)
                    drawCircleDirect(circleCenterX, circleCenterY, radius);
                else if (currentCircleAlgorithm == 1)
                    drawCirclePolar(circleCenterX, circleCenterY, radius);
                else if (currentCircleAlgorithm == 2)
                    drawCircleIterativePolar(circleCenterX, circleCenterY, radius);
                else if (currentCircleAlgorithm == 3)
                    drawCircleMidpoint(circleCenterX, circleCenterY, radius);
                else if (currentCircleAlgorithm == 4)
                    drawCircleModifiedMidpoint(circleCenterX, circleCenterY, radius);

                waitingForCirclePoints = false;
                circlePointsClicked = 0;
            }

            return 0;
        }
        // if (waitingForFillCircleWithLines)
        // {
        //     int mouseX = GET_X_LPARAM(lParam);
        //     int mouseY = GET_Y_LPARAM(lParam);

        //     int radius = 100;

        //     fillCircleWithLinesQuarter(mouseX, mouseY, radius, 1);

        //     waitingForFillCircleWithLines = false;
        //     return 0;
        // }
        if (waitingForFloodFill)
        {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);

            if (currentFloodFillAlgorithm == 0)
            {
                recursiveFloodFill(mouseX, mouseY);
            }
            else if (currentFloodFillAlgorithm == 1)
            {
                nonRecursiveFloodFill(mouseX, mouseY);
            }

            waitingForFloodFill = false;
            return 0;
        }
        if (waitingForEllipsePoints)
        {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);

            if (ellipsePointsClicked == 0)
            {
                ellipseCenterX = mouseX;
                ellipseCenterY = mouseY;
                ellipsePointsClicked = 1;

                cout << "Center selected: ("
                     << ellipseCenterX << ","
                     << ellipseCenterY << ")" << endl;
                cout << "Click X-radius point..." << endl;
            }
            else if (ellipsePointsClicked == 1)
            {
                ellipseRadiusAX = mouseX;
                ellipseRadiusAY = mouseY;
                ellipsePointsClicked = 2;

                int radA = abs(ellipseRadiusAX - ellipseCenterX);
                cout << "X-radius point selected. Horizontal radius: " << radA << endl;
                cout << "Click Y-radius point..." << endl;
            }
            else if (ellipsePointsClicked == 2)
            {
                ellipseRadiusBX = mouseX;
                ellipseRadiusBY = mouseY;
                ellipsePointsClicked = 3;

                int radA = abs(ellipseRadiusAX - ellipseCenterX);
                int radB = abs(ellipseRadiusBY - ellipseCenterY);

                cout << "Y-radius point selected. Radii: A=" << radA << ", B=" << radB << endl;
                cout << "Drawing ellipse..." << endl;

                // Draw the ellipse based on the selected algorithm
                if (currentEllipseAlgorithm == 0)
                    drawEllipseDirect(ellipseCenterX, ellipseCenterY, radA, radB);
                else if (currentEllipseAlgorithm == 1)
                    drawEllipsePolar(ellipseCenterX, ellipseCenterY, radA, radB);
                else if (currentEllipseAlgorithm == 2)
                    drawEllipseMidpoint(ellipseCenterX, ellipseCenterY, radA, radB);

                waitingForEllipsePoints = false;
                ellipsePointsClicked = 0;
            }
            return 0;
        }
        break;
    }

    case WM_SETCURSOR:
    {
        // Apply the selected cursor type
        SetCursor(LoadCursor(NULL, GetCursorHandleByType(currentCursorType)));
        return TRUE;
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
        if (waitingForEllipsePoints)
        {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 255));
            HFONT hFont = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Arial");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            if (ellipsePointsClicked == 0)
                TextOut(hdc, 10, 10, L"Click to select CENTER point", 28);
            else if (ellipsePointsClicked == 1)
                TextOut(hdc, 10, 10, L"Click to select X-RADIUS point", 30);
            else if (ellipsePointsClicked == 2)
                TextOut(hdc, 10, 10, L"Click to select Y-RADIUS point", 30);

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
