idf_component_register(SRCS ./app_driver.c ./app_main.c ./ble_mesh.c ./main_driver.c  ./weather.c ./mount.c ./file_server.c
                             ./updata.c
                       INCLUDE_DIRS ".")

set(EXTRA_COMPONENT_DIRS components)