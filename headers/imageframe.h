#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "ui_mainwindow.h"
#include "imagetextobject.h"
#include "options.h"

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

private slots:
  void changeZoom();
  void setRawText();
  void highlightSelection();
  void changeText();

signals:
  void rawTextChanged();

private:
  QString filepath, rawText;
  QRubberBand* rubberBand;
  QPoint origin;
  QGraphicsScene* scene;
  QWidget* parent;
  cv::Mat display;
  tesseract::PageIteratorLevel mode;
  Ui::MainWindow* ui;

  typedef struct {
    QVector<ImageTextObject*> textObjects;
    cv::Mat matrix;
  } State;
  QStack<State*> undo, redo;
  State* state;

  double scalar;
  double scaleFactor;
  bool middleDown;

  cv::Mat QImageToMat(QImage image);
  void mousePressEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent* event) override;
  void connections();
  void initUi(QWidget* parent);
  void zoomIn();
  void zoomOut();
  void showAll();
  void setOptions(Options* options);
  void populateTextObjects();
  void changeImage(QImage* img = nullptr);
  QString collect(cv::Mat& matrix);
  void inliers(QPair<QPoint, QPoint>);
};

#endif // IMAGEFRAME_H
