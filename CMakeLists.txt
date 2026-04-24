cmake_minimum_required(VERSION 3.16)
project(AirQualityMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Network Charts Concurrent)

# nlohmann/json
include(FetchContent)

# Budujemy zaleznosci jako statyczne biblioteki, aby uniknac problemow
# z linkowaniem DLL (szczegolnie w konfiguracjach Debug + MinGW w Qt Creator).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
# Dodatkowo wylaczamy alt-svc w curl (nieuzywane przez aplikacje),
# co eliminuje sporadyczne konflikty linkera na symbolach Curl_altsvc_*.
set(CURL_DISABLE_ALTSVC ON CACHE BOOL "Disable curl alt-svc feature" FORCE)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# CPR - HTTP
FetchContent_Declare(cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.11.1
)

FetchContent_MakeAvailable(json cpr)

set(SOURCES
    src/main.cpp
    src/api/ApiClient.cpp
    src/database/DatabaseManager.cpp
    src/analysis/DataAnalyzer.cpp
    src/gui/MainWindow.cpp
    src/gui/ChartWidget.cpp
    src/gui/MapWidget.cpp
)

set(HEADERS
    src/models/Station.h
    src/models/Sensor.h
    src/models/MeasurementData.h
    src/models/AirQualityIndex.h
    src/api/ApiClient.h
    src/database/DatabaseManager.h
    src/analysis/DataAnalyzer.h
    src/gui/MainWindow.h
    src/gui/ChartWidget.h
    src/gui/MapWidget.h
    src/utils/GeoUtils.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets Qt6::Network Qt6::Charts Qt6::Concurrent
    nlohmann_json::nlohmann_json cpr::cpr
)

# ---- Testy jednostkowe (klasa DataAnalyzer) ----
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(AirQualityTests
        tests/test_analyzer.cpp
        src/analysis/DataAnalyzer.cpp
    )
    target_include_directories(AirQualityTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(AirQualityTests PRIVATE
        GTest::gtest GTest::gtest_main
        nlohmann_json::nlohmann_json
    )
    add_test(NAME AirQualityTests COMMAND AirQualityTests)
    if(WIN32)
        get_filename_component(CXX_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set_tests_properties(AirQualityTests PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${CXX_COMPILER_BIN_DIR}"
        )
    endif()
endif()
