package com.example.soundtransfer

import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.Bundle
import android.os.StrictMode
import android.os.StrictMode.ThreadPolicy
import android.widget.Button
import android.widget.EditText
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

    private lateinit var loadButton : Button
    private lateinit var ipField : EditText

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        ipField = findViewById(R.id.editTextText)
        loadButton = findViewById(R.id.connect_button)

        val policy = ThreadPolicy.Builder().permitAll().build()
        StrictMode.setThreadPolicy(policy)
        loadButton.setOnClickListener {
            val pl = Player(ipField.text.toString(), 7777)
            val thread = Thread(pl)
            thread.start()
        }
    }

}
