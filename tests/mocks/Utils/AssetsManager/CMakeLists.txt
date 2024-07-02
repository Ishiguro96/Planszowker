set(LIB_NAME AssetsManagerMock)

add_library(${LIB_NAME} INTERFACE)

target_include_directories(${LIB_NAME} INTERFACE .)

target_link_libraries(${LIB_NAME}
        INTERFACE AssetsManager)
