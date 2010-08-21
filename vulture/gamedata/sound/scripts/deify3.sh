sox $1 tmp1.wav echo 1.0 1.0 217 0.5 111 0.2 310 0.2 400 0.2 61 0.3
sox $1 tmp2.wav pitch -700 30 cubic cos vol 0.7
sox tmp2.wav tmp3.wav echo 1.0 1.0 409 0.3 77 0.4 42 0.2 600 0.2 200 0.3
soxmix tmp1.wav tmp3.wav $2
rm tmp1.wav tmp2.wav tmp3.wav

