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
    src/utils/GeoUtils.h
    src/utils/Translator.h
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
    include(GoogleTest)
    gtest_discover_tests(AirQualityTests)
endif()
