cmake_minimum_required(VERSION 3.16)
project(SamFlashAlternative VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific options
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

# Find required packages
find_package(Threads REQUIRED)

# Try to find libserialport
set(LIBSERIALPORT_FOUND FALSE)
if(UNIX)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBSERIALPORT QUIET libserialport)
    endif()
else()
    # On Windows, try to find libserialport manually or use vcpkg
    find_path(LIBSERIALPORT_INCLUDE_DIR libserialport.h)
    find_library(LIBSERIALPORT_LIBRARY serialport)
    if(LIBSERIALPORT_INCLUDE_DIR AND LIBSERIALPORT_LIBRARY)
        set(LIBSERIALPORT_FOUND TRUE)
        set(LIBSERIALPORT_INCLUDE_DIRS ${LIBSERIALPORT_INCLUDE_DIR})
        set(LIBSERIALPORT_LIBRARIES ${LIBSERIALPORT_LIBRARY})
    endif()
endif()

if(LIBSERIALPORT_FOUND)
    message(STATUS "Found libserialport: ${LIBSERIALPORT_LIBRARIES}")
    add_definitions(-DHAVE_LIBSERIALPORT)
else()
    message(WARNING "libserialport not found - using stub implementation")
endif()

# Include directories
include_directories(src)

# Core library
set(CORE_SOURCES
    src/Core/device_interface.h
    src/Core/flash_manager.h
    src/Core/flash_manager.cpp
    src/Core/device_interface_factory.cpp
    src/Core/serial_transport.h
    src/Core/serial_transport.cpp
    src/Core/usb_serial_interface.h
    src/Core/usb_serial_interface.cpp
)

# Add stub file for reference (included conditionally in serial_transport.cpp)
if(NOT LIBSERIALPORT_FOUND)
    list(APPEND CORE_SOURCES src/Core/serial_transport_stub.cpp)
endif()

add_library(SamFlashCore ${CORE_SOURCES})
target_include_directories(SamFlashCore PRIVATE ${LIBSERIALPORT_INCLUDE_DIRS})
target_link_libraries(SamFlashCore Threads::Threads ${LIBSERIALPORT_LIBRARIES})

# GUI Application (if Qt is available)
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if(Qt6_FOUND)
    set(GUI_SOURCES
        src/GUI/main_window.cpp
        src/GUI/main_window.h
        src/GUI/main.cpp
    )
    
    qt6_standard_project_setup()
    qt6_add_executable(SamFlashGUI ${GUI_SOURCES})
    qt6_add_resources(SamFlashGUI "resources"
        PREFIX "/"
        FILES
            assets/icon.png
    )
    target_link_libraries(SamFlashGUI Qt6::Core Qt6::Widgets SamFlashCore)
endif()

# CLI Tool
add_executable(SamFlashCLI src/Scripts/main.cpp)
target_link_libraries(SamFlashCLI SamFlashCore)

# Tests
enable_testing()
find_package(GTest QUIET)
if(GTest_FOUND)
    set(TEST_SOURCES
        tests/test_flash_manager.cpp
        tests/test_device_interface.cpp
    )
    
    add_executable(SamFlashTests ${TEST_SOURCES})
    target_link_libraries(SamFlashTests GTest::gtest_main SamFlashCore)
    add_test(NAME SamFlashUnitTests COMMAND SamFlashTests)
endif()

# Installation
install(TARGETS SamFlashCore DESTINATION lib)
install(TARGETS SamFlashCLI DESTINATION bin)
if(Qt6_FOUND)
    install(TARGETS SamFlashGUI DESTINATION bin)
endif()

# Package configuration
set(CPACK_PACKAGE_NAME "SamFlash Alternative")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern flash memory programming tool")
set(CPACK_PACKAGE_VENDOR "SamFlash Alternative Project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/docs/LICENSE.txt")
include(CPack)
