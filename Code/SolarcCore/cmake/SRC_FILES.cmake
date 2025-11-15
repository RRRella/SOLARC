set(${PROJECT_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${PROJECT_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${PROJECT_NAME}_SRC 

${${PROJECT_NAME}_SRC_DIR}/SolarcApp.cpp

${${PROJECT_NAME}_SRC_DIR}/Window/Window.cpp
${${PROJECT_NAME}_SRC_DIR}/Window/WindowPlatform.cpp
${${PROJECT_NAME}_SRC_DIR}/Window/WindowsWindowPlatform.cpp
${${PROJECT_NAME}_SRC_DIR}/Window/WindowContext.cpp
${${PROJECT_NAME}_SRC_DIR}/Window/WindowContextPlatform.cpp
${${PROJECT_NAME}_SRC_DIR}/Window/WindowsWindowContextPlatform.cpp



${${PROJECT_NAME}_SRC_DIR}/Event/Event.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/WindowEvent.cpp


${${PROJECT_NAME}_SRC_DIR}/Utility/CompileTimeUtil.cpp
${${PROJECT_NAME}_SRC_DIR}/Utility/FileSystemUtil.cpp

${${PROJECT_NAME}_SRC_DIR}/MT/ThreadPool.cpp

)

set(${PROJECT_NAME}_HDRS

${${PROJECT_NAME}_INC_DIR}/SolarcApp.h

${${PROJECT_NAME}_INC_DIR}/Window/Window.h
${${PROJECT_NAME}_INC_DIR}/Window/WindowPlatform.h
${${PROJECT_NAME}_INC_DIR}/Window/WindowsWindowPlatform.h
${${PROJECT_NAME}_INC_DIR}/Window/WindowContext.h
${${PROJECT_NAME}_INC_DIR}/Window/WindowContextPlatform.h
${${PROJECT_NAME}_INC_DIR}/Window/WindowsWindowContextPlatform.h

${${PROJECT_NAME}_INC_DIR}/Event/Event.h
${${PROJECT_NAME}_INC_DIR}/Event/ApplicationEvent.h
${${PROJECT_NAME}_INC_DIR}/Event/WindowEvent.h
${${PROJECT_NAME}_INC_DIR}/Event/ObserverBus.h
${${PROJECT_NAME}_INC_DIR}/Event/EventBus.h
${${PROJECT_NAME}_INC_DIR}/Event/EventListener.h
${${PROJECT_NAME}_INC_DIR}/Event/EventProducer.h

${${PROJECT_NAME}_INC_DIR}/Preprocessor/API.h

${${PROJECT_NAME}_INC_DIR}/Utility/UUID.h
${${PROJECT_NAME}_INC_DIR}/Utility/CompileTimeUtil.h
${${PROJECT_NAME}_INC_DIR}/Utility/FileSystemUtil.h

${${PROJECT_NAME}_INC_DIR}/MT/ThreadPool.h
${${PROJECT_NAME}_INC_DIR}/MT/ThreadSafeQueue.h

)