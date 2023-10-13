add_executable(UnitTests TestMain.cpp)

target_sources(UnitTests PRIVATE
  SimpleTestCase.cpp
)

target_link_libraries(UnitTests PRIVATE Catch2::Catch2)
catch_discover_tests(UnitTests)
