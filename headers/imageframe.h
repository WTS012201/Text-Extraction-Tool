﻿#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "ui_mainwindow.h"
#include "imagetextobject.h"
#include "options.h"

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
  ImageTextObject* selection;
  Ui::MainWindow* ui;
  QStack<cv::Mat> undo, redo;
  QVector<ImageTextObject*> textObjects;

  cv::Mat* matrix;

  double scalar;
  double scaleFactor;

  void mousePressEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void connections();
  void initUi(QWidget* parent);
  void zoomIn();
  void zoomOut();
  void showAll();
  void setOptions(Options* options);
  void populateTextObjects();
  void changeImage(QImage* img = nullptr);
  cv::Mat* buildImageMatrix();
  QString collect(cv::Mat& matrix);
  cv::Mat QImageToMat(QImage);

  void inSelection(QPair<QPoint, QPoint>);
};

#endif // IMAGEFRAME_H
