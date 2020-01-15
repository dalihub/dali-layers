cmake_minimum_required(VERSION 3.9)
project(dali-layers)

set(CMAKE_CXX_STANDARD 14)

set(DALI_CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../dali-core )
set(DALI_ADAPTOR_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../dali-adaptor )
set(DALI_LAYERS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../dali-layers )

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ )
set(DALI_LAYER_SRC
    ${ROOT_DIR}/dali/devel-api/validation-layers.cpp
    ${ROOT_DIR}/dali/devel-api/validation-layers.h
    ${ROOT_DIR}/dali/devel-api/validation-layer-config.cpp
    ${ROOT_DIR}/dali/devel-api/validation-layer-config.h
    )

SET(DESKTOP_PREFIX $ENV{DESKTOP_PREFIX})
SET(INSTALL_PATH $ENV{DESKTOP_PREFIX})

ADD_COMPILE_OPTIONS( -fPIC )
#add_library( library SHARED library.cpp library.h )
add_library( dali-layers SHARED ${DALI_LAYER_SRC} )
INSTALL( TARGETS dali-layers LIBRARY DESTINATION ${INSTALL_PATH}/lib )

INCLUDE( ${ROOT_DIR}/dali/layers/build.cmake )

#add_library( validation SHARED validation.cpp layer.h library.h )
#add_library( validation2 SHARED validation2.cpp layer.h library.h )

include_directories( ${DALI_ADAPTOR_PATH} ${DALI_CORE_PATH} ${DALI_LAYERS_PATH} )
#target_link_libraries( validation dali-layer dl )
#target_link_libraries( validation2 dali-layer dl )



add_executable(hard_layer ${ROOT_DIR}/dali/dummy.cpp ${ROOT_DIR}/dali/devel-api/validation-layers.h)
#target_link_libraries( hard_layer dali-layer library dl )
#install( TARGETS dali-layer DESTINATION /tmp )
#install( TARGETS validation LIBRARY DESTINATION /d/development/DALI-Release )
