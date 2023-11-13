import struct

import pyaudio
import socket

# Инициализация захвата аудио для воспроизведения
p = pyaudio.PyAudio()
stream = p.open(format=pyaudio.paFloat32, channels=2, rate=48000, output=True)


# Создание сокета и прослушивание подключений
host = '0.0.0.0'  # Принимать подключения с любого адреса
port = 7777
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host, port))
s.listen(1)

print("Ожидание подключения...")
conn, addr = s.accept()
print("Подключено к", addr)

try:
    while True:
        data = conn.recv(1024)
        if not data:
            break

        stream.write(data)# Воспроизведение аудио
except KeyboardInterrupt:
    # Выход из цикла при нажатии Ctrl+C
    stream.stop_stream()
    stream.close()
    p.terminate()
    conn.close()
