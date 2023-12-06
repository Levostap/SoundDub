#include "audiocapture.h"
#include <vector>
#include <locale>
#include <codecvt>


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib,"winmm.lib") 


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Создание кнопки
            HWND button = CreateWindowEx(0, "BUTTON", "Start on device", 
                                         WS_CHILD | WS_VISIBLE, 10, 10, 120, 30, 
                                         hwnd, (HMENU)101, NULL, NULL);

            // Создание ComboBox
            HWND comboBox = CreateWindowEx(0, "COMBOBOX", NULL, 
                                           WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 
                                           10, 50, 120, 200, hwnd, (HMENU)102, NULL, NULL);

            // Заполнение ComboBox элементами
            std::vector<LPWSTR> dev_list = AudioCapture::getAllDevices();
            for (int i = 0; i < dev_list.size(); i++) {
                std::wstring wideString(dev_list[i]);
                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
                std::string narrowString = converter.to_bytes(wideString);
                SendMessage(comboBox, CB_ADDSTRING, 0, (LPARAM)(narrowString));
                printf("Device Name: %s\n", (wchar_t*)dev_list[i]);
                //SendMessage(comboBox, CB_ADDSTRING, 0, (LPARAM)items[i]);
            }

            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 101) {
                // Обработка нажатия кнопки
                MessageBox(hwnd, "Кнопка была нажата", "Сообщение", MB_OK);
            }
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


int main() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = "MainWindow";
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(
        0, "MainWindow", "EXAMPLE",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        300, 150, NULL, NULL, GetModuleHandle(0), NULL
    );
    ShowWindow(hwnd, SW_SHOWNORMAL);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}