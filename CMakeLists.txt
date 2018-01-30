project(eteam-rt)

# At least require cmake version 3.2
cmake_minimum_required(VERSION 3.2)

# Define which directories to use for libraries and executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Generate the compilation database by default -- comment out if not desired
set(CMAKE_EXPORT_COMPILE_COMMANDS On)

# the eteam library
add_library(eteam
    src/eteam.cc
)

target_include_directories(eteam
    PUBLIC include
)


# energy measurement tool
add_executable(energy
    src/program.cc
    src/normal_process.cc
    src/measure.cc
    src/execute.cc
    src/energy.cc
    src/main.cc
)

target_include_directories(energy
    PUBLIC include
)

target_link_libraries(energy
    eteam
)

# Installing targets
install(TARGETS eteam
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

install(TARGETS energy
    RUNTIME DESTINATION bin
)
