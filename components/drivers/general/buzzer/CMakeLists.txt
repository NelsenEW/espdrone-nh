idf_component_register(SRCS "buzzdeck.c" "piezo.c"
                       INCLUDE_DIRS "." "include"
                        REQUIRES espdrone utils platform config)