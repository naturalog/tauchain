MESSAGE( STATUS "note: you dont have to use cmake, our hand-written Makefile more-or-less works" )
MESSAGE( STATUS "" )

cmake_minimum_required(VERSION 3.0)

project(tauchain)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra")

set(SOURCE_FILES
    cli.cpp
    cli.h
    jsonld.cpp
    jsonld.h
    json_spirit.h
    misc.cpp
    misc.h
    nquads.cpp
    jsonld_object.cpp
    jsonld_object.h
    prover.cpp
    prover.h
    rdf.cpp
    rdf.h
    strings.h
    tau.cpp
)

set(MARPA_FILES ${SOURCE_FILES} marpa_tau.cpp)

find_package(Boost REQUIRED regex)

add_executable(tau ${SOURCE_FILES})
add_executable(m-tau ${MARPA_FILES})

target_link_libraries(m-tau PRIVATE "-L${CMAKE_SOURCE_DIR}/libmarpa/dist/.libs -lmarpa")
SET_TARGET_PROPERTIES(m-tau PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG -Dwith_marpa")

target_link_libraries(m-tau PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_include_directories(m-tau PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(tau PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_include_directories(tau PRIVATE ${Boost_INCLUDE_DIRS})



#<ngladitz> http://pastebin.com/Tm5AYHEg ... I added boost and dl and redid the source file listing to make it build (I didn't have a marpa.cpp) ... I don't really know what belongs where source file wise but it produces a binary :)
#<ngladitz> cmake 3.1 adds support for selection of language standards (though I would require 3.2 for it) which would make the -std=c++11 superfluous ... which in turn would make the build more portable
