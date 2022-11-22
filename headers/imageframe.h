#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "ui_mainwindow.h"
#include "imagetextobject.h"
#include "options.h"

#include <QMovie>
#include <QScrollBar>
#include <QRubberBand>
#include <QStack>
#include <QWidget>
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLayout>
#include <QMouseEvent>
#include <QLineEdit>
#include <QVector>
#include <QPair>
#include <QPoint>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QGraphicsTextItem>

class ImageFrame : public QGraphicsView
{
  Q_OBJECT

public:
  QHash<int, bool> keysPressed;
  ImageTextObject* selection;
  ImageFrame(
      QWidget* parent = nullptr,
      QWidget* tab = nullptr,
      Ui::MainWindow* ui = nullptr,
      Options* options = nullptr
      );
  ~ImageFrame();
  void undoAction();
  void redoAction();
  void setImage(QString);
  void setWidgets();
  void setMode(tesseract::PageIteratorLevel __mode);
  void extract(cv::Mat* mat = nullptr);
  void clear();
  void pasteImage(QImage* img);
  cv::Mat getImageMatrix();

  typedef struct State{
    QVector<ImageTextObject*> textObjects;
    cv::Mat matrix;
  } State;
  State*& getState();
  QString rawText;
  bool isProcessing;
  double scalar;
  double scaleFactor;

public slots:
  void zoomIn();
  void zoomOut();
  void highlightSelection();
  void removeSelection();

private slots:
  void changeZoom();
  void changeText();

signals:
  void processing();


private:
  QString filepath;
  QRubberBand* rubberBand;
  QPoint origin;
  QGraphicsScene* scene;
  QWidget* parent;
  cv::Mat display;
  tesseract::PageIteratorLevel mode;
  Ui::MainWindow* ui;
  QMovie *spinner;
  QWidget* tab;
  bool middleDown;

  QStack<State*> undo, redo;
  State* state;

  cv::Mat QImageToMat(QImage image);
  void mousePressEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent* event) override;
  void connections();
  void initUi(QWidget* parent);
  void showAll();
  void setOptions(Options* options);
  void populateTextObjects();
  void findSubstrings();
  void changeImage(QImage* img = nullptr);
  QString collect(cv::Mat& matrix);
  void inliers(QPair<QPoint, QPoint>);
};

#endif // IMAGEFRAME_H
