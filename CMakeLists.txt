idf_build_get_property(project_dir PROJECT_DIR)

# Initialize an empty list to hold conditional requirements
set(conditionally_required_components "")

if(EXISTS "${project_dir}/components/drv_console")
    list(APPEND conditionally_required_components "drv_console")
endif()

if(EXISTS "${project_dir}/components/drv_version")
    list(APPEND conditionally_required_components "drv_version")
endif()


idf_component_register(SRCS "cmd_system.c" "drv_system.c" 
                    INCLUDE_DIRS "." 
                    REQUIRES 
                        "console" 
                        "heap" 
                        ${conditionally_required_components}
                        "spi_flash"
                                      )
                 

