#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "../headers/options.h"
#include "imagetextobject.h"
#include "options.h"
#include "ui_mainwindow.h"

#include <QFuture>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QMovie>
#include <QPair>
#include <QPoint>
#include <QRubberBand>
#include <QScrollBar>
#include <QStack>
#include <QVector>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

constexpr double ZOOM_MAX = 5.0;

class ImageFrame : public QGraphicsView {
  Q_OBJECT

public:
  typedef struct State {
    QVector<ImageTextObject *> textObjects;
    cv::Mat matrix;
    ImageTextObject *selection;
  } State;

  QHash<int, bool> keysPressed;
  ImageTextObject *selection;
  bool isProcessing;
  State *stagedState;

  QString rawText;
  double scalar;
  double scaleIncrement;
  static cv::Scalar defaultColor;

  ImageFrame(QWidget *parent = nullptr, QWidget *tab = nullptr,
             Ui::MainWindow *ui = nullptr, Options *options = nullptr);
  ~ImageFrame();
  void undoAction();
  void redoAction();
  void setImage(QString);
  void setWidgets();
  void extract(cv::Mat *mat = nullptr);
  void clear();
  void pasteImage(QImage *img);
  void deleteSelection();
  cv::Mat getImageMatrix();

  State *&getState();
  void move(QPoint shift);
  void stageState();
  void hideHighlights();

public slots:
  void zoomIn();
  void zoomOut();
  void highlightSelection();
  void removeSelection();
  void groupSelections();

private slots:
  void changeZoom();
  void changeText();

signals:
  void processing();
  void colorSelected(cv::Scalar);
  void unlockState();

private:
  QWidget *tab;
  QString filepath;
  QRubberBand *rubberBand;
  QPoint origin;
  QGraphicsScene *scene;
  QWidget *parent;
  cv::Mat display;
  Options *options;
  Ui::MainWindow *ui;
  QMovie *spinner;

  bool dropper;
  bool middleDown;
  bool zoomChanged;
  bool hideAll;

  QStack<State *> undo, redo;
  State *state;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

  void connections();
  void initUi(QWidget *parent);
  void showAll();
  void setOptions(Options *options);
  void populateTextObjects();
  void findSubstrings();
  void changeImage(QImage *img = nullptr);
  QString collect(cv::Mat &matrix);
  void inliers(QPair<QPoint, QPoint>);
  void connectSelection(ImageTextObject *obj);
};

#endif // IMAGEFRAME_H
