cmake_minimum_required(VERSION 3.20)
project(ip-notifier VERSION 2.0.0 LANGUAGES CXX)

# ── Language standard ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow Objective-C++ on Apple
if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
endif()

# ── Dependencies (FetchContent) ──────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(httplib)

# ── Source files ─────────────────────────────────────────────────────────────
set(COMMON_SRC
    src/main.cpp
    src/config.cpp
    src/checker.cpp
    src/notifier.cpp
)

if(WIN32)
    list(APPEND COMMON_SRC src/tray_win32.cpp src/gui_win32.cpp)
elseif(APPLE)
    list(APPEND COMMON_SRC src/tray_macos.mm src/gui_macos.mm)
elseif(UNIX)
    list(APPEND COMMON_SRC src/tray_linux.cpp src/gui_linux.cpp)
endif()

# ── Executable ───────────────────────────────────────────────────────────────
if(WIN32)
    # WIN32 = hide console window (WinMain subsystem)
    add_executable(ip-notifier WIN32 ${COMMON_SRC})
else()
    add_executable(ip-notifier ${COMMON_SRC})
endif()

# Output all platforms to build/out/ for predictable CI artifact paths
set_target_properties(ip-notifier PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY                "${CMAKE_BINARY_DIR}/out"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/out"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/out"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/out"
)

target_include_directories(ip-notifier PRIVATE src)

# ── Common link libraries ─────────────────────────────────────────────────────
target_link_libraries(ip-notifier PRIVATE
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# ── Platform-specific configuration ──────────────────────────────────────────

if(WIN32)
    target_link_libraries(ip-notifier PRIVATE
        shell32 user32 gdi32 comctl32 winhttp
    )
    target_compile_definitions(ip-notifier PRIVATE UNICODE _UNICODE)

elseif(APPLE)
    target_compile_definitions(ip-notifier PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(ip-notifier PRIVATE
        OpenSSL::SSL OpenSSL::Crypto
        "-framework Foundation"
        "-framework AppKit"
        "-framework UserNotifications"
    )

elseif(UNIX)
    target_compile_definitions(ip-notifier PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    find_package(PkgConfig REQUIRED)
    find_package(OpenSSL REQUIRED)

    pkg_check_modules(GTK3          REQUIRED gtk+-3.0)
    pkg_check_modules(APPINDICATOR  REQUIRED appindicator3-0.1)
    pkg_check_modules(LIBNOTIFY     REQUIRED libnotify)

    target_include_directories(ip-notifier PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${APPINDICATOR_INCLUDE_DIRS}
        ${LIBNOTIFY_INCLUDE_DIRS}
    )
    target_link_libraries(ip-notifier PRIVATE
        OpenSSL::SSL OpenSSL::Crypto
        ${GTK3_LIBRARIES}
        ${APPINDICATOR_LIBRARIES}
        ${LIBNOTIFY_LIBRARIES}
    )
    target_compile_options(ip-notifier PRIVATE
        ${GTK3_CFLAGS_OTHER}
        ${APPINDICATOR_CFLAGS_OTHER}
    )
endif()
