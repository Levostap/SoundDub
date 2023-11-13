package com.example.soundtransfer

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.Bundle
import android.os.StrictMode
import android.os.StrictMode.ThreadPolicy
import androidx.appcompat.app.AppCompatActivity
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.net.DatagramPacket
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder


class MainActivity : AppCompatActivity() {

    lateinit var socket : Socket
    //lateinit var sin: InputStream
    lateinit var dis : DataInputStream

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val policy = ThreadPolicy.Builder().permitAll().build()
        StrictMode.setThreadPolicy(policy)
        socketSetUp()
        //reserve()
        proceedPlayer()
    }

    fun socketSetUp(){
        val serv = ServerSocket(PORT, 0, InetAddress.getByName(IP))
        socket = serv.accept()

        dis = DataInputStream(BufferedInputStream(socket.getInputStream()))

    }

    fun proceedPlayer(){

        //val af : AudioFormat = AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_FLOAT).setSampleRate(48000).build()
        //val aa : AudioAttributes = AudioAttributes.Builder()
        //    .setUsage(AudioAttributes.USAGE_MEDIA)
        //    .setContentType(AudioAttributes.CONTENT_TYPE_UNKNOWN)
        //    .build()
        //val at = AudioTrack(aa, af, 1024 , AudioTrack.MODE_STATIC, AudioManager.AUDIO_SESSION_ID_GENERATE)
        val at = AudioTrack.Builder()
            .setAudioAttributes(
                AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_MEDIA)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                    .build()
            )
            .setAudioFormat(
                AudioFormat.Builder()
                    .setEncoding(AudioFormat.ENCODING_PCM_FLOAT)
                    .setSampleRate(48000)
                    .setChannelMask(AudioFormat.CHANNEL_OUT_STEREO)
                    .build()
            )
            .setBufferSizeInBytes(1024)
            .build()
        var buffer = ByteArray(1024)
        var bufferF = FloatArray(256)
        var count : Int
        //Thread.sleep(1000)
        at.play()
        while(dis.read(buffer).also { count = it } > 0){
            var bBuffer = ByteBuffer.wrap(buffer).order(ByteOrder.LITTLE_ENDIAN)
            for (i : Int in 0 until count step(4)){
                bufferF[i / 4] = bBuffer.getFloat(i)
            }
            at.write(bufferF, 0, count / 4, AudioTrack.WRITE_BLOCKING)
        }
    }

    fun convertCharToFloat(data: ByteArray, size : Int): FloatArray {
        val floatData = FloatArray(data.size / 4) // Так как у нас 32-битные данные, каждый сэмпл занимает 4 байта

        val maxInt32Value = (1 shl 31).toFloat()

        for (i in data.indices step 4) {
            val sampleBytes = byteArrayOf(data[i], data[i + 1], data[i + 2], data[i + 3])
            val sampleInt = ByteBuffer.wrap(sampleBytes).order(ByteOrder.LITTLE_ENDIAN).int
            val normalizedSample = sampleInt.toFloat() / maxInt32Value

            floatData[i / 4] = normalizedSample
        }

        return floatData
    }

    fun reserve(){
        val buffer = ByteArray(8192)

        val SAMPLE_RATE = 48000;
        val CHANNEL_COUNT = AudioFormat.CHANNEL_OUT_STEREO;
        val ENCODING = AudioFormat.ENCODING_PCM_FLOAT;

        val bufferSize = AudioTrack.getMinBufferSize(SAMPLE_RATE, CHANNEL_COUNT, ENCODING)
        val audioAttributes = AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_MEDIA)
            .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
            .build()
        val audioFormat = AudioFormat.Builder()
            .setEncoding(ENCODING)
            .setSampleRate(SAMPLE_RATE)
            .build()
        val audioTrack = AudioTrack(
            audioAttributes,
            audioFormat,
            bufferSize,
            AudioTrack.MODE_STREAM,
            AudioManager.AUDIO_SESSION_ID_GENERATE
        )
        audioTrack.play()

        val packet = DatagramPacket(buffer, buffer.size)
        val af : AudioFormat = AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_FLOAT).setSampleRate(48000).build()
        val aa : AudioAttributes = AudioAttributes.Builder()
            .setUsage(AudioAttributes.USAGE_MEDIA)
            .setContentType(AudioAttributes.CONTENT_TYPE_UNKNOWN)
            .build()

        val floatBuffer = ByteBuffer.wrap(packet.data).asFloatBuffer()
        val audioFloats = FloatArray(floatBuffer.capacity())
        floatBuffer[audioFloats]

        for (i in audioFloats.indices) {
            audioFloats[i] = audioFloats[i] / 0x8000000
        }
        var count = 0
        while(dis.read(buffer).also { count = it } > 0) {
            audioTrack.write(audioFloats, 0, audioFloats.size - 1, AudioTrack.WRITE_NON_BLOCKING);
        }
    }

    companion object{
        val PORT = 7777
        val IP = "0.0.0.0"
    }
}
