#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <QWidget>
#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>

class ImageFrame : public QGraphicsView
{
  Q_OBJECT

public:
  ImageFrame(QGraphicsView* parent = nullptr);
  void setImage(QString);

  QGraphicsView* parent;
  QGraphicsScene* scene;

private:
  QString currImage;

};

#endif // IMAGEFRAME_H
