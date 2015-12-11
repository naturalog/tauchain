MESSAGE( STATUS "note: you dont have to use cmake, our hand-written Makefile more-or-less works" )
MESSAGE( STATUS "" )

#cmake_minimum_required(VERSION 3.0)
cmake_minimum_required(VERSION 2.8.7)


project(tauchain)




SET (CMAKE_C_COMPILER             "/usr/bin/clang")
SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
SET (CMAKE_AR      "/usr/bin/llvm-ar")
SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
SET (CMAKE_NM      "/usr/bin/llvm-nm")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -ggdb  -DDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fsanitize-undefined-trap-on-error -fsanitize=address -fsanitize=integer -fsanitize=unsigned-integer-overflow")


set(SOURCE_FILES
        jsonld_tau.cpp
        jsonld_tau.h
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
add_executable(tau-new ${SOURCE_FILES})
add_executable(tau-clitrace ${SOURCE_FILES})
add_executable(m-tau ${MARPA_FILES})
add_executable(m-tau3 ${MARPA_FILES})

set(COMMON_FLAGS "-I${Boost_INCLUDE_DIRS} ${CMAKE_CXX_FLAGS} -g -ggdb -DDEBUG")

SET_TARGET_PROPERTIES(tau   PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O1" )
SET_TARGET_PROPERTIES(tau-new   PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O0 -DKBDBG" )
SET_TARGET_PROPERTIES(tau-clitrace   PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O1 -Ddebug_cli -Dwith_marpa " )
SET_TARGET_PROPERTIES(m-tau PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS} -I${MARPA_DIST} -Dwith_marpa -DJSON -DDEBUG -O0")
SET_TARGET_PROPERTIES(m-tau3 PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS} -I${MARPA_DIST} -Dwith_marpa -O3")

target_link_libraries(m-tau3            PRIVATE "-L${MARPA_DIST}/.libs -lmarpa")
target_link_libraries(m-tau3            PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(m-tau             PRIVATE "-L${MARPA_DIST}/.libs -lmarpa")
target_link_libraries(m-tau             PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(  tau             PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(  tau-new         PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
target_link_libraries(  tau-clitrace    PRIVATE ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})
#target_include_directories(m-tau PRIVATE ${Boost_INCLUDE_DIRS} ${MARPA_DIST})
#target_include_directories(  tau PRIVATE ${Boost_INCLUDE_DIRS})



#<ngladitz> http://pastebin.com/Tm5AYHEg ... I added boost and dl and redid the source file listing to make it build (I didn't have a marpa.cpp) ... I don't really know what belongs where source file wise but it produces a binary :)
#<ngladitz> cmake 3.1 adds support for selection of language standards (though I would require 3.2 for it) which would make the -std=c++11 superfluous ... which in turn would make the build more portable
