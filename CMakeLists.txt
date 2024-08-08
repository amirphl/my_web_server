cmake_minimum_required(VERSION 3.8)
project(my_web_server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Change path from /src if needed, or add more directories
file(GLOB_RECURSE sources
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        )

add_executable(my_app ${sources})

# Add more include directories if needed
target_include_directories(my_app PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/third_party/mysql-connector-c++-9.0.0-macos14-arm64/include
)

# Set the path to the MySQL Connector/C++ libraries
link_directories(${CMAKE_SOURCE_DIR}/third_party/mysql-connector-c++-9.0.0-macos14-arm64/lib64)

# find_package(MySQLConnectorCpp REQUIRED)
# target_link_libraries(my_app PRIVATE MySQL::MySQLConnectorCpp)

# Link the MySQL Connector/C++ library
target_link_libraries(my_app
  mysqlcppconn
  mysqlcppconn-static
)

# Add precompiler definitions like that:
# add_definitions(-DSOME_DEFINITION)


# If you have precompiled headers you can add them like this
# target_precompiled_headers(my_app PRIVATE "${CMAKE_SOURCE_DIR}/src/pch.h")
