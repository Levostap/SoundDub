#include "audiocapture.h"
#include <vector>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }

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

int AudioCapture::runInDevice(int devNum){
    HRESULT er;
    WSADATA wsaData;
    
    UINT32 packetLength = 0;
    BYTE *pData;
    DWORD flags;

    sockaddr_in clientAddr;
    socklen_t clientAddressLength = sizeof(clientAddr);
    SOCKET clientSocket;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    SOCKET servSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(7777);

    CoInitialize(NULL);

    IMMDeviceEnumerator* enumerator;
    er = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    EXIT_ON_ERROR(er);

    int deviceNum;
    std::cin >> deviceNum;
    std::cout << std::endl << inet_ntoa(serverAddress.sin_addr) << std::endl;

    if (bind(servSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Error binding socket" << std::endl;
        closesocket(servSocket);
        return EXIT_FAILURE;
    }
    //connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

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
    er = devices->Item(deviceNum, &choosenDevice);
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