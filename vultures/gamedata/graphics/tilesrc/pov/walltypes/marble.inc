#include "colors.inc"
#declare WALLTYPE = merge {
    #declare GAP = 3;
    #declare DEPTH = 1;
    box {
        <-WALLWIDTH/2,-WALLHEIGHT/2,-WALLWIDTH/2>,
        < WALLWIDTH/2, WALLHEIGHT/2, WALLWIDTH/2>
        pigment {
            rgb <0.7,0.7,0.7>
        }
        normal {
            granite
        }
    }
    merge {
        box {
            <-WALLWIDTH/2+GAP,-WALLHEIGHT/2+GAP,-(WALLWIDTH/2+DEPTH)>,
            < -GAP/2, -GAP/2, (WALLWIDTH/2+DEPTH)>
        }
        box {
            < GAP/2,-WALLHEIGHT/2+GAP,-(WALLWIDTH/2+DEPTH)>,
            < WALLWIDTH/2-GAP, -GAP/2, (WALLWIDTH/2+DEPTH)>
        }
        box {
            <-WALLWIDTH/2+GAP,GAP/2,-(WALLWIDTH/2+DEPTH)>,
            < -GAP/2, WALLHEIGHT/2-GAP, (WALLWIDTH/2+DEPTH)>
        }
        box {
            < GAP/2,GAP/2,-(WALLWIDTH/2+DEPTH)>,
            < WALLWIDTH/2-GAP, WALLHEIGHT/2-GAP, (WALLWIDTH/2+DEPTH)>
        }

        box {
            <-(WALLWIDTH/2+DEPTH),-WALLHEIGHT/2+GAP,-WALLWIDTH/2+GAP>,
            < (WALLWIDTH/2+DEPTH), -GAP/2, -GAP/2>
        }
        box {
            <-(WALLWIDTH/2+DEPTH),-WALLHEIGHT/2+GAP, GAP/2>,
            < (WALLWIDTH/2+DEPTH), -GAP/2, WALLWIDTH/2-GAP>
        }
        box {
            <-(WALLWIDTH/2+DEPTH), GAP/2,-WALLWIDTH/2+GAP>,
            < (WALLWIDTH/2+DEPTH), WALLHEIGHT/2-GAP, -GAP/2>
        }
        box {
            <-(WALLWIDTH/2+DEPTH), GAP/2, GAP/2>,
            < (WALLWIDTH/2+DEPTH), WALLHEIGHT/2-GAP, WALLWIDTH/2-GAP>
        }

        texture {
            pigment {
                agate
                colour_map {
                    [0.0 colour White]
                    [0.9 colour White]
                    [1.0 rgb <0.8,0.5,0.8>]
                }
                scale <12,12,18>
            }
            finish {
                phong 0.8
                phong_size 200
                diffuse 0.8
                ambient 0.2
            }
        }
    }
}
