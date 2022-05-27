#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <QWidget>
#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLayout>

class ImageFrame : public QGraphicsView
{
  Q_OBJECT

public:
//  ImageFrame(QGraphicsView* parent = nullptr);
  ImageFrame(QWidget* parent = nullptr);
  void setImage(QString);

  QGraphicsScene* scene;
  QWidget* parent;

private:
  QString currImage;
  void mousePressEvent(QMouseEvent * event) override;
  void initUi(QWidget* parent);
};

#endif // IMAGEFRAME_H
