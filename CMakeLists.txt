cmake_minimum_required(VERSION 3.12)
project(cpp_app)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "../exe")

file(GLOB sourcefiles "*.h" "*.cpp" "src/*.h" "src/*.cpp")
add_executable(${PROJECT_NAME} ${sourcefiles}
)

find_package(OpenGL)
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} SDL2_image SDL2_ttf SDL2 SDL2main)