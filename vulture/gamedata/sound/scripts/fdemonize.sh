sox $1 tmp1.wav pitch +700 10 cubic cos
sox $1 tmp2.wav pitch -700 30 cubic cos
soxmix tmp1.wav tmp2.wav $2
rm tmp1.wav tmp2.wav

