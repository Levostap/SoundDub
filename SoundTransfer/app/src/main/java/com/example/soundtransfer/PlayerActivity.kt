package com.example.soundtransfer

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.CheckBox
import android.widget.FrameLayout
import android.widget.ImageButton
import android.widget.TextView
import android.widget.Switch
import androidx.appcompat.widget.SwitchCompat

class PlayerActivity : AppCompatActivity() {
    private lateinit var thread : Player
    private lateinit var ipText : TextView
    private lateinit var additionalMenu : FrameLayout
    private lateinit var additionalMenuButton: Button
    private lateinit var muteButton : CheckBox
    private lateinit var recording : SwitchCompat
    private lateinit var playStateButton : ImageButton
    private lateinit var nextTrack : ImageButton
    private lateinit var prevTrack : ImageButton

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.player_activity)

        ipText = findViewById(R.id.ipData)
        additionalMenu = findViewById(R.id.additional_menu)
        additionalMenuButton = findViewById(R.id.button)
        muteButton = findViewById(R.id.checkBoxMute)
        recording = findViewById(R.id.switchButton)
        playStateButton = findViewById(R.id.play_pause)
        nextTrack = findViewById(R.id.next_track)
        prevTrack = findViewById(R.id.previous_track)

        muteButton.setOnCheckedChangeListener { _, isChecked ->
            thread.isMuted = isChecked
        }
        additionalMenuButton.setOnClickListener {
            if(additionalMenu.visibility == View.GONE){
                additionalMenu.visibility = View.VISIBLE
            }
            else{
                additionalMenu.visibility = View.GONE
            }
        }
        val ip = intent.getStringExtra("ip")
        recording.setOnCheckedChangeListener { _, isChecked ->
            val commandManagement = CommandManagement(ip!!)
            commandManagement.sendCommand(if(isChecked) commandTypes.START_RECORDING else commandTypes.STOP_RECORDING)
        }
        nextTrack.setOnClickListener {
            val commandManagement = CommandManagement(ip!!)
            commandManagement.sendCommand(commandTypes.NEXT)
        }
        prevTrack.setOnClickListener {
            val commandManagement = CommandManagement(ip!!)
            commandManagement.sendCommand(commandTypes.PREV)
        }
        playStateButton.setOnClickListener {
            val commandManagement = CommandManagement(ip!!)
            commandManagement.sendCommand(commandTypes.PLAY_PAUSE)
        }

        ipText.text = ip
        thread = Player(ip!!, 7777)
        thread.start()

    }

    override fun onDestroy() {
        super.onDestroy()
        thread.interrupt()
    }
}