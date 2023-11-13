#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmeapi.h>
#include <mmdeviceapi.h>
#include <iostream>
#include <functiondiscoverykeys_devpkey.h>
#include <audioclient.h>
#include <fstream>
#include <winsock2.h>
#include <Ws2tcpip.h>


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib,"winmm.lib") 

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

void getAllDevices(IMMDeviceCollection* devices){
    UINT count;
    devices->GetCount(&count);
    IPropertyStore* iprop = NULL;
    for (UINT i = 0; i < count; i++) {
        IMMDevice* device;
        devices->Item(i, &device);

        LPWSTR deviceId;
        device->GetId(&deviceId);

        device->OpenPropertyStore(STGM_READ, &iprop);

        
        PROPVARIANT deviceName;
        PropVariantInit(&deviceName);
        iprop->GetValue(PKEY_Device_FriendlyName, &deviceName);
        wprintf(L"Device %d:\n", i);
        wprintf(L"Device Name: %s\n", deviceName.pwszVal);
        wprintf(L"-------------------------\n");

        CoTaskMemFree(deviceId);
        device->Release();
    }
}

void playing(BYTE *pData, WAVEFORMATEX* e, UINT32 numFramesAvailable);

int main() {

    
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData) != 0;
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("185.130.233.71");
    serverAddress.sin_port = htons(7777);


    // Инициализация COM
    CoInitialize(NULL);

    // Создание экземпляра IMMDeviceEnumerator
    IMMDeviceEnumerator* enumerator;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);

    // Получение списка аудиоустройств
    IMMDeviceCollection* devices;
    enumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &devices);

    getAllDevices(devices);

    int deviceNum;
    std::cin >> deviceNum;

    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    IMMDevice* choosenDevice = NULL;
    devices->Item(deviceNum, &choosenDevice);

    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;

    choosenDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);

    WAVEFORMATEX *pwfx = NULL;
    pAudioClient->GetMixFormat(&pwfx);

    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;

    pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, hnsRequestedDuration, 0, pwfx, NULL);

    pAudioClient->GetBufferSize(&bufferFrameCount);

    pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pCaptureClient);

    hnsActualDuration = (double)REFTIMES_PER_SEC *
                     bufferFrameCount / pwfx->nSamplesPerSec;

    pAudioClient->Start();

    int timer = 0;
    UINT32 packetLength = 0;
    BYTE *pData;
    DWORD flags;
    //FILE* file = fopen( "myfile.bin", "wb" );
    while(timer <= 1000000){
        Sleep(hnsActualDuration/REFTIMES_PER_MILLISEC/2);

        pCaptureClient->GetNextPacketSize(&packetLength);

        while(packetLength != 0){
            pCaptureClient->GetBuffer(&pData, &numFramesAvailable, &flags, NULL, NULL);


            if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
            {
                pData = NULL;
            }

            /*std::cout << pwfx->nAvgBytesPerSec<< std::endl;
            std::cout << pwfx->nSamplesPerSec << "   " << pwfx->nChannels << std::endl;
            exit(0);*/
            //std::cout << (int)(char)(unsigned char)254 << " ";
            send(clientSocket, reinterpret_cast<const char*>(pData), sizeof((const char*)pData) * packetLength, 0);
            //std::ofstream wf("myfile.pcm", std::ios::app | std::ios::binary);
            //wf << pData;
            //for(int i = 0; i < packetLength; i++){
            //    wf << pData[i];
                //pData[i] = 0;
            //}
            //playing(pData, pwfx, numFramesAvailable);

            pCaptureClient->ReleaseBuffer(numFramesAvailable);
            pCaptureClient->GetNextPacketSize(&packetLength);

        }

        timer++;
    }

    // Освобождение ресурсов и завершение работы с COM
    devices->Release();
    enumerator->Release();
    CoUninitialize();

    return 0;
}

void playing(BYTE *pData, WAVEFORMATEX* wfx, UINT32 NUMPTS){
        //const int sampleRate;
        //const int NUMPTS = SampleRate;
        BYTE* waveOut = pData;//new BYTE [NUMPTS];
        //memcpy(waveIn, pData, sizeof(pData) * sampleRate);
        WAVEFORMATEX wfOut;
        HWAVEOUT hWaveOut;
        WAVEHDR WaveOutHdr;
        WAVEHDR waveInHdr;
        WaveOutHdr.lpData = (LPSTR)waveOut;
            WaveOutHdr.dwBufferLength = NUMPTS;
            WaveOutHdr.dwBytesRecorded = 0;
            WaveOutHdr.dwUser = 0L;
            WaveOutHdr.dwFlags = 0L;
            WaveOutHdr.dwLoops = 1L;

            wfOut.wFormatTag = wfx->wFormatTag;
            wfOut.nChannels = wfx->nChannels;
            wfOut.nSamplesPerSec = wfx->nSamplesPerSec;
            wfOut.wBitsPerSample = wfx->wBitsPerSample;
            wfOut.nAvgBytesPerSec = wfx->nAvgBytesPerSec;
            wfOut.nBlockAlign = wfx->nBlockAlign;
            wfOut.cbSize = wfx->cbSize;
            waveOutSetVolume(hWaveOut,0xFFFFFFFF);
            

        waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfOut, 0L, 0L, WAVE_FORMAT_DIRECT);
        waveOutPrepareHeader(hWaveOut,&WaveOutHdr,sizeof(WAVEHDR));
        waveOutWrite(hWaveOut,&WaveOutHdr,sizeof(WAVEHDR));
        //waveOutRestart(hWaveOut);
        //do{}
        //while(waveOutUnprepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR)) == WAVERR_STILLPLAYING);
}