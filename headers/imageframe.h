#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "ui_mainwindow.h"
#include "imagetextobject.h"
#include "options.h"
#include "content.h"

#include <QStack>
#include <QWidget>
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLayout>
#include <QMouseEvent>
#include <QLineEdit>
#include <QProgressBar>
#include <QThread>
#include <chrono>
#include <thread>
#include <QVector>
#include <QPair>
#include <QPoint>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <QGraphicsTextItem>

class ImageFrame : public QGraphicsView
{
  Q_OBJECT

public:
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
  void extract();
  void clear();

private slots:
  void changeZoom();
  void setRawText();
  void showHighlights();
  void changeText();

signals:
  void rawTextChanged();

private:
  QString filepath, rawText;
  QGraphicsScene* scene;
  QWidget* parent;
  QSize originalSize;
  QHash<int, bool> keysPressed;
  tesseract::PageIteratorLevel mode;
  ImageTextObject* selection;
  Ui::MainWindow* ui;
  QStack<cv::Mat> undo, redo;

  cv::Mat* matrix;

  float scalar;
  float scaleFactor;

  QVector<ImageTextObject*> textObjects;

  void keyReleaseEvent(QKeyEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void connections();
  void mousePressEvent(QMouseEvent * event) override;
  void initUi(QWidget* parent);
  void zoomIn();
  void zoomOut();
  void showAll();
  void setOptions(Options* options);
  void resize(QSize size);
  void populateTextObjects();
  void changeImage();
  cv::Mat* buildImageMatrix();
  QString collect(cv::Mat& matrix);
  cv::Mat QImageToMat(QImage);
};

#endif // IMAGEFRAME_H
