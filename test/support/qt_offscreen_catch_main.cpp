#include <catch2/catch_session.hpp>

#include <QByteArray>
#include <QGuiApplication>

#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char *argv[])
{
  if(qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  }

  Catch::Session session;
  std::vector<char const *> catchArgv(argv, argv + argc);
  const auto parseResult = session.applyCommandLine(argc, catchArgv.data());
  if(parseResult != 0) {
    std::fflush(stdout);
    std::fflush(stderr);
    std::_Exit(parseResult);
  }
  const bool hasTestFilters = session.config().hasTestFilters();

  QGuiApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  int result = session.run();
  if(result == Catch::UnmatchedTestSpecExitCode && !hasTestFilters) {
    result = 0;
  }

  std::fflush(stdout);
  std::fflush(stderr);
  std::_Exit(result);
}
