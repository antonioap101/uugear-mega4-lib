cmake_minimum_required(VERSION 3.16)
project(uugear_mega4_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # bueno para .so y para -fPIC

#set(UUGEAR_INSTALL OFF CACHE BOOL "Install targets" FORCE) # desactivar instalación por defecto
option(UUGEAR_INSTALL "Install targets" OFF)
option(UUGEAR_LINK_CORE_LIB "Link plugins against core library" ON)


include(FetchContent)
include(CTest)

#-----------------------------------------------
# libusb (paquete o FetchContent)
#-----------------------------------------------
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0)
endif ()

if (NOT LIBUSB_FOUND)
    message(WARNING "libusb not found — fetching via FetchContent")
    FetchContent_Declare(
            libusb
            GIT_REPOSITORY https://github.com/libusb/libusb.git
            GIT_TAG v1.0.29
    )
    FetchContent_MakeAvailable(libusb)
    set(LIBUSB_INCLUDE_DIRS ${libusb_SOURCE_DIR}/libusb)
    set(LIBUSB_LIBRARIES usb-1.0)
else ()
    message(STATUS "libusb found in system")
endif ()

#-----------------------------------------------
# Opciones
#-----------------------------------------------
option(UUGEAR_BUILD_PLUGINS "Build bundled plugins" ON)
option(UUGEAR_INSTALL "Install targets" ON)

