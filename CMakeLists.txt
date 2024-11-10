cmake_minimum_required(VERSION 3.15)
project(MyWebServer VERSION 1.0)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set path to MySQL Connector
set(MySQL_Connector_DIR "${CMAKE_SOURCE_DIR}/third_party/mysql-connector-c++-9.0.0-macos14-arm64")

# Add MySQL library path
link_directories(${MySQL_Connector_DIR}/lib64)

# Collect source files
file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cc"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Add include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${MySQL_Connector_DIR}/include
)

# Link MySQL Connector libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        "${MySQL_Connector_DIR}/lib64/libmysqlcppconn.dylib"
)

# Set RPATH for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    INSTALL_RPATH "${MySQL_Connector_DIR}/lib64"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add compiler flags if needed
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
