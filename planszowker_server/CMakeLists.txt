set(EXEC_NAME PlanszowkerServer)

set(SOURCES
        main.cpp
)

add_executable(${EXEC_NAME} ${SOURCES})

target_link_libraries(${EXEC_NAME}
                      PRIVATE ErrorHandler
                      PRIVATE GamesServer
                      PRIVATE EasyLogging
                      PRIVATE Supervisor

                      sfml-graphics
                      sfml-network

                      PRIVATE nlohmann_json::nlohmann_json
                      )

target_include_directories(${EXEC_NAME}
                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                             PRIVATE headers/PlanszowkerServer
                             PRIVATE headers
                           )
# Copy config
file(COPY config.plameta DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Copy core lua scripts, except games
file(COPY scripts DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
        PATTERN "scripts/games/**" EXCLUDE)

if (WIN32)
  set(PYTHON_EXEC py -3)
else()
  set(PYTHON_EXEC python3)
endif()

add_custom_target(PackGames ALL
        COMMAND ${PYTHON_EXEC} ${CMAKE_CURRENT_SOURCE_DIR}/../python/pack_games.py ${CMAKE_CURRENT_BINARY_DIR}/scripts/games
        COMMENT "Packing games into .plagame files"
        DEPEND always_rebuild
        VERBATIM
        )

install(TARGETS ${EXEC_NAME} RUNTIME_DEPENDENCIES DESTINATION ${EXEC_NAME}
        ARCHIVE DESTINATION ${EXEC_NAME}
        LIBRARY DESTINATION ${EXEC_NAME}
        RUNTIME DESTINATION ${EXEC_NAME})