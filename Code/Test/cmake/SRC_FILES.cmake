set(${PROJECT_NAME}_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(${PROJECT_NAME}_INC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(${PROJECT_NAME}_SRC 

${${PROJECT_NAME}_SRC_DIR}/Event/ObserverBusTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Event/EventSystemStressTest.cpp

${${PROJECT_NAME}_SRC_DIR}/MT/JobSystemTest.cpp

${${PROJECT_NAME}_SRC_DIR}/Window/WindowTest.cpp

${${PROJECT_NAME}_SRC_DIR}/Rendering/RHIIntegrationTestFixture.h
${${PROJECT_NAME}_SRC_DIR}/Rendering/RHIIntegrationTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Rendering/RHIFrameCycleIntegrationTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Rendering/RHIVsyncIntegrationTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Rendering/RHIWindowStateIntegrationTest.cpp
${${PROJECT_NAME}_SRC_DIR}/Rendering/RHIGPUSyncIntegrationTest.cpp

${${PROJECT_NAME}_SRC_DIR}/main.cpp
${${PROJECT_NAME}_SRC_DIR}/FreqUsedSymbolsOfTesting.h

)
