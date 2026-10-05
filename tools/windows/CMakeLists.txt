cmake_minimum_required(VERSION 3.25)
project(virtual_item_diagnostic LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Build with MSVC for Win32")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(/W4 /WX /EHsc /permissive-)
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
add_executable(sample_virtual_item_state sample_virtual_item_state.cpp)
target_link_libraries(sample_virtual_item_state PRIVATE bcrypt)
add_executable(item_predicate_fixture item_predicate_fixture.cpp)
target_link_options(item_predicate_fixture PRIVATE /OPT:NOICF)
add_executable(trace_map_selection trace_map_selection.cpp raw_event_recorder.cpp)
target_compile_options(trace_map_selection PRIVATE /guard:cf /sdl)
target_link_options(trace_map_selection PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT /OPT:NOICF
                    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/trace_map_selection.map")
target_link_libraries(trace_map_selection PRIVATE bcrypt)
add_executable(map_selection_fixture map_selection_fixture.cpp)
target_link_options(map_selection_fixture PRIVATE /OPT:NOICF)

add_executable(recorder_preflight recorder_preflight.cpp)
set_target_properties(recorder_preflight PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(recorder_preflight PRIVATE _WIN32_WINNT=0x0A00)
target_compile_options(recorder_preflight PRIVATE /guard:cf /sdl)
target_link_options(recorder_preflight PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT /OPT:NOICF
                    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/recorder_preflight.map")
target_link_libraries(recorder_preflight PRIVATE bcrypt)

add_executable(raw_recorder_check raw_recorder_check.cpp raw_event_recorder.cpp)
set_target_properties(raw_recorder_check PROPERTIES CXX_EXTENSIONS OFF)
target_compile_definitions(raw_recorder_check PRIVATE _WIN32_WINNT=0x0A00)
target_compile_options(raw_recorder_check PRIVATE /guard:cf /sdl)
target_link_options(raw_recorder_check PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT /OPT:NOICF
                    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/raw_recorder_check.map")
target_link_libraries(raw_recorder_check PRIVATE bcrypt)

add_executable(observer_diagnostic_check observer_diagnostic_check.cpp
               observer_diagnostic.cpp observer_diagnostic_tests.cpp)
set_target_properties(observer_diagnostic_check PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(observer_diagnostic_check PRIVATE /guard:cf /sdl)
target_link_options(observer_diagnostic_check PRIVATE /guard:cf /DYNAMICBASE /NXCOMPAT /OPT:NOICF
                    "/MAP:${CMAKE_CURRENT_BINARY_DIR}/observer_diagnostic_check.map")
