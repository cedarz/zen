cmake_minimum_required (VERSION 3.12)

set(PROJECT "Zen")
project(${PROJECT})


set(VULKAN_SDK "C:/VulkanSDK/1.2.141.2")
set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
include_directories(
	#${CMAKE_CURRENT_SOURCE_DIR}/Includes
	${VULKAN_SDK}/Include
	${THIRD_PARTY}/glfw/include
	${THIRD_PARTY}/glm/glm
)
			
file(GLOB_RECURSE VK_SOURCES "DRender/*.cpp" "DRender/*.h")

###### project #####
add_subdirectory(CGExperiment)

###### third_party #####
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${THIRD_PARTY}/glfw)
#file(GLOB_RECURSE THIRD_PARTY_SOURCES "third_party/*.c" "third_party/*.cpp" "third_party/*.h")

set(ALL_SOURCES ${VK_SOURCES} ${THIRD_PARTY_SOURCES})

add_executable(${PROJECT} WIN32 ${ALL_SOURCES})
target_link_libraries(${PROJECT} glfw)

#message(${VK_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${VK_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${THIRD_PARTY_SOURCES})


set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT})
set_target_properties(${PROJECT} PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug  
	CXX_STANDARD 17
)
add_definitions(-D_GLFW_WIN32 -D_CRT_SECURE_NO_WARNINGS)



if(WIN32)
   set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
endif(WIN32)

##### lib #####
target_link_libraries(${PROJECT} ${VULKAN_SDK}/lib/vulkan-1.lib)

target_link_libraries(${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/assimp-vc140-mt.lib>)
target_link_libraries(${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/FreeImaged.lib>)
target_link_libraries(${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/FreeImagePlusd.lib>)
target_link_libraries(${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/freetyped.lib>)
target_link_libraries(${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/SDL2.lib>)
target_link_libraries(${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/SDL2_mixer.lib>)
target_link_libraries(${PROJECT} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/SDL2main.lib>)



##### post build #####
# copy dlls
add_custom_command(
	TARGET ${PROJECT} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/assimp-vc140-mt.dll> $<TARGET_FILE_DIR:${PROJECT}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/smpeg2.dll> $<TARGET_FILE_DIR:${PROJECT}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/FreeImaged.dll> $<TARGET_FILE_DIR:${PROJECT}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/FreeImagePlusd.dll> $<TARGET_FILE_DIR:${PROJECT}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/SDL2.dll> $<TARGET_FILE_DIR:${PROJECT}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/SDL2_mixer.dll> $<TARGET_FILE_DIR:${PROJECT}>
)

# copy resource
add_custom_command(
	TARGET ${PROJECT} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/shaders"  $<TARGET_FILE_DIR:${PROJECT}>/shaders
)
