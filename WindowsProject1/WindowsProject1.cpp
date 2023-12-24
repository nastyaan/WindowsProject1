#include <windows.h>
#include <commdlg.h>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <locale>
#include <codecvt>

using namespace std;

// Глобальные переменные
HBITMAP hBitmap = NULL;

// Определение идентификатора меню
#define IDM_OPEN 1001

// Структура для хранения информации об изображении
struct ImageInfo {
    std::vector<COLORREF>* pixels;
    int width;
    int height;
};

// Функция открытия BMP файла через диалоговое окно
bool OpenBMPFile(HWND hWnd, wchar_t** selectedFile)
{
    OPENFILENAME ofn;
    wchar_t fileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"BMP Files\0*.bmp\0";
    ofn.lpstrFile = fileName;
    ofn.lpstrTitle = L"Выберите BMP файл";
    ofn.nMaxFile = sizeof(fileName);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        *selectedFile = _wcsdup(fileName);
        return true;
    }

    return false;
}

// Функция для чтения пиксельных цветов из файла
ImageInfo* ReadPixelColorsFromFile(const wchar_t* filename) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<COLORREF>* pixelColors = new std::vector<COLORREF>();

    if (file.is_open()) {
        char* header = new char[54];
        file.read(header, 54);

        int width = *reinterpret_cast<int*>(&header[18]);
        int height = *reinterpret_cast<int*>(&header[22]);

        int dataSize = width * height * 3;

        char* data = new char[dataSize];

        file.read(data, dataSize);

        COLORREF backgroundColor = RGB((unsigned char)data[2], (unsigned char)data[1], (unsigned char)data[0]);

        for (int i = 0; i < dataSize; i += 3) {
            COLORREF color = RGB((unsigned char)data[i + 2], (unsigned char)data[i + 1], (unsigned char)data[i]);
            pixelColors->push_back(color);
        }

        delete[] data;
        delete[] header;

        file.close();

        // Замена цвета фона на белый
        for (COLORREF& color : *pixelColors) {
            if (color == backgroundColor) {
                color = RGB(255, 255, 255);
            }
        }

        ImageInfo* info = static_cast<ImageInfo*>(malloc(sizeof(ImageInfo)));
        info->pixels = pixelColors;
        info->height = height;
        info->width = width;

        return info;
    }

    return nullptr;
}

// Функция отображения BMP изображения
void DrawImage(const ImageInfo* imageInfo, HDC hdcWindow, HWND hwnd) {
    int imageWidth = imageInfo->width;
    int imageHeight = imageInfo->height;
    std::vector<COLORREF>* pixelColors = imageInfo->pixels;

    RECT windowRect;
    GetClientRect(hwnd, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    // Закрашиваем окно белыми пикселями
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdcWindow, &windowRect, whiteBrush);
    DeleteObject(whiteBrush);

    for (int x = 0; x < imageWidth; ++x) {
        for (int y = 0; y < imageHeight; ++y) {
            SetPixel(hdcWindow, imageWidth * 0.2 - x + windowWidth / 1.05, y + windowHeight / -3.5, pixelColors->at(pixelColors->size() - (1 + imageWidth * y + x)));
        }
    }
}





// Функция обработки сообщений
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static wchar_t* selectedFile = nullptr; // Объявление переменной selectedFile
    static ImageInfo* imageInfo = nullptr; // Объявление переменной imageInfo

    switch (msg) {
    case WM_COMMAND:
        // Обработка сообщений меню
        switch (LOWORD(wParam)) {
        case IDM_OPEN:
            // Открытие BMP файла
            if (OpenBMPFile(hwnd, &selectedFile)) {
                imageInfo = ReadPixelColorsFromFile(selectedFile);
                if (imageInfo) {
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            break;
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        int windowWidth = LOWORD(lParam);
        int windowHeight = HIWORD(lParam);
        PaintRgn(hdc, CreateRectRgn(0, 0, windowWidth, windowHeight));
        if (imageInfo != nullptr) {
            DrawImage(imageInfo, hdc, hwnd);
        }

        EndPaint(hwnd, &ps);

        return 0;
    } break;

    case WM_SIZE:
        // Обновление окна при изменении размера
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_CLOSE:
        // Закрытие окна
        DeleteObject(hBitmap);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}


// Функция создания и отображения окна
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Регистрация класса окна
    const wchar_t CLASS_NAME[] = L"MyWindowClass";

    WNDCLASS wndClass = { };
    wndClass.lpfnWndProc = WndProc;
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = CLASS_NAME;

    RegisterClass(&wndClass);

    // Создание окна
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Пример приложения Windows с отображением BMP",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Создание меню
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, IDM_OPEN, L"Открыть BMP файл");
    AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, L"Файл");

    // Установка меню для окна
    SetMenu(hwnd, hMenu);

    // Отображение окна
    ShowWindow(hwnd, nCmdShow);

    // Цикл обработки сообщений
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}