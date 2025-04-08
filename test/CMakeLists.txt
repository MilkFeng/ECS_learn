cmake_minimum_required(VERSION 3.30)
project(ECS_TEST)

include_directories("./googletest/include")
include_directories("../src/")

link_directories("./googletest/lib")

set(GTEST_LIBRARIES gtest gtest_main gmock gmock_main pthread)

add_executable(${PROJECT_NAME}
        entity_test.cc
        storage_test.cc
        registry_test.cc
        commands_test.cc
        system_test.cc
        scheduler_test.cc
        components_test.cc
        viewer_test.cc
        app_test.cc)
target_link_libraries(${PROJECT_NAME} PRIVATE ${GTEST_LIBRARIES})