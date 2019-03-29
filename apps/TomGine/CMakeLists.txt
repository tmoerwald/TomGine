set(APPS_TOMGINE 0 CACHE BOOL "Example apps for TomGine." )
IF(APPS_TOMGINE)

message(STATUS "TOMGINE_RESOURCES is: " ${CMAKE_CURRENT_SOURCE_DIR}/Resources/)
add_definitions(-DTOMGINE_RESOURCES="${CMAKE_CURRENT_SOURCE_DIR}/Resources/")

add_executable(simple main_tomgine.cpp)
target_link_libraries(simple tomgine)

add_executable(tgPlyViewer tgPlyViewer.cpp)
target_link_libraries(tgPlyViewer tomgine)

add_executable(tomgineoszi main_tomgine_oszi.cpp)
target_link_libraries(tomgineoszi tomgine)

add_executable(tomgineWrapper main_tgwrapper.cpp)
target_link_libraries(tomgineWrapper tomgine)

add_executable(tomgine_remove_dublicates tomgine_remove_dublicates.cpp)
target_link_libraries(tomgine_remove_dublicates tomgine)

ENDIF(APPS_TOMGINE)
