CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(BACKGROUND_SPLIT)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

FIND_PACKAGE(OpenCV REQUIRED)
LINK_DIRECTORIES(${PROJECT_BINARY_DIR}/lib)

INCLUDE_DIRECTORIES(./)
INCLUDE_DIRECTORIES(./src)

# BGDifference，高斯背景差分法动态链接库生成
SET(LIB_BGDIFF_SOURCE
	./src/BGDifference/BGDifference.h
	./src/BGDifference/BGDifference.cpp)
ADD_LIBRARY(BGDiff SHARED ${LIB_BGDIFF_SOURCE})
TARGET_LINK_LIBRARIES(BGDiff
	${OpenCV_LIBS})

# ViBe动态链接库生成
SET(LIB_VIBE_SOURCE
	./src/ViBe/Vibe.h
	./src/ViBe/Vibe.cpp)
ADD_LIBRARY(vibe SHARED ${LIB_VIBE_SOURCE})
TARGET_LINK_LIBRARIES(vibe
	${OpenCV_LIBS})

# ViBe+动态链接库生成
SET(LIB_VIBEPLUS_SOURCE
	./src/ViBe+/ViBePlus.h
	./src/ViBe+/ViBePlusMacro.h
	./src/ViBe+/ViBePlus.cpp)
ADD_LIBRARY(vibe+ SHARED ${LIB_VIBEPLUS_SOURCE})
TARGET_LINK_LIBRARIES(vibe+
	${OpenCV_LIBS})

# 生成FrameDifference测试程序
ADD_EXECUTABLE(FrameDifference_test ./src/FramesDifference/main.cpp)
TARGET_LINK_LIBRARIES(FrameDifference_test
	${OpenCV_LIBS})


# 生成BGDifference测试程序
SET(LIB_BGDIFF BGDiff)
ADD_EXECUTABLE(BGDiff_test ./src/BGDifference/main.cpp)
TARGET_LINK_LIBRARIES(BGDiff_test
	${LIB_BGDIFF})

# 生成ViBe测试程序
SET(LIB_VIBE vibe)
ADD_EXECUTABLE(vibe_test ./src/ViBe/main.cpp)
TARGET_LINK_LIBRARIES(vibe_test
	${LIB_VIBE})

# 生成ViBe+测试程序
SET(LIB_VIBEPLUS vibe+)
ADD_EXECUTABLE(vibe+_test ./src/ViBe+/main.cpp)
TARGET_LINK_LIBRARIES(vibe+_test
	${LIB_VIBEPLUS})
