add_executable(runtime_smoke_tests runtime_smoke_tests.cpp)

target_link_libraries(
  runtime_smoke_tests
  PRIVATE ChartSystem::runtime chartsys::options chartsys::warnings)

add_test(NAME runtime_smoke_tests COMMAND runtime_smoke_tests)
