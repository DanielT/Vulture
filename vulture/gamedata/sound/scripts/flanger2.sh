sox $1 tmp1.wav flanger 0.7 0.87 3.0 0.9 0.5 -t
sox tmp1.wav $2 flanger 0.8 0.88 3.0 0.4 0.5 -t
rm tmp1.wav

