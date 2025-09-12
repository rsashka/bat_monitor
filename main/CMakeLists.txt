idf_component_register(
    SRCS "main.cpp" 
    SRCS "TaskServer.cpp"
    SRCS "TaskADC.cpp" 
    SRCS "TaskConfig.cpp"
    SRCS "PropertyParser.cpp"
#    PRIV_REQUIRES nvs_flash
    INCLUDE_DIRS "."
    INCLUDE_DIRS "/usr/local/include/magic_enum"
)
