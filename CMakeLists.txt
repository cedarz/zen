cmake_minimum_required (VERSION 3.12)

project(zen)


set(VULKAN_SDK "C:/VulkanSDK/1.2.141.2")
set(THIRD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/Includes
	#${CMAKE_CURRENT_SOURCE_DIR}/externs
	${VULKAN_SDK}/Include
	${THIRD_PARTY}/glfw/include
	${THIRD_PARTY}/glm
	${THIRD_PARTY}/glad/include
)
			

###### third_party #####
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${THIRD_PARTY}/glfw)

###### project gl460 #####
set(GL_DEMO "gldemo")
file(GLOB_RECURSE GL_SOURCES "CGExperiment/*.cpp" "*CGExperiment/.h" ${THIRD_PARTY}/glad/*.h ${THIRD_PARTY}/glad/*.c)
source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${GL_SOURCES})
add_executable(${GL_DEMO} WIN32 ${GL_SOURCES})
message(${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(${GL_DEMO} PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug  
	CXX_STANDARD 17
)
if(WIN32)
   set_target_properties(${GL_DEMO} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(${GL_DEMO} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
endif(WIN32)
target_link_libraries(${GL_DEMO} glfw)

# copy resource
add_custom_command(
	TARGET ${GL_DEMO} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/CGExperiment/shaders"  $<TARGET_FILE_DIR:${GL_DEMO}>/shaders
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/CGExperiment/textures"  $<TARGET_FILE_DIR:${GL_DEMO}>/textures
)


##### project vulkan #####
file(GLOB_RECURSE VK_SOURCES "DRender/*.cpp" "DRender/*.h")
set(ALL_VK_SOURCES ${VK_SOURCES} ${THIRD_PARTY_SOURCES})

set(VK_DEMO "vkdemo")
#file(GLOB_RECURSE THIRD_PARTY_SOURCES "third_party/*.c" "third_party/*.cpp" "third_party/*.h")
add_executable(${VK_DEMO} WIN32 ${ALL_VK_SOURCES})
target_link_libraries(${VK_DEMO} glfw)

#message(${VK_SOURCES})
source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${VK_SOURCES})
#source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${THIRD_PARTY_SOURCES})


set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT ${VK_DEMO})
set_target_properties(${VK_DEMO} PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Debug  
	CXX_STANDARD 17
)
add_definitions(-D_GLFW_WIN32 -D_CRT_SECURE_NO_WARNINGS)



if(WIN32)
   set_target_properties(${VK_DEMO} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(${VK_DEMO} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
endif(WIN32)

##### lib #####
target_link_libraries(${VK_DEMO} ${VULKAN_SDK}/lib/vulkan-1.lib)

target_link_libraries(${VK_DEMO} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/assimp-vc140-mt.lib>)
target_link_libraries(${VK_DEMO} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/FreeImaged.lib>)
target_link_libraries(${VK_DEMO} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/FreeImagePlusd.lib>)
target_link_libraries(${VK_DEMO} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/freetyped.lib>)
target_link_libraries(${VK_DEMO} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/SDL2.lib>)
target_link_libraries(${VK_DEMO} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/SDL2_mixer.lib>)
target_link_libraries(${VK_DEMO} $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/Libs/Debug/SDL2main.lib>)

##### post build #####
# copy dlls
add_custom_command(
	TARGET ${VK_DEMO} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/assimp-vc140-mt.dll> $<TARGET_FILE_DIR:${VK_DEMO}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/smpeg2.dll> $<TARGET_FILE_DIR:${VK_DEMO}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/FreeImaged.dll> $<TARGET_FILE_DIR:${VK_DEMO}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/FreeImagePlusd.dll> $<TARGET_FILE_DIR:${VK_DEMO}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/SDL2.dll> $<TARGET_FILE_DIR:${VK_DEMO}>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different $<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/Dlls/Debug/SDL2_mixer.dll> $<TARGET_FILE_DIR:${VK_DEMO}>
)

# copy resource
add_custom_command(
	TARGET ${VK_DEMO} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/DRender/shaders"  $<TARGET_FILE_DIR:${VK_DEMO}>/shaders
)
