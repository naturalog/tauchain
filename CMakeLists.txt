MESSAGE( STATUS "note: you dont have to use cmake, our hand-written Makefile more-or-less works" )
MESSAGE( STATUS "" )

#cmake_minimum_required(VERSION 3.0)
cmake_minimum_required(VERSION 2.8.7)

project(tauchain)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -ggdb -DDEBUG")

set(SOURCE_FILES
    cli.cpp
    cli.h
    jsonld.cpp
    jsonld.h
    json_spirit.h
    misc.cpp
    misc.h
    nquads.cpp
    unifiers.cpp
    json_object.cpp
    json_object.h
    prover.cpp
    prover.h
    rdf.cpp
    rdf.h
    strings.h
    tau.cpp
    univar.cpp
    univar.h
)

set(MARPA_FILES ${SOURCE_FILES} marpa_tau.cpp)
set(MARPA_DIST ${CMAKE_SOURCE_DIR}/libmarpa/dist/)

find_package(Boost REQUIRED regex)

add_executable(tau ${SOURCE_FILES})
add_executable(m-tau ${MARPA_FILES})
add_executable(m-tau3 ${MARPA_FILES})

set(COMMON_FLAGS "-I${Boost_INCLUDE_DIRS} ${CMAKE_CXX_FLAGS} -g -ggdb")# -DPREDVARS")

SET_TARGET_PROPERTIES(tau   PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O0" )
SET_TARGET_PROPERTIES(m-tau PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS} -I${MARPA_DIST} -Dwith_marpa  -DDEBUG -O0")
SET_TARGET_PROPERTIES(m-tau3 PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS} -I${MARPA_DIST} -Dwith_marpa -O3")

target_link_libraries(m-tau3 PRIVATE "-L${MARPA_DIST}/.libs -lmarpa")
target_link_libraries(m-tau3  PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(m-tau  PRIVATE "-L${MARPA_DIST}/.libs -lmarpa")
target_link_libraries(m-tau  PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(  tau  PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
#target_include_directories(m-tau PRIVATE ${Boost_INCLUDE_DIRS} ${MARPA_DIST})
#target_include_directories(  tau PRIVATE ${Boost_INCLUDE_DIRS})



#<ngladitz> http://pastebin.com/Tm5AYHEg ... I added boost and dl and redid the source file listing to make it build (I didn't have a marpa.cpp) ... I don't really know what belongs where source file wise but it produces a binary :)
#<ngladitz> cmake 3.1 adds support for selection of language standards (though I would require 3.2 for it) which would make the -std=c++11 superfluous ... which in turn would make the build more portable
