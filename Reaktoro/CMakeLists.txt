# Require a certain version of cmake
cmake_minimum_required(VERSION 3.6)

# Recursively collect all header files from the current directory
file(GLOB_RECURSE HEADER_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.hpp *.hxx)

# Recursively collect all source files from the current directory
file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

# The name of the shared and static libraries
set(REAKTORO_SHARED_LIB ${PROJECT_NAME}${SUFFIX_SHARED_LIBS})
set(REAKTORO_STATIC_LIB ${PROJECT_NAME}${SUFFIX_STATIC_LIBS})

set(YAMLCPP_LIB yaml-cpp)
if(MSVC)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(YAMLCPP_LIB libyaml-cppmdd)
    else()
        set(YAMLCPP_LIB libyaml-cppmd)
    endif()
endif()

# Set the list of names of the third-party targets and libraries
set(THIRDPARTY_TARGETS PUGIXML YAMLCPP MINIZ CVODE)
set(THIRDPARTY_LIBS pugixml ${YAMLCPP_LIB} miniz sundials_cvode sundials_nvecserial)

if(LINK_PHREEQC)
    set(THIRDPARTY_TARGETS ${THIRDPARTY_TARGETS} PHREEQC)
    set(THIRDPARTY_LIBS ${THIRDPARTY_LIBS} phreeqc)
endif()

if(LINK_GEMS)
    set(THIRDPARTY_TARGETS ${THIRDPARTY_TARGETS} GEMS)
    set(THIRDPARTY_LIBS ${THIRDPARTY_LIBS} gems)
endif()

# Compile Reaktoro into object files
add_library(ReaktoroObject OBJECT ${HEADER_FILES} ${SOURCE_FILES})

# Allow cotire to add Eigen header files to the generated prefix header file
set_target_properties(ReaktoroObject PROPERTIES
    COTIRE_PREFIX_HEADER_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/Reaktoro/Math/Eigen")

# Apply cotire to ReaktoroObject target to improve compilation time
cotire(ReaktoroObject)

# Specify the third-party dependencies of Reaktoro to ensure proper build sequence
add_dependencies(ReaktoroObject ${THIRDPARTY_TARGETS})

# Check if a shared library for Reaktoro must be built
if(BUILD_SHARED_LIBS)
    if(MSVC)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    endif()
    add_library(ReaktoroShared SHARED $<TARGET_OBJECTS:ReaktoroObject>)
    target_link_libraries(ReaktoroShared ${THIRDPARTY_LIBS})
    set_target_properties(ReaktoroShared PROPERTIES OUTPUT_NAME Reaktoro)
    install(TARGETS ReaktoroShared
        ARCHIVE
            DESTINATION lib
            COMPONENT libraries
        LIBRARY
            DESTINATION lib
            COMPONENT libraries
        RUNTIME
            DESTINATION bin
            COMPONENT libraries
    )
endif()

# Check if a static library for Reaktoro must be built
if(BUILD_STATIC_LIBS)
    add_library(ReaktoroStatic STATIC $<TARGET_OBJECTS:ReaktoroObject>)
    target_link_libraries(ReaktoroStatic ${THIRDPARTY_LIBS})
    set_target_properties(ReaktoroStatic PROPERTIES OUTPUT_NAME Reaktoro)
    install(TARGETS ReaktoroStatic DESTINATION "lib" COMPONENT libraries)
endif()

# Create an install target for Reaktoro header files
install(DIRECTORY ${REAKTORO_SOURCE_DIR}
    DESTINATION "include" COMPONENT headers
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.hxx")

# Create an install target for Eigen header files
install(DIRECTORY ${REAKTORO_SOURCE_DIR}/Math/Eigen
    DESTINATION "include/Reaktoro/Math" COMPONENT headers)
