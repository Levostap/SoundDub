import pyaudio
import socket

# Инициализируйте захват звука с выхода звука
p = pyaudio.PyAudio()
stream = p.open(format=pyaudio.paFloat32, channels=2, input=True, rate=44100, frames_per_buffer=1024)

# Создайте сокет и подключитесь к удаленному устройству
host = '127.0.0.1'
port = 7777
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((host, port))

try:
    while True:
        data = stream.read(1024)  # Захват аудио
        s.send(data)  # Отправка данных через сокет
except KeyboardInterrupt:
    # Выход из цикла при нажатии Ctrl+C
    stream.stop_stream()
    stream.close()
    p.terminate()
    s.close()

