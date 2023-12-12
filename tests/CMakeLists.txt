find_package(Catch2 3 REQUIRED)

set(executable test-${PROJECT_NAME})
add_executable(${executable})

target_sources(${executable}
    PRIVATE utility.test.cpp
    PRIVATE maybe.test.cpp)
target_link_libraries(${executable}
    PRIVATE ${PROJECT_NAME}
    PRIVATE Catch2::Catch2WithMain)

if (MSVC)
    target_compile_options(${executable} PRIVATE "/W4")
else ()
    target_compile_options(${executable} PRIVATE "-Wall" "-Wextra" "-Wpedantic")
endif ()

include(Catch)
catch_discover_tests(${executable})
