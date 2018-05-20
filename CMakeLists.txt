cmake_minimum_required(VERSION 3.10)
project(apriori C)

set(CMAKE_C_STANDARD 99)

add_executable(apriori apriori.c lib/cJSON.c lib/cJSON.h demo.c apriori.h)
