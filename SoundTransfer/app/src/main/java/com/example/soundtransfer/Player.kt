package com.example.soundtransfer

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioTrack
import java.io.BufferedInputStream
import java.io.DataInputStream
import java.lang.Exception
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder

class Player(ip : String, port : Int) : Runnable {
    private val socket : Socket = Socket(ip, port)
    private var dis : DataInputStream = DataInputStream(BufferedInputStream(socket.getInputStream()))


    override fun run(){
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
        try{
            at.play()
            while(dis.read(buffer).also { count = it } > 0){
                var bBuffer = ByteBuffer.wrap(buffer).order(ByteOrder.LITTLE_ENDIAN)
                for (i : Int in 0 until count step(4)){
                    bufferF[i / 4] = bBuffer.getFloat(i)
                }
                at.write(bufferF, 0, count / 4, AudioTrack.WRITE_BLOCKING)
            }
        } catch (e : Exception){
            e.printStackTrace()
        }
    }
}