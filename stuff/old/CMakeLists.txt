ffff
execute_process(COMMAND "sh -c ./perms.py > perms.cpp")



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
