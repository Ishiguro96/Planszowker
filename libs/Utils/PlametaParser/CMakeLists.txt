set(LIB_NAME PlametaParser)

project(${LIB_NAME})

set(SOURCES
        Parser.cpp
        Entry.cpp
   )

add_library(${LIB_NAME} STATIC ${SOURCES})

target_link_libraries(${LIB_NAME}
                        PRIVATE EasyLogging
                        PRIVATE ErrorHandler
                      )

target_include_directories(${LIB_NAME}
                             PUBLIC headers

                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                             PRIVATE headers/${LIB_NAME}
                           )

target_compile_definitions(${LIB_NAME} PUBLIC ELPP_THREAD_SAFE)
