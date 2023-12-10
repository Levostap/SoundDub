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
#include <mutex>


class AudioCapture{
    private:
        bool isRec;
        bool isCon;
        int socketInit(SOCKET* sock);
    public:
    std::mutex rec_mutex;
        AudioCapture(){
            isRec = false;
            isCon = false;
        };
        
        static std::vector<LPWSTR> getAllDevices();

        int runInDevice(int devNum);

        void setRecord(bool state);
        std::string connected_device();

        int changeRecordingState(const std::vector<float>& audioData);
};

struct WavHeader {
    char chunkId[4];
    uint32_t chunkSize;
    char format[4];
    char subchunk1Id[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char subchunk2Id[4];
    uint32_t subchunk2Size;
};