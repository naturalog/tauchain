MESSAGE( STATUS "note: you dont have to use cmake, our hand-written Makefile more-or-less works" )
MESSAGE( STATUS "" )

#cmake_minimum_required(VERSION 3.0)
cmake_minimum_required(VERSION 2.8.7)


project(tauchain)


execute_process(COMMAND "sh -c ./perms.py > perms.cpp")


SET (CMAKE_C_COMPILER   "/usr/bin/clang")
SET (CMAKE_CXX_COMPILER "/usr/bin/clang++")
SET (CMAKE_AR      "/usr/bin/llvm-ar")
SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
SET (CMAKE_NM      "/usr/bin/llvm-nm")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")



find_package(Boost REQUIRED regex)



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -ggdb -g")
set(COMMON_FLAGS "-I${Boost_INCLUDE_DIRS} ${CMAKE_CXX_FLAGS} ")
set(ASAN " -fsanitize=address -fsanitize=integer -fsanitize=undefined -fsanitize=unsigned-integer-overflow")


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


foreach(i [ "oneword-racer-asan" "-Doneword -O3 ${ASAN} " ] )
add_executable("tau-${i[0]}" ${SOURCE_FILES})
SET_TARGET_PROPERTIES(tau-${i[0]} PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  ${i[1]}" )
endforeach(i)


add_executable(tau-oneword-racer-asan ${SOURCE_FILES})
add_executable(tau-struct-racer-asan ${SOURCE_FILES})
add_executable(tau-oneword-racer ${SOURCE_FILES})
add_executable(tau-oneword-racer-marpa ${SOURCE_FILES})
add_executable(tau-oneword-debug-marpa ${SOURCE_FILES})
add_executable(tau-oneword-debug ${SOURCE_FILES})
add_executable(tau-struct-debug ${SOURCE_FILES})
add_executable(tau-struct-debug ${SOURCE_FILES})
add_executable(tau-struct-debug-kbdbg ${SOURCE_FILES})


SET_TARGET_PROPERTIES(tau-debug    PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O0" )
SET_TARGET_PROPERTIES(tau-release  	PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS} -O3" )
SET_TARGET_PROPERTIES(tau-new   	PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O0 -DNEW" )
SET_TARGET_PROPERTIES(tau-new   PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O0 -DKBDBG" )
SET_TARGET_PROPERTIES(tau-clitrace   PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS}  -DDEBUG -O1 -Ddebug_cli" )
SET_TARGET_PROPERTIES(m-tau PROPERTIES COMPILE_FLAGS "${COMMON_FLAGS} -I${MARPA_DIST} -Dwith_marpa  -DDEBUG -O0")
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
