package com.example.soundtransfer

enum class commandTypes(val code : Int) {
    NEXT(1),
    PLAY_PAUSE(2),
    PREV(3),
    START_RECORDING(4),
    STOP_RECORDING(5)
}