#include <iostream>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmeapi.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <Audiopolicy.h>
#include <fstream>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "uuid.lib")

// Функция для инициализации WASAPI захвата звука
HRESULT InitializeAudioCapture(IAudioClient** audioClient, IAudioCaptureClient** captureClient) {
    HRESULT hr;
    IMMDeviceEnumerator* deviceEnumerator = NULL;
    IMMDevice* defaultDevice = NULL;

    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        return hr;
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
    if (FAILED(hr)) {
        CoUninitialize();
        return hr;
    }

    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
        deviceEnumerator->Release();
        CoUninitialize();
        return hr;
    }

    
    hr = defaultDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)audioClient);
    defaultDevice->Release();
    if (FAILED(hr)) {
        deviceEnumerator->Release();
        CoUninitialize();
        return hr;
    }

    WAVEFORMATEX* pwfx = NULL;
    (*audioClient)->GetMixFormat(&pwfx);

    hr = (*audioClient)->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pwfx, NULL);
    if (FAILED(hr)) {
        (*audioClient)->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return hr;
    }


    hr = (*audioClient)->GetService(__uuidof(IAudioCaptureClient), (void**)captureClient);
    if (FAILED(hr)) {
        (*audioClient)->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return hr;
    }
    std::cout << pwfx->nAvgBytesPerSec << " " << pwfx->wFormatTag; 
    return S_OK;
}

int main() {
    // Инициализация соксов
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    // Инициализация WASAPI захвата звука
    IAudioClient* audioClient = NULL;
    IAudioCaptureClient* captureClient = NULL;
    if (FAILED(InitializeAudioCapture(&audioClient, &captureClient))) {
        std::cerr << "Failed to initialize audio capture." << std::endl;
        return 1;
    }

    // Создание сокета
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        audioClient->Release();
        captureClient->Release();
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serverAddr.sin_port = htons(7777);

    /*if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to the server." << std::endl;
        closesocket(serverSocket);
        audioClient->Release();
        captureClient->Release();
        WSACleanup();
        return 1;
    }*/

    // Захват и передача аудио
    const int bufferSize = 4096; // Размер буфера аудио данных
    BYTE* audioBuffer = new BYTE[bufferSize];
    UINT32 packetLength = 0;
    DWORD flags;

    while (true) {

        captureClient->GetNextPacketSize(&packetLength);

        HRESULT hr = captureClient->GetBuffer(&audioBuffer, &packetLength, &flags, NULL, NULL);
        if (FAILED(hr)) {
            std::cerr << "Failed to get audio buffer." << std::endl;
            break;
        }

        // Отправляем аудио данные на сервер
        /*int bytesSent = send(serverSocket, reinterpret_cast<const char*>(audioBuffer), packetLength, 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Failed to send audio data." << std::endl;
            break;
        }
        */

        std::ofstream wf("myfile.pcm", std::ios::app | std::ios::binary);
            //wf << pData;
            //std::cout << audioBuffer << std::endl;
            for(int i = 0; i < packetLength; i++){
                wf << audioBuffer[i];
            }

        captureClient->ReleaseBuffer(packetLength);

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            std::cerr << "Audio buffer is silent." << std::endl;
        }
    }

    // Освобождение ресурсов
    closesocket(serverSocket);
    audioClient->Release();
    captureClient->Release();
    WSACleanup();

    return 0;
}