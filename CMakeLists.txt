cmake_minimum_required(VERSION 3.10)

project(BitmapOMatic CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Bitmap-o-matic
	src/main.cpp
	src/EditorWindow.cpp
	src/IconCanvasView.cpp
	src/IconDocument.cpp
	src/Palette.cpp
	src/Panels.cpp
	src/Raster.cpp
)

target_link_libraries(Bitmap-o-matic be)
