set(CMAKE_AUTOMOC ON)

add_executable(chart_standalone
  main.cpp
  main_window.hpp
  main_window.cpp)

target_link_libraries(
  chart_standalone
  PRIVATE chart_view::chart_view_options
          chart_view::chart_view_warnings
          chart_view::chart_qtwidgets)

chart_view_configure_linker(chart_standalone)
chart_view_copy_runtime_dlls(chart_standalone)
