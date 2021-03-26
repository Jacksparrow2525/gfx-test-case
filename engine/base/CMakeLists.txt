set(TARGET_NAME Core)
set(CMAKE_CXX_STANDARD 17)

set(CURRENT_DIR ${COCOS_SRC_PATH}/base/)

file(GLOB_RECURSE HEADER_FILES ${CURRENT_DIR}/*.h)
file(GLOB_RECURSE SOURCE_CPP_FILES ${CURRENT_DIR}/*.cpp ${CURRENT_DIR}/*.cc)
file(GLOB_RECURSE SOURCE_C_FILES ${CURRENT_DIR}/*.c)

file(GLOB_RECURSE MATH_HEADER_FILES ${COCOS_SRC_PATH}/math/*.h ${COCOS_SRC_PATH}/math/*.inl)
file(GLOB_RECURSE MATH_SOURCE_CPP_FILES ${COCOS_SRC_PATH}/math/*.cpp ${COCOS_SRC_PATH}/math/*.cc)
file(GLOB_RECURSE MATH_SOURCE_C_FILES ${COCOS_SRC_PATH}/math/*.c)

file(GLOB PLATFORM_SUPPORT_BASE
  ${COCOS_SRC_PATH}/platform/FileUtils.h
  ${COCOS_SRC_PATH}/platform/FileUtils.cpp
)

if(COCOS_PLATFORM_WIN32)
  file(GLOB PLATFORM_SUPPORT_TARGET
    ${COCOS_SRC_PATH}/platform/win32/FileUtils-win32.h
    ${COCOS_SRC_PATH}/platform/win32/FileUtils-win32.cpp
  )
elseif(COCOS_PLATFORM_ANDROID)
  file(GLOB PLATFORM_SUPPORT_TARGET
    ${COCOS_SRC_PATH}/platform/android/FileUtils-android.h
    ${COCOS_SRC_PATH}/platform/android/FileUtils-android.cpp
  )
elseif(COCOS_PLATFORM_IOS OR COCOS_PLATFORM_OSX)
  file(GLOB PLATFORM_SUPPORT_TARGET
    ${COCOS_SRC_PATH}/platform/apple/FileUtils-apple.h
    ${COCOS_SRC_PATH}/platform/apple/FileUtils-apple.mm
  )
endif()

########### script engine ###########

set(CURRENT_DIR ${COCOS_SRC_PATH}/bindings/jswrapper)

file(GLOB SE_HEADER_FILES ${CURRENT_DIR}/*.h)
file(GLOB SE_SOURCE_CPP_FILES ${CURRENT_DIR}/*.cpp  ${CURRENT_DIR}/*.cc)
file(GLOB SE_SOURCE_C_FILES ${CURRENT_DIR}/*.c)

file(GLOB_RECURSE SE_V8_HEADER_FILES ${CURRENT_DIR}/v8/*.h)
file(GLOB_RECURSE SE_V8_SOURCE_CPP_FILES ${CURRENT_DIR}/v8/*.cpp ${CURRENT_DIR}/v8/*.cc)
file(GLOB_RECURSE SE_V8_SOURCE_C_FILES ${CURRENT_DIR}/v8/*.c)

set(SE_GENERAL_FILES
		${COCOS_SRC_PATH}/bindings/manual/jsb_classtype.cpp
		${COCOS_SRC_PATH}/bindings/manual/jsb_classtype.h
		${COCOS_SRC_PATH}/bindings/manual/jsb_conversions.cpp
		${COCOS_SRC_PATH}/bindings/manual/jsb_conversions.h
		${COCOS_SRC_PATH}/bindings/manual/jsb_helper.cpp
		${COCOS_SRC_PATH}/bindings/manual/jsb_helper.h
		${COCOS_SRC_PATH}/bindings/manual/jsb_global_init.cpp
		${COCOS_SRC_PATH}/bindings/manual/jsb_global_init.h
		${COCOS_SRC_PATH}/bindings/manual/jsb_platform.h

		${COCOS_SRC_PATH}/bindings/dop/BufferPool.cpp
		${COCOS_SRC_PATH}/bindings/dop/BufferPool.h
		${COCOS_SRC_PATH}/bindings/dop/ObjectPool.cpp
		${COCOS_SRC_PATH}/bindings/dop/ObjectPool.h
		${COCOS_SRC_PATH}/bindings/dop/jsb_dop.cpp
		${COCOS_SRC_PATH}/bindings/dop/jsb_dop.h
		${COCOS_SRC_PATH}/bindings/dop/PoolType.h
		${COCOS_SRC_PATH}/bindings/dop/BufferAllocator.h
		${COCOS_SRC_PATH}/bindings/dop/BufferAllocator.cpp

		${COCOS_SRC_PATH}/bindings/event/CustomEventTypes.h
		${COCOS_SRC_PATH}/bindings/event/EventDispatcher.cpp
		${COCOS_SRC_PATH}/bindings/event/EventDispatcher.h
		)

if(COCOS_PLATFORM_WIN32)
	set(SE_PLATFORM_SPECIFIC_FILES
			${COCOS_SRC_PATH}/bindings/manual/jsb_platfrom_win32.cpp
			)
elseif(COCOS_PLATFORM_ANDROID)
	set(SE_PLATFORM_SPECIFIC_FILES
			${COCOS_SRC_PATH}/bindings/manual/jsb_platform_android.cpp
			${COCOS_SRC_PATH}/bindings/manual/JavaScriptJavaBridge.cpp
			${COCOS_SRC_PATH}/bindings/manual/JavaScriptJavaBridge.h
			)
elseif(COCOS_PLATFORM_IOS OR COCOS_PLATFORM_OSX)
	set(SE_PLATFORM_SPECIFIC_FILES
			${COCOS_SRC_PATH}/bindings/manual/jsb_platfrom_apple.mm
			${COCOS_SRC_PATH}/bindings/manual/JavaScriptObjCBridge.h
			${COCOS_SRC_PATH}/bindings/manual/JavaScriptObjCBridge.mm
			)
	find_library(CORETEXT_LIBRARY CoreText)
	find_library(COREGRAPHICS_LIBRARY CoreGraphics)
	set(SE_PLATFORM_SPECIFIC_LIBS ${CORETEXT_LIBRARY} ${COREGRAPHICS_LIBRARY})
endif()

set(ALL_FILES
	${HEADER_FILES}
	${SOURCE_CPP_FILES}
	${SOURCE_C_FILES}
	${MATH_HEADER_FILES}
	${MATH_SOURCE_CPP_FILES}
	${MATH_SOURCE_C_FILES}
    ${CC_EXTERNAL_SOURCES}
    ${PLATFORM_SUPPORT_BASE}
    ${PLATFORM_SUPPORT_TARGET}

	${SE_HEADER_FILES}
	${SE_SOURCE_CPP_FILES}
	${SE_SOURCE_C_FILES}
	${SE_V8_HEADER_FILES}
	${SE_V8_SOURCE_CPP_FILES}
	${SE_V8_SOURCE_C_FILES}
	${SE_GENERAL_FILES}
	${SE_PLATFORM_SPECIFIC_FILES}
)

if(COCOS_UNICODE)
	add_definitions("-DUNICODE -D_UNICODE")
endif()

if (COCOS_STATIC)
	add_library(${TARGET_NAME} STATIC ${HEADER_FILES} ${ALL_FILES})
else()
	add_library(${TARGET_NAME} SHARED ${HEADER_FILES} ${ALL_FILES})
endif()

if(COCOS_PLATFORM_OSX)
	find_library(PLATFORM_SUPPORT_LIBRARY AppKit)
endif()

target_link_libraries(${TARGET_NAME} ${CC_EXTERNAL_LIBS} ${PLATFORM_SUPPORT_LIBRARY} ${SE_PLATFORM_SPECIFIC_LIBS})

set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME CCCore)
set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME_DEBUG CCCore_d)

message(STATUS "${TARGET_NAME} configuration completed.")

target_compile_definitions(${TARGET_NAME} PUBLIC
    $<$<CONFIG:Debug>:CC_DEBUG=1>
)
