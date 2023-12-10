#include "audiocapture.h"
#include <vector>
#include <intrin.h>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <algorithm>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { /*goto Exit;*/ }

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);


std::vector<LPWSTR> AudioCapture::getAllDevices(){
    CoInitialize(NULL);
    IMMDeviceCollection* devices;
    IMMDeviceEnumerator* enumerator;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    enumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &devices);
    UINT count;
    devices->GetCount(&count);
    IPropertyStore* iprop = NULL;
    std::vector<LPWSTR> out;
    for (UINT i = 0; i < count; i++) {
        IMMDevice* device;
        devices->Item(i, &device);

        LPWSTR deviceId;
        device->GetId(&deviceId);

        device->OpenPropertyStore(STGM_READ, &iprop);
        
        PROPVARIANT deviceName;
        PropVariantInit(&deviceName);
        iprop->GetValue(PKEY_Device_FriendlyName, &deviceName);
        out.push_back(deviceName.pwszVal);
        //wprintf(L"Device %d:\n", i);
        //wprintf(L"Device Name: %s\n", deviceName.pwszVal);
        //wprintf(L"-------------------------\n");

        CoTaskMemFree(deviceId);
        device->Release();
    }
    devices->Release();
    enumerator->Release();
    CoUninitialize();
    return out;
}

int AudioCapture::socketInit(SOCKET* sock){
    
    *sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(7777);
    if (bind(*sock, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(*sock);
        return EXIT_FAILURE;
    }
    return 0;
}

void AudioCapture::setRecord(bool state){
    isRec = state;
}

int AudioCapture::changeRecordingState(const std::vector<float>& audioData){
        std::ofstream file("1.wav", std::ios::binary);
        WavHeader wavHeader;
        std::memcpy(wavHeader.chunkId, "RIFF", 4);
        wavHeader.chunkSize = static_cast<uint32_t>(audioData.size() * sizeof(float) + sizeof(WavHeader) - 8);
        std::memcpy(wavHeader.format, "WAVE", 4);
        std::memcpy(wavHeader.subchunk1Id, "fmt ", 4);
        wavHeader.subchunk1Size = 16; // размер части fmt, фиксированный для формата PCM
        wavHeader.audioFormat = 3; // код формата float (IEEE)
        wavHeader.numChannels = 2;
        wavHeader.sampleRate = 48000;
        wavHeader.bitsPerSample = sizeof(float) * 8;
        wavHeader.byteRate = wavHeader.sampleRate * wavHeader.numChannels * wavHeader.bitsPerSample / 8;
        wavHeader.blockAlign = wavHeader.numChannels * wavHeader.bitsPerSample / 8;
        std::memcpy(wavHeader.subchunk2Id, "data", 4);
        wavHeader.subchunk2Size = static_cast<uint32_t>(audioData.size() * sizeof(float));
        // Запись заголовка в файл
        file.write(reinterpret_cast<char*>(&wavHeader), sizeof(WavHeader)); 
        file.write(reinterpret_cast<const char*>(audioData.data()), audioData.size() * sizeof(float));
        file.close();
        return 0;
}

void convertBytesToFloat(const BYTE* byteArray, size_t byteSize, float* floatArray) {
    // Проверка на корректные аргументы
    if (byteArray == nullptr || floatArray == nullptr) {
        std::cerr << "Error: Invalid arguments." << std::endl;
        return;
    }

    // Проверка на четное количество байтов (для float)
    if (byteSize % sizeof(float) != 0) {
        std::cerr << "Error: Invalid byte size for float conversion." << std::endl;
        return;
    }

    // Копирование байтов в массив float
    std::memcpy(floatArray, byteArray, byteSize);

    std::reverse(reinterpret_cast<char*>(floatArray), reinterpret_cast<char*>(floatArray + byteSize / sizeof(float)));
}

int AudioCapture::runInDevice(int devNum){
    HRESULT er;
    
    UINT32 packetLength = 0;
    BYTE *pData;
    DWORD flags;

    sockaddr_in clientAddr;
    socklen_t clientAddressLength = sizeof(clientAddr);
    SOCKET clientSocket;

    CoInitialize(NULL);

    IMMDeviceEnumerator* enumerator;
    er = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    EXIT_ON_ERROR(er);

    SOCKET servSocket;
    socketInit(&servSocket);
    sock_cr:

    if (listen(servSocket, 1) == -1) {
        std::cerr << "Error listening on port" << std::endl;
        closesocket(servSocket);
        return EXIT_FAILURE;
    }
    clientSocket = accept(servSocket, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddressLength);

    IMMDeviceCollection* devices;
    enumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &devices);

    IMMDevice* choosenDevice = NULL;
    er = devices->Item(devNum, &choosenDevice);
    EXIT_ON_ERROR(er);
    
    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;

    er = choosenDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(er);

    WAVEFORMATEX *pwfx = NULL;
    er = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(er);

    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;

    er = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, NULL);
    EXIT_ON_ERROR(er);

    er = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(er);

    er = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);
    EXIT_ON_ERROR(er);

    hnsActualDuration = (double)REFTIMES_PER_SEC *
                     bufferFrameCount / pwfx->nSamplesPerSec;

    er = pAudioClient->Start();
    EXIT_ON_ERROR(er);
    //unsigned short channels = _byteswap_ushort(pwfx->nChannels);
    //unsigned long samples = _byteswap_ulong(pwfx->nSamplesPerSec);
    //send(clientSocket, (const char*)(samples), sizeof(samples), 0);//автоопределение дискретизации
    //send(clientSocket, (const char*)(pwfx->nChannels), sizeof(pwfx->nChannels), 0);//автоопределение кАналов

    std::vector<float> record;
    while(true){
        Sleep(hnsActualDuration/REFTIMES_PER_MILLISEC/2);

        pCaptureClient->GetNextPacketSize(&packetLength);

        while(packetLength != 0){
            pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);

            if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
            {
                pData = NULL;
            }

            if(send(clientSocket, reinterpret_cast<const char*>(pData), sizeof((const char*)pData) * packetLength, 0) == SOCKET_ERROR){
                closesocket(clientSocket);
                goto sock_cr;
            }


            rec_mutex.lock();
            if(isRec){
                size_t byteSize = sizeof(pData);
                size_t floatSize = byteSize / sizeof(float);
                float* floatArray = new float[floatSize];

                convertBytesToFloat(pData, byteSize, floatArray);
                for(int i = 0; i < floatSize; i++){
                    record.push_back(floatArray[i]);
                }
            }else if(!record.empty()){
                changeRecordingState(record);
                record.clear();
            }
            rec_mutex.unlock();

            pCaptureClient->ReleaseBuffer(numFramesAvailable);
            pCaptureClient->GetNextPacketSize(&packetLength);

        }
    }
    Exit:
        CoTaskMemFree(pwfx);
        SAFE_RELEASE(choosenDevice)
        SAFE_RELEASE(enumerator)
        SAFE_RELEASE(pAudioClient)
        SAFE_RELEASE(pCaptureClient)

    return 1;
}