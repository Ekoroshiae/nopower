cmake_minimum_required(VERSION 3.2)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(nopower)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O2 -std=gnu99 -nostdlib")

add_executable(nopower plugin.c)

target_link_libraries(nopower
  SceCtrlForDriver_stub
  ScePowerForDriver_stub
  SceThreadmgrForDriver_stub
  SceModulemgrForDriver_stub
  taihenForKernel_stub
  taihenModuleUtils_stub
)

vita_create_self(nopower.skprx nopower CONFIG exports.yml UNSAFE)
