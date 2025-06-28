cmake_minimum_required(VERSION 3.16)
project(FMKick)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
        rtaudio
        GIT_REPOSITORY https://github.com/thestk/rtaudio.git
        GIT_TAG master
)
FetchContent_MakeAvailable(rtaudio)

add_executable(fm_kick main.cpp)
target_link_libraries(fm_kick PRIVATE rtaudio)
