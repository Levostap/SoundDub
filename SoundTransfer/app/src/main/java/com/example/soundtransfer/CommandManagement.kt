package com.example.soundtransfer

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import java.io.BufferedOutputStream
import java.io.DataOutputStream
import java.net.Socket

class CommandManagement(ip : String) {
    private var dis : DataOutputStream = DataOutputStream(BufferedOutputStream(Socket(ip, 7778).getOutputStream()))

    fun sendCommand(command: commandTypes) = runBlocking{
        withContext(Dispatchers.IO) {
            val buffer = byteArrayOf(0, 0,0,0)
            for (i in 0..3) buffer[i] = (command.code shr (i*8)).toByte()
            dis.write(buffer)
            dis.close()
        }
    }
}