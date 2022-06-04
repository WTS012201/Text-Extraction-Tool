#ifndef IMAGETEXTOBJECT_H
#define IMAGETEXTOBJECT_H

#include <QFrame>
#include <QWidget>
#include <QVector>
#include <QPair>
#include <QPushButton>
#include <QTextEdit>
#include <QHash>
#include <QImage>
#include <QList>
#include <cmath>
#include "opencv2/opencv.hpp"

class QcvScalar : public cv::Scalar{
  friend inline bool operator==(
      const QcvScalar &e1, const QcvScalar &e2
      ) noexcept{
    bool rVal = (e1.val[0] == e2.val[0]);
    rVal &= (e1.val[1] == e2.val[1]);
    rVal &= (e1.val[2] == e2.val[2]);
    return rVal;
  }

  friend inline uint qHash(const QcvScalar &key, uint seed) noexcept{
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.val[0]);
    seed = hash(seed, key.val[1]);
    seed = hash(seed, key.val[2]);
    return seed;
  }
};

namespace Ui {
class ImageTextObject;
}

class ImageTextObject : public QWidget
{
  Q_OBJECT

public:
  explicit ImageTextObject
  (QWidget *parent = nullptr, cv::Mat* __mat = nullptr);
  ~ImageTextObject();
  ImageTextObject(QWidget *parent,
                  ImageTextObject& old,
                  QTextEdit* tEdit,
                  cv::Mat* mat
                  );

  QPoint topLeft, bottomRight;

  void setText(QString __text);
  QString getText();
  void addLineSpace(QPair<QPoint, QPoint>*);
  QVector<QPair<QPoint, QPoint>*> getLineSpaces();
  void setLineSpaces(QVector<QPair<QPoint, QPoint>*> spaces);
  void initSizeAndPos();
  void highlightSpaces();
  void scaleAndPosition(float scalar);
  void setImage(cv::Mat* __image);
  void showCVImage();
  void setFilepath(QString __filepath);
  void fillText();
private:
  QVector<QPair<QPoint, QPoint>*> lineSpace;
  QHash<QPair<QPoint, QPoint>*, QPushButton*> highlights;

  Ui::ImageTextObject *ui;
  QString filepath;
  QString text;
  QTextEdit* textEdit;
  cv::Scalar bgIntensity;
  cv::Mat& mat;

  QPoint findTopLeftCorner();
  QPoint findBottomRightCorner();
  cv::Mat QImageToMat();
  void determineBgColor();
};

#endif // IMAGETEXTOBJECT_H
