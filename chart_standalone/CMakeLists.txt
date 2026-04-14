add_executable(chart_standalone src/main.cpp src/main_window.cpp)

target_link_libraries(
  chart_standalone
  PRIVATE ChartSystem::qtwidgets
          Qt6::Core
          Qt6::Gui
          Qt6::Widgets
          chartsys::options
          chartsys::warnings)

if(CHARTSYS_BUILD_STANDALONE)
  install(
    TARGETS chart_standalone
    RUNTIME DESTINATION bin)
endif()
