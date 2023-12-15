set(executable test-${PROJECT_NAME})
add_executable(${executable})

target_sources(${executable}
    PRIVATE test_utility.hpp
    PRIVATE test_main.cpp
    PRIVATE utility.test.cpp
    PRIVATE meta.test.cpp
    PRIVATE maybe.test.cpp)
target_link_libraries(${executable}
    PRIVATE ${PROJECT_NAME})

if (MSVC)
    target_compile_options(${executable} PRIVATE "/W4")
else ()
    target_compile_options(${executable} PRIVATE "-Wall" "-Wextra" "-Wpedantic")
endif ()
