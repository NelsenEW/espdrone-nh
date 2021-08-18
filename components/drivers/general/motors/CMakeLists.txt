idf_component_register(SRCS "motors_def_ed2.c" "motors.c"
                       INCLUDE_DIRS "." "include"
                        REQUIRES espdrone utils platform config)