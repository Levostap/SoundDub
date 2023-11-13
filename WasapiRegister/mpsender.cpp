#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <winsock2.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib,"winmm.lib") 

int main() {

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData) != 0;
    // Открываем файл MP3 для чтения в бинарном режиме
    std::ifstream mp3File("./filea.mp3", std::ios::binary);

    if (!mp3File.is_open()) {
        std::cerr << "Не удалось открыть файл MP3." << std::endl;
        return 1;
    }

    // Читаем содержимое файла в буфер
    std::vector<char> buffer(std::istreambuf_iterator<char>(mp3File), {});

    // Создаем сокет
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Устанавливаем адрес и порт сервера
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(7777);

    // Подключаемся к серверу
    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    std::cout << "Connected" << std::endl;

    send(clientSocket, buffer.data(), buffer.size() , 0);
    
    while(true){}
    // Отправляем данные через сокет


    // Закрываем сокет и файл
    mp3File.close();

    return 0;
}