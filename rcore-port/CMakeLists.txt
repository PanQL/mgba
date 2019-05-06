cmake_minimum_required(VERSION 3.0)

##=======  useful macro ========

set(SOURCE_FILES "" CACHE STRING "Source Files" FORCE)
macro(add_source_files)
	file(GLOB_RECURSE newlist ${ARGN})

	foreach (filepath ${newlist})
		string(FIND ${filepath} ${CMAKE_BINARY_DIR} found)
		if (NOT found EQUAL 0)
			set(SOURCE_FILES ${SOURCE_FILES} ${filepath} CACHE STRING "Source Files" FORCE)
		endif ()
	endforeach ()
endmacro()

function(removeDuplicateSubstring stringIn stringOut)
	separate_arguments(stringIn)
	list(REMOVE_DUPLICATES stringIn)
	string(REPLACE ";" " " stringIn "${stringIn}")
	set(${stringOut} "${stringIn}" PARENT_SCOPE)
endfunction()

##===================compile excutable file=====

include(${CMAKE_CURRENT_LIST_DIR}/cmake/common.cmake)

project(mgba C)
add_source_files(src/*.c)

#if ( EXISTS ${CMAKE_CURRENT_LIST_DIR}/../build/libmgba.a ) 
	#add_library(libmgba STATIC IMPORTED GLOBAL)
	#set_property(TARGET libmgba PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_LIST_DIR}/../build/libmgba.a)
#else()
	#message(STATUS "error! libmgba not exits")
#endif()

removeDuplicateSubstring(${CMAKE_C_FLAGS} CMAKE_C_FLAGS)
removeDuplicateSubstring(${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS)

message(STATUS "source files include ${SOURCE_FILES}")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_include_directories(
	${PROJECT_NAME} 
	PUBLIC 
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
)
#set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

#target_link_libraries(${PROJECT_NAME}
        #${CMAKE_CURRENT_LIST_DIR}/../build/libmgba.a
#)
