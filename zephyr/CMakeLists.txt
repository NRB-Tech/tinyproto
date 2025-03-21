# Copyright (c) 2024 TinyProto Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TINYPROTO)
  # Enable appropriate compile definitions based on Kconfig
  if(TINYPROTO_ERROR_DET_CHECKSUM)
    zephyr_compile_definitions(CONFIG_ENABLE_CHECKSUM)
  endif()

  if(CONFIG_TINYPROTO_ERROR_DET_FCS16)
    zephyr_compile_definitions(CONFIG_ENABLE_FCS16)
  endif()

  if(TINYPROTO_ERROR_DET_FCS32)
    zephyr_compile_definitions(CONFIG_ENABLE_FCS32)
  endif()

  # Create the interface library first
  zephyr_interface_library_named(TINYPROTO)
  
  # Set include directories for both interface and regular library
  zephyr_include_directories(
    ../src
    ../src/hal
    ../src/hal/zephyr
    ../src/proto
  )
  
  target_include_directories(TINYPROTO INTERFACE 
    ../src
    ../src/hal
    ../src/hal/zephyr
    ../src/proto
  )

  # Enable thread support in Zephyr
  zephyr_compile_definitions(CONFIG_TINYHAL_THREAD_SUPPORT=1)
  
  # Force .inl files to be treated as C
  set_source_files_properties(
    ../src/hal/zephyr/zephyr_hal.inl
    ../src/hal/zephyr/zephyr_serial.inl
    PROPERTIES
    LANGUAGE C
  )
  
  # Use the same source file collection approach as the main CMakeLists.txt
  file(GLOB_RECURSE TINYPROTO_SOURCE_FILES ../src/*.cpp ../src/*.c)
  
  # Add tinyproto library
  zephyr_library()
  
  # Add all the source files collected by GLOB_RECURSE
  zephyr_library_sources(${TINYPROTO_SOURCE_FILES})

  # Set C++ specific compile options
  zephyr_library_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
  zephyr_library_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
  
  # Define Zephyr platform
  zephyr_library_compile_definitions(__ZEPHYR__=1)
  
  # Link with Zephyr kernel
  zephyr_library_link_libraries(TINYPROTO)
  target_link_libraries(TINYPROTO INTERFACE zephyr_interface)
  
  # Link against kernel specifically before anything else to resolve z_impl functions
  target_link_libraries(TINYPROTO INTERFACE kernel)
endif() 