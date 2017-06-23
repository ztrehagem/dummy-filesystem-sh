cmake_minimum_required(VERSION 3.7)

include_directories(src)

add_executable(main
  main.c
  src/hexdump.c
)
