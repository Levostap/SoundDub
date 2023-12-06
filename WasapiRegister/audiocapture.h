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
#include <vector>

class AudioCapture{
    private:
        bool isRec;
        bool isCon;
    public:
        AudioCapture() = default;
        AudioCapture(SOCKET serverSocket);
        
        static std::vector<LPWSTR> getAllDevices();

        int runInDevice(int devNum);

        bool isRecording();
        bool isConnected();

        int startRecording();
        int stopRecording();
};