# Carpeta de salida para plugins en build tree
set(UUGEAR_PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")

#-----------------------------------------------
# Core library (API + Hub + Manager)
#-----------------------------------------------
add_library(uugear_mega4_lib STATIC
        src/Mega4/Mega4Hub.cpp
        src/Mega4/plugins/PluginManager.cpp
        include/UUGear/Mega4/Mega4Types.hpp
)
target_include_directories(uugear_mega4_lib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${LIBUSB_INCLUDE_DIRS}
)
target_link_libraries(uugear_mega4_lib
        PUBLIC ${LIBUSB_LIBRARIES}
)

# dlfcn para Linux (macOS no necesita -ldl)
if (UNIX AND NOT APPLE)
    target_link_libraries(uugear_mega4_lib PRIVATE dl)
endif ()

#-----------------------------------------------
# Plugins (MODULE)
#-----------------------------------------------

# --------------- Function to simplify plugin registration ---------------
function(register_uugear_plugin plugin_name plugin_src)
    message(STATUS "register_uugear_plugin(): Registering plugin with name: ${dir} from source: ${plugin_src}")
    add_library(${plugin_name} MODULE ${plugin_src})

    target_include_directories(${plugin_name}
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    set_target_properties(${plugin_name} PROPERTIES
            OUTPUT_NAME "${plugin_name}"
            LIBRARY_OUTPUT_DIRECTORY "${UUGEAR_PLUGIN_OUTPUT_DIR}"
            RUNTIME_OUTPUT_DIRECTORY "${UUGEAR_PLUGIN_OUTPUT_DIR}"
            PREFIX "" # avoid lib prefix on .so/.dll
    )

    if (UUGEAR_LINK_CORE_LIB)
        target_link_libraries(${plugin_name} PRIVATE uugear_mega4_lib)
    endif ()

    # Add the plugin target to the list
    set(UUGEAR_PLUGIN_TARGETS ${UUGEAR_PLUGIN_TARGETS} ${plugin_name} PARENT_SCOPE)

    message(STATUS "Registered plugin: ${plugin_name}")
endfunction()

# --------------- Check file existence function  ---------------
function(check_file_existence file_path)
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${file_path}")
        message(FATAL_ERROR "Required file not found: ${CMAKE_CURRENT_SOURCE_DIR}/${file_path}")
    endif ()
endfunction()

if (UUGEAR_BUILD_PLUGINS)
    #    file(GLOB PLUGIN_DIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src/Mega4/plugins" "${CMAKE_CURRENT_SOURCE_DIR}/src/Mega4/plugins/*")

    #    foreach (dir ${PLUGIN_DIRS})
    #        if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/Mega4/plugins/${dir}/${dir}.cpp")
    #            register_uugear_plugin(${dir} "src/Mega4/plugins/${dir}/${dir}.cpp")
    #        endif ()
    #    endforeach ()

    # --- Storage Plugin ---
    set(UUGEAR_STORAGE_PLUGIN_INCLUDE "include/UUGear/Mega4/plugins/StoragePlugin.hpp")
    check_file_existence(${UUGEAR_STORAGE_PLUGIN_INCLUDE})

    set(UUGEAR_STORAGE_PLUGIN_SOURCE "src/Mega4/plugins/StoragePlugin/StoragePlugin.cpp")
    check_file_existence(${UUGEAR_STORAGE_PLUGIN_SOURCE})

    register_uugear_plugin(uugear_mega4_StoragePlugin ${UUGEAR_STORAGE_PLUGIN_SOURCE})
    add_compile_definitions(UUGEAR_HAS_STORAGE_PLUGIN)
endif ()


#-----------------------------------------------
# Ejemplo
#-----------------------------------------------
add_executable(toggle_port examples/toggle_port.cpp)
target_link_libraries(toggle_port uugear_mega4_lib)

# Facilitar que tu binario busque plugins en runtime (Linux/macOS)
# $ORIGIN apunta a la carpeta del ejecutable; añadimos /plugins
# En Windows no aplica RPATH, se usa la carpeta de .dll
if (UNIX)
    set_target_properties(toggle_port PROPERTIES
            BUILD_RPATH "\$ORIGIN/plugins"
            INSTALL_RPATH "\$ORIGIN/plugins"
            SKIP_BUILD_RPATH FALSE
            SKIP_INSTALL_RPATH FALSE
    )
endif ()

#-----------------------------------------------
# Tests (GoogleTest)
#-----------------------------------------------
find_package(GTest QUIET)

if (NOT GTest_FOUND)
    message(WARNING "GoogleTest not found in system — fetching via FetchContent")
    include(FetchContent)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
else ()
    message(STATUS "GoogleTest found in system")
endif ()

enable_testing()

add_executable(tests
        tests/test_Mega4Hub.cpp
        tests/test_main.cpp
        tests/plugins/test_Mega4Hub_Plugins.cpp
)

# Asegurarse de que los plugins se construyan antes de los tests
add_dependencies(tests ${UUGEAR_PLUGIN_TARGETS})

target_link_libraries(tests
        uugear_mega4_lib
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)


#-----------------------------------------------
# Install (custom layout)
#-----------------------------------------------
if (UUGEAR_INSTALL)
    include(GNUInstallDirs)

    # Base install paths
    set(UUGEAR_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/UUGear/Mega4")
    set(UUGEAR_LIB_DIR "${CMAKE_INSTALL_LIBDIR}/uugear/mega4")
    set(UUGEAR_PLUGIN_DIR "${UUGEAR_LIB_DIR}/plugins")

    install(TARGETS uugear_mega4_lib
            ARCHIVE DESTINATION ${UUGEAR_LIB_DIR}
            LIBRARY DESTINATION ${UUGEAR_LIB_DIR}
    )


    install(DIRECTORY include/UUGear/Mega4/
            DESTINATION ${UUGEAR_INCLUDE_DIR}
            FILES_MATCHING PATTERN "*.hpp"
            PATTERN "plugins/*" EXCLUDE
    )

    #    if (UUGEAR_BUILD_PLUGINS)
    #        install(DIRECTORY ${UUGEAR_PLUGIN_OUTPUT_DIR}/
    #                DESTINATION ${UUGEAR_PLUGIN_DIR}
    #                FILES_MATCHING PATTERN "*.so" PATTERN "*.dll" PATTERN "*.dylib"
    #        )
    #    endif ()

    if (TARGET uugear_mega4_StoragePlugin)
        install(FILES
                ${UUGEAR_STORAGE_PLUGIN_INCLUDE}
                DESTINATION ${UUGEAR_INCLUDE_DIR}/plugins
        )
        install(TARGETS uugear_mega4_StoragePlugin
                LIBRARY DESTINATION ${UUGEAR_PLUGIN_DIR}
                RUNTIME DESTINATION ${UUGEAR_PLUGIN_DIR}
        )
    endif ()

    install(TARGETS toggle_port
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    message(STATUS "Library install dir: ${UUGEAR_LIB_DIR}")
    message(STATUS "Plugin install dir: ${UUGEAR_PLUGIN_DIR}")
endif ()


