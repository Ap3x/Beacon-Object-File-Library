cmake_minimum_required(VERSION 3.20)

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

set(BOF_COPY_COMMANDS)

foreach(BOF_NAME ${BOF_TARGETS})
    add_library(${BOF_NAME} OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/${BOF_NAME}/bof.cpp)
    target_include_directories(${BOF_NAME} PRIVATE ${BOF_INCLUDE_DIRS})
    target_compile_options(${BOF_NAME} PRIVATE /GS- /EHsc)

    list(APPEND BOF_COPY_COMMANDS
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_OBJECTS:${BOF_NAME}> ${BOF_OUTPUT_DIR}/${BOF_NAME}.obj
    )
endforeach()

# Convenience target to build all BOFs and copy them
add_custom_target(bofs ALL
    ${BOF_COPY_COMMANDS}
    DEPENDS ${BOF_TARGETS}
    COMMENT "Copying BOF object files to ${BOF_OUTPUT_DIR}"
)
