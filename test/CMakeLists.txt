set(TESTS
    #"src/test_utils.cpp"
    "src/test_packet.cpp"
    #"src/test_lamport_clock.cpp"
    #"src/test_message_queue.cpp"
    #"src/test_logger.cpp"
    #"src/test_node.cpp"
)

add_executable(tests ${TESTS})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_link_libraries(tests gtest_main distributed-me)

add_test(
    NAME ${This}
    COMMAND ${This}
)