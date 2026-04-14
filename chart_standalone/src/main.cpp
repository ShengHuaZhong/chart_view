#include "main_window.hpp"

#include <QApplication>

auto main(int argc, char* argv[]) -> int {
  QApplication app(argc, argv);

  MainWindow window;
  window.show();

  return app.exec();
}
