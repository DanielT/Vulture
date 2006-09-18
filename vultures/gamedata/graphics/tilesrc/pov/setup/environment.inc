#declare WALLWIDTH = 100;
#declare WALLHEIGHT = 78;
#declare SCALE = 0.57;

#macro VULTURES ( WALLTYPE, ROTATION )

camera {
    orthographic
    location <0,0,-500>
    look_at <0,0,0>
    right 144*SCALE
    up 123*SCALE
    rotate <18.5,ROTATION,0>
    translate <0,-1.25,0>
}

light_source {
    <-10,20,-300>
    rgb <1,1,1>
    rotate <0,30,0>
}

light_source {
    <-10,20, 300>
    rgb <1,1,1>
    rotate <0,30,0>
}

difference {
    difference {
        merge {
            WALLTYPE
            hollow
        }
        merge {
            object {
                WALLTYPE
                scale <0.99,1,0.99>
                translate <0,0.01,0>
            }
            object {
                WALLTYPE
                scale <0.99,1,0.99>
                translate <0,-0.01,0>
            }
        }
    }
    box {
        <-WALLWIDTH,-WALLHEIGHT,-WALLWIDTH>,
        < WALLWIDTH, WALLHEIGHT,         0>
        rotate <0,225,0>
    }
}
#end
