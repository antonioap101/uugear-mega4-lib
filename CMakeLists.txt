cmake_minimum_required(VERSION 3.16)
project(uugear_mega4_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # bueno para .so y para -fPIC

#-----------------------------------------------
# Opciones
#-----------------------------------------------
option(UUGEAR_INSTALL "Enable installation targets" OFF)
option(UUGEAR_LINK_CORE_LIB "Link plugins against core library" ON)
option(UUGEAR_BUILD_PLUGINS "Build bundled plugins" ON)

include(FetchContent)
include(CTest)

# -----------------------------------------------
# detect_processor_count(<VAR_NAME>)
# Detects the number of available logical processors. Sets the given variable name to the detected count, or defaults to 4 if detection fails.
# -----------------------------------------------
function(detect_processor_count VAR_NAME)
    include(ProcessorCount)
    ProcessorCount(_DETECTED_COUNT)

    if(NOT _DETECTED_COUNT OR _DETECTED_COUNT EQUAL 0)
        set(${VAR_NAME} 4 PARENT_SCOPE)
        message(WARNING "Could not determine processor count, defaulting to 4")
    else()
        set(${VAR_NAME} ${_DETECTED_COUNT} PARENT_SCOPE)
        message(STATUS "Processor count detected: ${_DETECTED_COUNT}")
    endif()
endfunction()

#-----------------------------------------------
# libusb (paquete o FetchContent)
#-----------------------------------------------
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(LIBUSB libusb-1.0)
endif ()

if (NOT LIBUSB_FOUND)
    include(ExternalProject)

    message(WARNING "libusb not found — building from source with Autotools")

    set(LIBUSB_PREFIX "${CMAKE_BINARY_DIR}/_deps/libusb")

    # Llama a la función para obtener el número de procesadores
    detect_processor_count(PROCESSOR_COUNT)
    ExternalProject_Add(libusb_project
            GIT_REPOSITORY https://github.com/libusb/libusb.git
            GIT_TAG v1.0.29
            PREFIX ${LIBUSB_PREFIX}
            CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${LIBUSB_PREFIX}
            BUILD_COMMAND make -j${PROCESSOR_COUNT}
            INSTALL_COMMAND make install
            BUILD_IN_SOURCE 1              # build inside the source tree
            UPDATE_DISCONNECTED 1          # Don't re-download the repo each time
            LOG_CONFIGURE ON               # Show the log of ./configure
            LOG_BUILD ON                   # Show the log of make
            LOG_INSTALL ON                 # Show the log of make install
    )

    # Paths for consumers
    set(LIBUSB_INCLUDE_DIRS ${LIBUSB_PREFIX}/include/libusb-1.0)
    set(LIBUSB_LIBRARIES ${LIBUSB_PREFIX}/lib/libusb-1.0.a)
else ()
    message(STATUS "libusb found in system")
endif ()

#-----------------------------------------------
# libmount (mount/umount management)
#-----------------------------------------------
find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(LIBMOUNT mount)
endif ()

#-----------------------------------------------
# libmount (fallback build via Autotools)
#-----------------------------------------------
if (NOT LIBMOUNT_FOUND)
    include(ExternalProject)

    message(WARNING "libmount not found — building util-linux (libmount) with Autotools")

    set(UTIL_LINUX_PREFIX "${CMAKE_BINARY_DIR}/_deps/util-linux")

    # Llama a la función para obtener el número de procesadores
    detect_processor_count(PROCESSOR_COUNT)
    ExternalProject_Add(util_linux_project
            GIT_REPOSITORY https://github.com/util-linux/util-linux.git
            GIT_TAG v2.41.2
            PREFIX ${UTIL_LINUX_PREFIX}
            CONFIGURE_COMMAND ./autogen.sh &&
            ./configure --disable-all-programs --enable-libmount --enable-libblkid --disable-nls --without-python --prefix=${UTIL_LINUX_PREFIX}
            BUILD_COMMAND make -j${PROCESSOR_COUNT}
            INSTALL_COMMAND make install
            BUILD_IN_SOURCE 1              # build inside the source tree
            UPDATE_DISCONNECTED 1          # Don't re-download the repo each time
            LOG_CONFIGURE ON               # Show the log of ./configure
            LOG_BUILD ON                   # Show the log of make
            LOG_INSTALL ON                 # Show the log of make install
    )

    set(LIBMOUNT_INCLUDE_DIRS ${UTIL_LINUX_PREFIX}/include)
    set(LIBMOUNT_LIBRARIES ${UTIL_LINUX_PREFIX}/lib/libmount.a)
else ()
    message(STATUS "libmount found in system")
endif ()


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
# Carpeta de salida para plugins en build tree
set(UUGEAR_PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")

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
# Enlazar libmount al plugin StoragePlugin
#-----------------------------------------------
if (TARGET uugear_mega4_StoragePlugin)
    # Añadir include y librerías si se han definido (por pkg-config o build externo)
    if (LIBMOUNT_INCLUDE_DIRS)
        message(STATUS "Adding libmount include dirs to StoragePlugin: ${LIBMOUNT_INCLUDE_DIRS}")
        target_include_directories(uugear_mega4_StoragePlugin PRIVATE ${LIBMOUNT_INCLUDE_DIRS})
    else ()
        message(WARNING "LIBMOUNT_INCLUDE_DIRS is not defined; libmount headers may not be found")
    endif ()

    if (LIBMOUNT_LIBRARIES)
        message(STATUS "Linking libmount libraries to StoragePlugin: ${LIBMOUNT_LIBRARIES}")
        target_link_libraries(uugear_mega4_StoragePlugin PRIVATE ${LIBMOUNT_LIBRARIES})
    else ()
        message(WARNING "LIBMOUNT_LIBRARIES is not defined; libmount may not be linked correctly")
    endif ()

    # Si se construyó libmount con ExternalProject_Add, asegurar orden correcto
    if (TARGET util_linux_project)
        message(STATUS "Adding dependency on util_linux_project for StoragePlugin")
        add_dependencies(uugear_mega4_StoragePlugin util_linux_project)
    else ()
        message(STATUS "No dependency on util_linux_project needed for StoragePlugin")
    endif ()

    if (TARGET libusb_project)
        message(STATUS "Adding dependency on libusb_project for StoragePlugin")
        add_dependencies(uugear_mega4_StoragePlugin libusb_project)
    else ()
        message(STATUS "No dependency on libusb_project needed for StoragePlugin")
    endif ()
endif ()

#-----------------------------------------------
# Build aggregate targets (for convenience)
#-----------------------------------------------

# 1. Target “all plugins” to build all plugins
if (UUGEAR_BUILD_PLUGINS AND UUGEAR_PLUGIN_TARGETS)
    add_custom_target(uugear_all_plugins ALL
            DEPENDS ${UUGEAR_PLUGIN_TARGETS}
            COMMENT "Building all UUGear Mega4 plugins"
    )
endif()

# 2. Target “full build” (core library + all plugins)
add_custom_target(uugear_full_build
        DEPENDS uugear_mega4_lib ${UUGEAR_PLUGIN_TARGETS}
        COMMENT "Building core library + all plugins"
)

#-----------------------------------------------
# Ejemplo
#-----------------------------------------------
add_executable(toggle_port examples/toggle_port.cpp)
target_link_libraries(toggle_port uugear_mega4_lib)


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

    message(STATUS "Library install dir: ${UUGEAR_LIB_DIR}")
    message(STATUS "Plugin install dir: ${UUGEAR_PLUGIN_DIR}")
endif ()


