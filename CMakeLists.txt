cmake_minimum_required(VERSION 3.0)
project(tauchain)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    cli.cpp
    cli.h
    json_spirit.h
    jsonld.cpp
    jsonld.h
    Makefile
    marpa.cpp
    marpa.h
    match.cpp
    match.h
    misc.cpp
    misc.h
    nquads.cpp
    object.cpp
    object.h
    prover.cpp
    prover.h
    rdf.cpp
    rdf.h
    strings.h
    tau.cpp
    ThreadPool.h)

add_executable(tau ${SOURCE_FILES})


#option (USE_MYMATH
#       "Use tutorial provided math implementation" ON)