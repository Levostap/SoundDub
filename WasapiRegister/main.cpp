#include "audiocapture.h"
#include <vector>
#include <locale>
#include <codecvt>
#include <thread>
#include <cstdint>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib,"winmm.lib") 

int commandManager(AudioCapture* ac);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND comboBox;
    switch (uMsg) {
        case WM_CREATE: {
            // Создание кнопки
            HWND button = CreateWindowEx(0, "BUTTON", "Start on device", 
                                         WS_CHILD | WS_VISIBLE, 10, 10, 120, 30, 
                                         hwnd, (HMENU)101, NULL, NULL);
            // Создание ComboBox
            comboBox = CreateWindowEx(0, "COMBOBOX", NULL, 
                                           WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 
                                           10, 50, 200, 200, hwnd, (HMENU)102, NULL, NULL);
            // Заполнение ComboBox элементами
            std::vector<LPWSTR> dev_list = AudioCapture::getAllDevices();
            for (int i = 0; i < dev_list.size(); i++) {
                std::wstring wideString(dev_list[i]);
                std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
                std::string narrowString = converter.to_bytes(wideString);
                SendMessage(comboBox, CB_ADDSTRING, 0, (LPARAM)(narrowString.c_str()));
            }

            break;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == 101) {
                // Обработка нажатия кнопки
                int uSelectedItem;
                uSelectedItem = (int)SendMessage(comboBox,
                    CB_GETCURSEL, 0, 0L);
                AudioCapture ac;
                std::thread ACThread(&AudioCapture::runInDevice, &ac, uSelectedItem);
                std::thread CMDThread(commandManager, &ac);
                ACThread.join();
                CMDThread.join();
                //ac.runInDevice(uSelectedItem);
            } 
            if(LOWORD(wParam) == 102 && HIWORD(lParam) == CBN_SELCHANGE){
                //if(HIWORD(lParam) == CBN_SELENDOK || HIWORD(lParam) == CBN_DROPDOWN){
                    MessageBox(hwnd, "Some sort of parameter was choosen: ", "Was choosen", MB_OK);
                //}
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
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = "MainWindow";
    RegisterClass(&wc);
    HWND hwnd = CreateWindowEx(
        0, "MainWindow", "AudioDubber",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        300, 150, NULL, NULL, GetModuleHandle(0), NULL
    );
    ShowWindow(hwnd, SW_SHOWNORMAL);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    WSACleanup();
    return 0;
}

int commandManager(AudioCapture* ac){
    sockaddr_in clientAddr;
    socklen_t clientAddressLength = sizeof(clientAddr);
    SOCKET clientSocket;
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(7778);
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(sock);
        return EXIT_FAILURE;
    }
    if (listen(sock, 1) == -1) {
        std::cerr << "Error listening on port" << std::endl;
        closesocket(sock);
        return EXIT_FAILURE;
    }
    while(true){
        clientSocket = accept(sock, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddressLength);
        char buf[4];
        recv(clientSocket, buf, sizeof(buf), 0);
        int code;
        memcpy(&code, buf, sizeof(int));
        INPUT input;

        input.type = INPUT_KEYBOARD;
        input.ki.dwFlags = 0;
        switch (code)
        {
        case 1://next
            input.ki.wVk = VK_MEDIA_NEXT_TRACK;
            SendInput(1, &input, sizeof(INPUT));
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            break;
        case 2:
            //pause
            input.ki.wVk = VK_MEDIA_PLAY_PAUSE;
            SendInput(1, &input, sizeof(INPUT));
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            break;
        case 3:
            //prev
            input.ki.wVk = VK_MEDIA_PREV_TRACK;
            SendInput(1, &input, sizeof(INPUT));
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            break;
        case 4:
            ac->rec_mutex.lock();
            ac->setRecord(true);
            ac->rec_mutex.unlock();
            break;
        case 5:
            //stoprec
            ac->rec_mutex.lock();
            ac->setRecord(false);
            ac->rec_mutex.unlock();
            break;
        default:
            break;
        }
        closesocket(clientSocket);
    }
}