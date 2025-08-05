cmake_minimum_required(VERSION 3.16)

project(FileModifier VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
if (NOT Qt6_FOUND)
    find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
endif()

# Set up Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files
set(SOURCES
    main.cpp
    mainwindow.cpp
    fileprocessor.cpp
)

set(HEADERS
    mainwindow.h
    fileprocessor.h
)

set(FORMS
    mainwindow.ui
)

# Create executable
add_executable(FileModifier ${SOURCES} ${HEADERS} ${FORMS})

# Link Qt libraries
target_link_libraries(FileModifier Qt::Core Qt::Widgets)

# Set output directory
set_target_properties(FileModifier PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Windows specific settings
if(WIN32)
    set_target_properties(FileModifier PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install rules
install(TARGETS FileModifier
    RUNTIME DESTINATION bin
) 