add_library(tenbox_platform STATIC
    ${CMAKE_SOURCE_DIR}/src/platform/windows/hypervisor/whvp_platform.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/windows/hypervisor/whvp_vm.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/windows/hypervisor/whvp_vcpu.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/windows/console/std_console_port.cpp
)

target_include_directories(tenbox_platform
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(tenbox_platform
    PUBLIC
        tenbox_core
        WinHvPlatform
        WinHvEmulation
)
