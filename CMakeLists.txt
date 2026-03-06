cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(BeaconObjectFileLibrary LANGUAGES CXX)
endif()

set(BOF_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/Core ${CMAKE_CURRENT_SOURCE_DIR}/Core/base)
set(BOF_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bofs)

file(MAKE_DIRECTORY ${BOF_OUTPUT_DIR})

set(BOF_TARGETS
    WhoAmI
    EnumDeviceDrivers
    GetSystemDirectory
    Ipconfig
    FileExfiltrationUrlEncoded
    RegistryPersistence
    TimeStomp
)

# Release: compile-only BOF object files
set(BOF_COPY_COMMANDS)

foreach(BOF_NAME ${BOF_TARGETS})
    add_library(${BOF_NAME} OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/${BOF_NAME}/bof.cpp)
    target_include_directories(${BOF_NAME} PRIVATE ${BOF_INCLUDE_DIRS})
    target_compile_options(${BOF_NAME} PRIVATE /GS- /EHsc)

    list(APPEND BOF_COPY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_OBJECTS:${BOF_NAME}> ${BOF_OUTPUT_DIR}/${BOF_NAME}.obj
    )
endforeach()

add_custom_target(bofs ALL
    ${BOF_COPY_COMMANDS}
    DEPENDS ${BOF_TARGETS}
    COMMENT "Copying BOF object files to ${BOF_OUTPUT_DIR}"
)

# Debug: standalone executables with mocked Beacon APIs
option(BUILD_DEBUG_EXES "Build debug executables for BOFs" OFF)

if(BUILD_DEBUG_EXES)
    set(MOCK_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Core/base/mock.cpp
    )

    foreach(BOF_NAME ${BOF_TARGETS})
        add_executable(${BOF_NAME}_debug
            ${CMAKE_CURRENT_SOURCE_DIR}/${BOF_NAME}/bof.cpp
            ${MOCK_SOURCES}
        )
        target_include_directories(${BOF_NAME}_debug PRIVATE ${BOF_INCLUDE_DIRS})
        target_compile_definitions(${BOF_NAME}_debug PRIVATE _DEBUG)
        target_compile_options(${BOF_NAME}_debug PRIVATE /EHsc /std:c++20)
        set_target_properties(${BOF_NAME}_debug PROPERTIES
            OUTPUT_NAME ${BOF_NAME}
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/debug
        )
    endforeach()
endif()
