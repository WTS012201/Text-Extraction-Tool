#include "../headers/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  MainWindow w;
  w.show();

  QVector<QString> args{argv + 1, argv + argc};
  if (!args.empty())
    w.loadArgs(args);

  return a.exec();
}
