#ifndef IMAGETEXTOBJECT_H
#define IMAGETEXTOBJECT_H

#include <QFrame>
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
  ImageTextObject(QWidget *parent, ImageTextObject& old);

  QPoint topLeft, bottomRight;

  void setText(QString __text);
  QString getText();
  void addLineSpace(QPair<QPoint, QPoint>);
  QVector<QPair<QPoint, QPoint>> getLineSpaces();
  void setLineSpaces(QVector<QPair<QPoint, QPoint>> spaces);
  void setPos();
  void highlightSpaces();
private:
  QVector<QPair<QPoint, QPoint>> lineSpace;
  Ui::ImageTextObject *ui;
  QString text;
  QPoint findTopLeftCorner();
  QPoint findBottomRightCorner();
};

#endif // IMAGETEXTOBJECT_H
