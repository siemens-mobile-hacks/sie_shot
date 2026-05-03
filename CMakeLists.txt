cmake_minimum_required(VERSION 3.28.2)

include(../sdk/config.cmake)
project(sieelf_sie_shot C)

link_libraries(crt crt_helper c gcc m)
file(GLOB_RECURSE SOURCES "src/*.c")

add_executable(SieShot_ELKA ${SOURCES})
target_sdk_setup(SieShot_ELKA ELKA)
target_compile_options(SieShot_ELKA BEFORE PUBLIC -std=c11 -Wall -Werror)
set_target_properties(SieShot_ELKA PROPERTIES OUTPUT_NAME "SieShot")

add_executable(SieShot_NSG ${SOURCES})
target_sdk_setup(SieShot_NSG NEWSGOLD)
target_compile_options(SieShot_NSG BEFORE PUBLIC -std=c11 -Wall -Werror)

add_executable(SieShot_SG ${SOURCES})
target_sdk_setup(SieShot_SG SG)
target_compile_options(SieShot_SG BEFORE PUBLIC -std=c11 -Wall -Werror)
