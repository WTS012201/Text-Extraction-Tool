#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "colortray.h"
#include "imageframe.h"
#include "tabscroll.h"

#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMovie>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

signals:
  void switchConnections();

private slots:
  void on_actionRemove_Selection_Ctrl_R_triggered();
  void on_actionAdd_Selection_Ctrl_A_triggered();
  void on_actionGroup_Ctrl_G_triggered();
  void on_actionHide_All_triggered();
  void colorTray();
  void on_actionSave_Image_triggered();
  void on_actionOpen_Image_triggered(bool paste = false);
  void fontSelected();
  void fontSizeChanged();
  void on_actionOptions_triggered();
  void on_actionUndo_triggered();
  void on_actionRedo_2_triggered();
  void pastImage();

private:
  QShortcut *zoomIn, *zoomOut;
  QClipboard *clipboard;
  ImageFrame *iFrame;
  Ui::MainWindow *ui;
  Options *options;
  ColorTray *colorMenu;
  TabScroll *currTab;
  quint8 shift;
  QSettings *settings;

  void keyReleaseEvent(QKeyEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void loadImage(QString fileName);
  void initUi();
  void scanSettings();
  void connections();
  void readSettings();
  void writeSettings(bool __default = false);
};
#endif // MAINWINDOW_H
