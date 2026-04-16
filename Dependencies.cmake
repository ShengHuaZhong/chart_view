include(cmake/CPM.cmake)

function(chart_view_setup_dependencies)
  set(chart_view_VCPKG_BIN_DIR "" CACHE INTERNAL "vcpkg runtime bin directory" FORCE)
  set(chart_view_VCPKG_DEBUG_BIN_DIR "" CACHE INTERNAL "vcpkg debug runtime bin directory" FORCE)
  set(chart_view_VCPKG_RUNTIME_DLLS_RELEASE "" CACHE INTERNAL "vcpkg release runtime DLLs" FORCE)
  set(chart_view_VCPKG_RUNTIME_DLLS_DEBUG "" CACHE INTERNAL "vcpkg debug runtime DLLs" FORCE)

  set(_chart_view_vcpkg_installed_dir "")
  if(DEFINED VCPKG_INSTALLED_DIR AND NOT VCPKG_INSTALLED_DIR STREQUAL "")
    file(TO_CMAKE_PATH "${VCPKG_INSTALLED_DIR}" _chart_view_vcpkg_installed_dir)
  elseif(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_TOOLCHAIN_FILE STREQUAL "")
    file(TO_CMAKE_PATH "${CMAKE_TOOLCHAIN_FILE}" _chart_view_toolchain_file)
    if(_chart_view_toolchain_file MATCHES "/scripts/buildsystems/vcpkg\\.cmake$")
      get_filename_component(_chart_view_vcpkg_root "${_chart_view_toolchain_file}/../../.." ABSOLUTE)
      file(TO_CMAKE_PATH "${_chart_view_vcpkg_root}/installed" _chart_view_vcpkg_installed_dir)
    endif()
  endif()

  if(DEFINED VCPKG_TARGET_TRIPLET
     AND NOT VCPKG_TARGET_TRIPLET STREQUAL ""
     AND NOT _chart_view_vcpkg_installed_dir STREQUAL "")
    file(TO_CMAKE_PATH "${_chart_view_vcpkg_installed_dir}/${VCPKG_TARGET_TRIPLET}/bin" _chart_view_vcpkg_bin_dir)
    file(TO_CMAKE_PATH "${_chart_view_vcpkg_installed_dir}/${VCPKG_TARGET_TRIPLET}/debug/bin" _chart_view_vcpkg_debug_bin_dir)

    set(chart_view_VCPKG_BIN_DIR "${_chart_view_vcpkg_bin_dir}" CACHE INTERNAL "vcpkg runtime bin directory" FORCE)
    set(chart_view_VCPKG_DEBUG_BIN_DIR "${_chart_view_vcpkg_debug_bin_dir}" CACHE INTERNAL "vcpkg debug runtime bin directory" FORCE)

    file(
      GLOB _chart_view_vcpkg_runtime_dlls_release
      LIST_DIRECTORIES FALSE
      "${_chart_view_vcpkg_bin_dir}/zlib*.dll"
      "${_chart_view_vcpkg_bin_dir}/zstd*.dll")
    file(
      GLOB _chart_view_vcpkg_runtime_dlls_debug
      LIST_DIRECTORIES FALSE
      "${_chart_view_vcpkg_debug_bin_dir}/zlib*.dll"
      "${_chart_view_vcpkg_debug_bin_dir}/zstd*.dll")

    list(SORT _chart_view_vcpkg_runtime_dlls_release)
    list(SORT _chart_view_vcpkg_runtime_dlls_debug)

    set(
      chart_view_VCPKG_RUNTIME_DLLS_RELEASE
      "${_chart_view_vcpkg_runtime_dlls_release}"
      CACHE INTERNAL "vcpkg release runtime DLLs"
      FORCE)
    set(
      chart_view_VCPKG_RUNTIME_DLLS_DEBUG
      "${_chart_view_vcpkg_runtime_dlls_debug}"
      CACHE INTERNAL "vcpkg debug runtime DLLs"
      FORCE)
  endif()

  if(chart_view_BUILD_RUNTIME OR chart_view_BUILD_QTWIDGETS OR chart_view_BUILD_STANDALONE)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui GuiPrivate Widgets)

    get_filename_component(_chart_view_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
    file(TO_CMAKE_PATH "${_chart_view_qt_prefix}" _chart_view_qt_prefix)
    file(TO_CMAKE_PATH "${_chart_view_qt_prefix}/bin" _chart_view_qt_bin_dir)
    file(TO_CMAKE_PATH "${_chart_view_qt_prefix}/plugins" _chart_view_qt_plugin_dir)

    set(chart_view_QT_PREFIX_DIR "${_chart_view_qt_prefix}" CACHE INTERNAL "Qt installation prefix")
    set(chart_view_QT_BIN_DIR "${_chart_view_qt_bin_dir}" CACHE INTERNAL "Qt runtime bin directory")
    set(chart_view_QT_PLUGIN_DIR "${_chart_view_qt_plugin_dir}" CACHE INTERNAL "Qt plugin directory")
  endif()

  if(chart_view_BUILD_RUNTIME OR (BUILD_TESTING AND chart_view_BUILD_TESTS))
    find_package(PROJ CONFIG REQUIRED)
  endif()

  if(BUILD_TESTING AND chart_view_BUILD_TESTS AND NOT TARGET Catch2::Catch2WithMain)
    cpmaddpackage(
      NAME
      Catch2
      VERSION
      3.12.0
      GITHUB_REPOSITORY
      "catchorg/Catch2"
      SYSTEM
      YES)
  endif()
endfunction()
