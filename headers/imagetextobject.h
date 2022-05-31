#ifndef IMAGETEXTOBJECT_H
#define IMAGETEXTOBJECT_H

#include <QWidget>
#include <QVector>
#include <QPair>
#include "opencv2/opencv.hpp"

namespace Ui {
class ImageTextObject;
}

class ImageTextObject : public QWidget
{
  Q_OBJECT

public:
  explicit ImageTextObject
  (QWidget *parent = nullptr);
  ~ImageTextObject();

  cv::Point topLeft, bottomRight;

  void setText(QString __text);
  QString getText();
  void addLineSpace(QPair<cv::Point, cv::Point>);
  QVector<QPair<cv::Point, cv::Point>> getLineSpaces();

private:
  QVector<QPair<cv::Point, cv::Point>> lineSpace;
  Ui::ImageTextObject *ui;
  QString text;
};

#endif // IMAGETEXTOBJECT_H
