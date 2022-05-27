#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include <QWidget>
#include "opencv2/opencv.hpp"
#include "tesseract/baseapi.h"
#include <QLabel>
#include <QGraphicsScene>

namespace Ui {
class ImageFrame;
}

class ImageFrame : public QWidget
{
  Q_OBJECT

public:
  explicit ImageFrame(QWidget *parent = nullptr);
  ~ImageFrame();
  void setImage(QString);

private:
  QString currImage;
  QGraphicsScene *scene;
  Ui::ImageFrame *ui;
  void initUi(QWidget *parent);
};

#endif // IMAGEFRAME_H
