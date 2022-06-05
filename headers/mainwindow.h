#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileDialog>
#include <QMainWindow>
#include <QFile>
#include "imageframe.h"
#include <QSplitter>
#include <QTextEdit>
#include <QShortcut>

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
  void fontSelected();
  void fontSizeChanged();
  void on_actionOptions_triggered();
  void on_actionUndo_triggered();
  void on_actionRedo_2_triggered();

private:
  void keyReleaseEvent(QKeyEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  QShortcut *undo, *redo, *ctrl;
  ImageFrame *iFrame;
  Ui::MainWindow *ui;
  Options* options;
  void initUi();
  void loadData();
  void connections();
};
#endif // MAINWINDOW_H
