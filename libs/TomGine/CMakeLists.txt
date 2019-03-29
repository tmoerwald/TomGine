cmake_minimum_required(VERSION 2.8)

set(TG_NO_EVENT_HANDLING 0 CACHE BOOL "Disable threaded event handling (workaround GUI update bug Ubuntu 14.04)")
project(tomgine)

add_definitions(-DGL_GLEXT_PROTOTYPES)

# find dependent packages
IF(WIN32)
	#find_package(OpenGL REQUIRED)
	#include_directories(${OpenGL_INCLUDE_DIR})
	set(GL_LIBRARIES OpenGL32 glew32)
	set(GLU_LIBRARIES glu32)
	message(STATUS "GL_LIBRARIES: ${GL_LIBRARIES}")
	message(STATUS "GLU_LIBRARIES: ${GLU_LIBRARIES}")
	
	add_definitions(-DGLFW_EXPOSE_NATIVE_WGL)
	add_definitions(-DGLFW_EXPOSE_NATIVE_WIN32)
	
ELSE(WIN32)
	add_definitions(-DLINUX)
	find_package(PkgConfig REQUIRED)
	pkg_search_module(GL REQUIRED gl)
	pkg_search_module(GLU REQUIRED glu)
	message(STATUS "OpenGL_LIBRARIES: ${OpenGL_LIBRARIES}")
	if(NOT GL_FOUND)
		message(FATAL "libgl not found, required by TomGine")
	endif(NOT GL_FOUND)
ENDIF(WIN32)

#find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

IF(TG_NO_EVENT_HANDLING)
add_definitions(-DTG_NO_EVENT_HANDLING)
endif(TG_NO_EVENT_HANDLING)

# font selection
set(ENABLE_FTGL_FONTS 0 CACHE BOOL "Enable ftgl fonts do draw OpenGL fonts?" )
set(ENABLE_GLX_FONTS 0 CACHE BOOL "Enable glx fonts do draw OpenGL fonts?" )
IF(ENABLE_FTGL_FONTS)
#	pkg_search_module(FTGL REQUIRED ftgl)
#	if(NOT FTGL_FOUND)
#		message(FATAL "libftgl not found, required by TomGine")
#	endif(NOT FTGL_FOUND)
  set(TRUETYPE_FONT /usr/share/fonts/truetype/freefont/FreeSans.ttf CACHE PATH "A valid truetype font on your system")
  add_definitions(-DTTF_FONT="${TRUETYPE_FONT}")
#  include_directories(${FTGL_INCLUDE_DIRS})
  add_definitions(-DUSE_FTGL_FONT)
ENDIF(ENABLE_FTGL_FONTS)

IF(ENABLE_GLX_FONTS)
  set(GLX_FONT_NAME 10x20 CACHE PATH "A valid system font (shell command: xlsfonts)")
  add_definitions(-DGLX_FONT="${GLX_FONT_NAME}")
ENDIF(ENABLE_GLX_FONTS)

set(SOURCE_CPP 
	src/ply.c
	src/tgCamera.cpp
	src/tgCollission.cpp
	src/tgEngine.cpp
	src/tgError.cpp
	src/tgErrorMetric.cpp
	src/tgFont.cpp
	src/tgFrameBufferObject.cpp
	src/tgFrustum.cpp
	src/tgLight.cpp
	src/tgMaterial.cpp
	src/tgMathlib.cpp
	src/tgModel.cpp
	src/tgModelLoader.cpp
	src/tgPlot2D.cpp
	src/tgPose.cpp
	src/tgQuaternion.cpp
	src/tgRenderModel.cpp
	src/tgShapeCreator.cpp
	src/tgTexture.cpp
	src/tgTextureModel.cpp
	src/tgThread.cpp
	src/tgTimer.cpp
	src/tgTomGineThread.cpp)

set(SOURCE_H
	include/GLEvent.h
	include/GLInput.h
	include/GLWindow.h
	include/headers.h
	include/ply.h
	include/PlyStructure.h
	include/tgCamera.h
	include/tgCollission.h
	include/tgEngine.h
	include/tgError.h
	include/tgErrorMetric.h
	include/tgFont.h
	include/tgFrameBufferObject.h
	include/tgFrustum.h
	include/tgLight.h
	include/tgMaterial.h
	include/tgMathlib.h
	include/tgModel.h
	include/tgModelLoader.h
	include/tgPlot2D.h
	include/tgPose.h
	include/tgQuaternion.h
	include/tgRenderModel.h
	include/tgShapeCreator.h
	include/tgSingleton.h
	include/tgTexture.h
	include/tgTextureModel.h
	include/tgThread.h
	include/tgTimer.h
	include/tgTomGine.h
	include/tgTomGineThread.h
	include/ub3Image.h
)

if(LINUX)
  set(SOURCE_CPP ${SOURCE_CPP}
	  GLXCBWindow.cpp
	  GLXCBWindowGetEvent.cpp)
endif(LINUX)
		
if(WIN32)
	set(SOURCE_CPP ${SOURCE_CPP} GLFWWindow.cpp)
	set(BOOST_LIBRARIES libboost_thread-vc140-mt-gd-1_63)
endif(WIN32)

add_library(${PROJECT_NAME} STATIC ${SOURCE_CPP})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS} ${GL_LIBRARIES} ${GLU_LIBRARIES})
if(WIN32)
	target_link_libraries(${PROJECT_NAME} ${BOOST_LIBRARIES} glfw3)
endif(WIN32)

if(LINUX)
  target_link_libraries(${PROJECT_NAME} pthread X11 X11-xcb xcb rt)
endif(LINUX)

if(ENABLE_FTGL_FONTS)
  target_link_libraries(${PROJECT_NAME} ${FTGL_LIBRARIES})
endif(ENABLE_FTGL_FONTS)

if(WIN32)

else(WIN32)
  install(DIRECTORY DESTINATION include/${TOMGINE_INCLUDE_NAME})
  install(FILES ${SOURCE_H} DESTINATION include/${TOMGINE_INCLUDE_NAME})
  install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
endif(WIN32)
