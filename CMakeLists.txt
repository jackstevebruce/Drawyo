cmake_minimum_required(VERSION 3.16)
project(drawyo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(XFIXES REQUIRED xfixes)

# High performance optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(SOURCES
    src/main.cpp
    src/StrokeEngine.cpp
    src/CanvasOverlay.cpp
    src/ToolbarWidget.cpp
    src/ExportManager.cpp
    src/TrayManager.cpp
)

set(HEADERS
    src/DrawingTypes.h
    src/StrokeEngine.h
    src/CanvasOverlay.h
    src/ToolbarWidget.h
    src/ExportManager.h
    src/TrayManager.h
)

add_executable(drawyo ${SOURCES} ${HEADERS})

target_include_directories(drawyo PRIVATE
    src
    ${X11_INCLUDE_DIRS}
    ${XFIXES_INCLUDE_DIRS}
)

target_link_libraries(drawyo PRIVATE
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
    ${X11_LIBRARIES}
    ${XFIXES_LIBRARIES}
)

install(TARGETS drawyo DESTINATION bin)
install(FILES resources/drawyo.desktop DESTINATION share/applications)
