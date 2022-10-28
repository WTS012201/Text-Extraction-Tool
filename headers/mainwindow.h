#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "colortray.h"
#include "imageframe.h"
#include "tabscroll.h"

#include <QFileDialog>
#include <QMainWindow>
#include <QFile>
#include <QSplitter>
#include <QTextEdit>
#include <QShortcut>
#include <QClipboard>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow;}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void colorTray();
  void on_actionSave_Image_triggered();
  void on_actionOpen_Image_triggered(bool paste=false);
  void fontSelected();
  void fontSizeChanged();
  void on_actionOptions_triggered();
  void on_actionUndo_triggered();
  void on_actionRedo_2_triggered();
  void pastImage();
private:
  QShortcut *undo, *redo, *ctrl, *open, *save, *paste;
  QClipboard *clipboard;
  ImageFrame *iFrame;
  Ui::MainWindow *ui;
  Options* options;
  ColorTray* colorMenu;
  TabScroll* currTab;
  bool deleting;

  void keyReleaseEvent(QKeyEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void loadImage(QString fileName);
  void initUi();
  void loadData();
  void connections();
};
#endif // MAINWINDOW_H
