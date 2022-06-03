#ifndef IMAGETEXTOBJECT_H
#define IMAGETEXTOBJECT_H

#include <QFrame>
#include <QWidget>
#include <QVector>
#include <QPair>
#include <QPushButton>
#include <QTextEdit>
#include <QHash>

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
  ImageTextObject(QWidget *parent, ImageTextObject& old, QTextEdit* tEdit);

  QPoint topLeft, bottomRight;

  void setText(QString __text);
  QString getText();
  void addLineSpace(QPair<QPoint, QPoint>*);
  QVector<QPair<QPoint, QPoint>*> getLineSpaces();
  void setLineSpaces(QVector<QPair<QPoint, QPoint>*> spaces);
  void initSizeAndPos();
  void highlightSpaces();
  void scaleAndPosition(float scalar);
private:
  QVector<QPair<QPoint, QPoint>*> lineSpace;
  QHash<QPair<QPoint, QPoint>*, QPushButton*> highlights;

  Ui::ImageTextObject *ui;
  QString text;
  QTextEdit* textEdit;

  QPoint findTopLeftCorner();
  QPoint findBottomRightCorner();
};

#endif // IMAGETEXTOBJECT_H
