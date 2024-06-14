cmake_minimum_required(VERSION 3.28.2)

include(../sdk/config.cmake)
project(sieelf_sie_shot C)

if (APPLE)
    set(CMAKE_C_LINK_FLAGS "")
endif()

include_directories("../libsie/src/include")
link_libraries(crt crt_helper c gcc m z png ft sie)
file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(SieShot_NSG ${SOURCES})
target_sdk_setup(SieShot_NSG NEWSGOLD)
target_link_directories(SieShot_NSG PUBLIC "../libsie/build/lib/NSG")
target_compile_options(SieShot_NSG BEFORE PUBLIC -std=gnu99 -Wall -Werror)

add_executable(SieShot_ELKA ${SOURCES})
target_sdk_setup(SieShot_ELKA ELKA)
target_link_directories(SieShot_ELKA PUBLIC "../libsie/build/lib/ELKA")
target_compile_options(SieShot_ELKA BEFORE PUBLIC -std=gnu99 -Wall -Werror)
