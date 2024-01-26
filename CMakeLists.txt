# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

if(DEFINED ENV{RMAKER_PATH})
  set(RMAKER_PATH $ENV{RMAKER_PATH})
else()
  set(RMAKER_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
endif(DEFINED ENV{RMAKER_PATH})

# Add RainMaker components and other common application components
#set(EXTRA_COMPONENT_DIRS ${RMAKER_PATH}/components/esp-insights/components ${RMAKER_PATH}/components ${RMAKER_PATH}/examples/common  $ENV{IDF_PATH}/examples/bluetooth/esp_ble_mesh/common_components/example_init)

set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/bluetooth/esp_ble_mesh/common_components/example_init
                         ${RMAKER_PATH}/components/esp-insights/components
                         ${RMAKER_PATH}/components ${RMAKER_PATH}/examples/common
                         $ENV{IDF_PATH}/examples/bluetooth/esp_ble_mesh/common_components/example_nvs
                         )


set(PROJECT_VER "2.0")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(multi_device)
