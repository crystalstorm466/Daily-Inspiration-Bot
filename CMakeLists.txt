 cmake_minimum_required(VERSION 3.22)
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Create the executable
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# Find DPP and xlnt (Excel read/write)
find_package(DPP REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
#find_package(croncpp CONFIG REQUIRED)
# Include directories

find_library(LIBCRON_LIBRARY NAMES libcron cron PATHS /usr/lib /usr/local/lib)
if (NOT LIBCRON_LIBRARY)
  message(FATAL_ERROR "Could not find libcron")
endif()


target_include_directories(${PROJECT_NAME} PRIVATE
    ${DPP_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/extern/Scheduler
    ${CURL_INCLUDE_DIRS}
)

# Link all required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DPP_LIBRARIES}
    OpenSSL::SSL 
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
    Threads::Threads
    rt
  )

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

