#declare WALLTYPE = merge {
    #declare VERTBRICKS = 4;
    #declare HORZBRICKS = 4;
    #declare BRICKHEIGHT = WALLHEIGHT/VERTBRICKS;
    #declare BRICKWIDTH = WALLWIDTH/HORZBRICKS;
    #declare GAP = BRICKHEIGHT/18;
    #declare GROOVE = GAP*2;
    #declare Y = 0;
    #while (Y<VERTBRICKS)
        #declare X = 0;
        #while (X<HORZBRICKS)
            #declare Z = 0;
            #while (Z<HORZBRICKS)
                box {
                    <-WALLWIDTH/2+(BRICKWIDTH)*X+GAP,-WALLHEIGHT/2+(BRICKHEIGHT)*Y+GAP,-WALLWIDTH/2+(BRICKWIDTH)*Z+GAP>,
                    <-WALLWIDTH/2+(BRICKWIDTH)*(X+1)-GAP,-WALLHEIGHT/2+(BRICKHEIGHT)*(Y+1)-GAP, -WALLWIDTH/2+(BRICKWIDTH)*(Z+1)-GAP>
                    pigment { rgb <1,1,1> }
                    normal { dents 0.75 scale 4 }
                }
                #declare Z = Z + 1;
            #end
            #declare X = X + 1;
        #end
        #declare Y = Y + 1;
    #end
    box {
        <-WALLWIDTH/2+GROOVE,-WALLHEIGHT/2,-WALLWIDTH/2+GROOVE>,
        < WALLWIDTH/2-GROOVE, WALLHEIGHT/2, WALLWIDTH/2-GROOVE>
        pigment { rgb <0.6,0.2,0> }
    }
}
