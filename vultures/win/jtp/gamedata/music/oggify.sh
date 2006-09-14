#!/bin/sh
timidity -Ow1sl -a -o "$1-1.wav" -s 44100 "$1.mid" && sox "$1-1.wav" -sw "$1.wav" && rm "$1-1.wav" && oggenc -o "$1.ogg" "$1.wav" && rm "$1.wav"

