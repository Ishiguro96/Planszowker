set(LIB_NAME GamesDesigner)

project(${LIB_NAME})

set(SOURCES
        GameDesigner.cpp
   )

add_library(${LIB_NAME} STATIC ${SOURCES})

target_link_libraries(${LIB_NAME}
                      )

target_include_directories(${LIB_NAME}
                             PUBLIC headers

                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                             PRIVATE headers/${LIB_NAME}
                           )
