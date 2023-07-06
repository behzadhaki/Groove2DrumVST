project(Groove2DrumFX VERSION 0.0.1)

set(BaseTargetName Groove2DrumFX)

# --------------------------------------------
#---- Torch Installation ---------------------

# Download libtorch if it's not installed
find_package(Torch)
if(NOT Torch_FOUND)
    set(TORCH_VERSION 2.0.0)
    set(LIBTORCH_INSTALL_DIR "C:/libtorch-${TORCH_VERSION}")
    # if directory exists, delete it
    if(EXISTS ${LIBTORCH_INSTALL_DIR})
        message(STATUS "Deleting existing libtorch directory...")
        file(REMOVE_RECURSE ${LIBTORCH_INSTALL_DIR})
    endif()

    # Check if we're running on Windows
    if(MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "Downloading libtorch (DEBUG) for Windows...")
            set(TORCH_FILE libtorch-win-shared-with-deps-debug-${TORCH_VERSION}%2Bcpu.zip)
        else()
            message(STATUS "Downloading libtorch (RELEASE) for Windows...")
            set(TORCH_FILE libtorch-win-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip)
        endif()

        file(DOWNLOAD
                https://download.pytorch.org/libtorch/cpu/${TORCH_FILE}
                ${CMAKE_BINARY_DIR}/${TORCH_FILE}
                SHOW_PROGRESS
                )

        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_BINARY_DIR}/${TORCH_FILE}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                )

        # Move the extracted libtorch directory to the desired location
        file(RENAME ${CMAKE_BINARY_DIR}/libtorch ${LIBTORCH_INSTALL_DIR})

        set(Torch_DIR ${LIBTORCH_INSTALL_DIR}/share/cmake/Torch)
        find_package(Torch REQUIRED)

        # Add Torch to the include directories
        include_directories(${TORCH_INCLUDE_DIRS})

        # Add Torch libraries to the linker flags
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TORCH_LINKER_FLAGS}")

    else()
        message(STATUS "Downloading libtorch for MacOS...")
        set(TORCH_FILE libtorch-cxx11-abi-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip)
        file(DOWNLOAD
                https://download.pytorch.org/libtorch/cpu/libtorch-macos-${TORCH_VERSION}.zip
                ${CMAKE_BINARY_DIR}/${TORCH_FILE}
                SHOW_PROGRESS
                )

        execute_process(COMMAND unzip ${TORCH_FILE}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                )

    endif()

    set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/libtorch)
endif()

# ----------------------------------------------

juce_add_plugin("${BaseTargetName}"
        # VERSION ...                               # Set this if the plugin version is different from the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "BehzadHaki"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
        PLUGIN_MANUFACTURER_CODE Juce
        PLUGIN_CODE MFxt
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "Groove2Drum")

target_sources(${BaseTargetName} PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Model/MonotonicV1ModelAPI.cpp
        Source/Model/VAE_V1ModelAPI.cpp
        Source/gui/CustomGuiTextEditors.cpp
        Source/ProcessorThreads/GrooveThread.cpp
        Source/ProcessorThreads/ModelThread.cpp)

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

if (MSVC)
    file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(TARGET ${BaseTargetName}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TORCH_DLLS}
            $<TARGET_FILE_DIR:${BaseTargetName}>)
endif()

target_link_libraries(${BaseTargetName} PRIVATE
        shared_plugin_helpers
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags
        ${TORCH_LIBRARIES}
        )

# Add libtorch headers to the include directories
target_include_directories(${BaseTargetName} PRIVATE "${LIBTORCH_INSTALL_DIR}/include")