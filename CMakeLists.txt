cmake_minimum_required(VERSION 3.5)
project(license_system C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find sodium library
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

include_directories(${SODIUM_INCLUDE_DIRS})
link_directories(${SODIUM_LIBRARY_DIRS})

# Source files
set(LICENSE_CLIENT_SRC license_client.c crypto_utils.c)
set(LICENSE_SERVER_SRC license_server.c crypto_utils.c activation_db.c)
set(VERIFY_SAV_SRC verify_sav.c)

# Libraries
add_library(license_client_lib SHARED ${LICENSE_CLIENT_SRC})
add_library(verify_sav_lib SHARED ${VERIFY_SAV_SRC})

# Executables
add_executable(test1 test1.c ${LICENSE_CLIENT_SRC})
add_executable(license_server ${LICENSE_SERVER_SRC})

# Link libraries
target_link_libraries(license_client_lib PRIVATE ${SODIUM_LIBRARIES})
target_link_libraries(verify_sav_lib PRIVATE ${SODIUM_LIBRARIES})
target_link_libraries(test1 PRIVATE ${SODIUM_LIBRARIES} verify_sav_lib)
target_link_libraries(license_server PRIVATE ${SODIUM_LIBRARIES})

# Runtime paths
set_target_properties(test1 PROPERTIES BUILD_RPATH "$ORIGIN")
set_target_properties(license_client_lib PROPERTIES BUILD_RPATH "$ORIGIN")
set_target_properties(license_server PROPERTIES BUILD_RPATH "$ORIGIN")
