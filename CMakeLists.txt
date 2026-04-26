cmake_minimum_required(VERSION 3.10)

project(license_system C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 使用 GCC
set(CMAKE_C_COMPILER gcc)

# 关闭 PIE（全局）
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

# ===== 编译选项（只影响 compile 阶段）=====
add_compile_options(
    -O0
    -fno-stack-protector
    -fvisibility=hidden
    -ffunction-sections
    -fdata-sections
    -fno-unroll-loops
)

# ===== 查找 libsodium =====
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# 注意：静态链接时要用静态 flags
pkg_check_modules(SODIUM_STATIC REQUIRED libsodium)

include_directories(${SODIUM_STATIC_INCLUDE_DIRS})
link_directories(${SODIUM_STATIC_LIBRARY_DIRS})

# 强制静态库
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

# ===== 源码 =====
set(LICENSE_CLIENT_SRC license_client.c crypto_utils.c)
set(LICENSE_SERVER_SRC license_server.c crypto_utils.c activation_db.c)
set(VERIFY_SAV_SRC verify_sav.c)

add_executable(test1 test1.c ${LICENSE_CLIENT_SRC} ${VERIFY_SAV_SRC})
add_executable(license_server ${LICENSE_SERVER_SRC})

# ===== 链接库 =====
target_link_libraries(test1 PRIVATE ${SODIUM_STATIC_LIBRARIES})
target_link_libraries(license_server PRIVATE ${SODIUM_STATIC_LIBRARIES})

# ===== 链接选项（只影响 link 阶段）=====
target_link_options(test1 PRIVATE
    -no-pie
    -z execstack
    -Wl,--gc-sections
    -static
)

target_link_options(license_server PRIVATE
    -no-pie
    -z execstack
    -Wl,--gc-sections
    -static
)

# # ===== strip（保留一个就够）=====
# add_custom_command(TARGET test1 POST_BUILD
#     COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:test1>
# )

# add_custom_command(TARGET license_server POST_BUILD
#     COMMAND ${CMAKE_STRIP} --strip-all $<TARGET_FILE:license_server>
# )