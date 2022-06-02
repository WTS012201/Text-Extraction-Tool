#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include "ui_mainwindow.h"
#include "imagetextobject.h"
#include "options.h"

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
  void setImage(QString);
  void setWidgets(Ui::MainWindow* ui);
  void setMode(tesseract::PageIteratorLevel __mode);
  void extract();
  void clear();

private slots:
  void changeZoom();
  void setRawText();
signals:
  void rawTextChanged();

private:
  QString currImage, rawText;
  QGraphicsScene* scene;
  QWidget* parent;
  QSize originalSize;
  tesseract::PageIteratorLevel mode;

  QLineEdit* zoomEdit;
  QProgressBar* progressBar;
  QLabel* zoomLabel;
  QVBoxLayout* contentLayout;
  QTextEdit* textEdit;

  float scalar;
  float scaleFactor;

  QVector<ImageTextObject*> textObjects;

  QVector<QString> getLines(QString);
  QVector<QString> getLastWords(QVector<QString> lines);
  void buildConnections();
  void mousePressEvent(QMouseEvent * event) override;
  void initUi(QWidget* parent);
  void zoomIn();
  void zoomOut();
  void showAll();
  void setOptions(Options* options);
  void resize(QSize size);
  void populateTextObjects();
  QString collect(cv::Mat& matrix, tesseract::PageIteratorLevel mode);
  cv::Mat QImageToMat(QImage);
  void test();
};

#endif // IMAGEFRAME_H
