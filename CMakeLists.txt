idf_component_register(SRCS "cmd_system.c" "drv_system.c" 
                    INCLUDE_DIRS "." 
                    REQUIRES "console" "heap" "drv_console" "drv_version" "spi_flash"
                                      )
                 

