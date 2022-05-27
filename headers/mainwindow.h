#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QMainWindow>
#include <QFile>
#include "../headers/imageframe.h"
#include <QSplitter>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_actionOpen_Image_triggered();

private:
  ImageFrame *iFrame;
  Ui::MainWindow *ui;
  void initUi();
};
#endif // MAINWINDOW_H
