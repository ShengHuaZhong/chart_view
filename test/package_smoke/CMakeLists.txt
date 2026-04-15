cmake_minimum_required(VERSION 3.29)

project(chart_view_package_smoke LANGUAGES CXX)

find_package(chart_view CONFIG REQUIRED)

add_executable(package_smoke main.cpp)
target_link_libraries(package_smoke PRIVATE chart_view::chart_runtime)

if(TARGET chart_view::chart_qtwidgets)
  target_compile_definitions(package_smoke PRIVATE CHART_VIEW_PACKAGE_HAS_QTWIDGETS=1)
  target_link_libraries(package_smoke PRIVATE chart_view::chart_qtwidgets)
endif()
