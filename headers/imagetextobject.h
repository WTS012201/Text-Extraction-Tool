#ifndef IMAGETEXTOBJECT_H
#define IMAGETEXTOBJECT_H

#include "ui_mainwindow.h"
#include "opencv2/opencv.hpp"

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

signals:
  void selection();

public:
  bool isSelected, isChanged;
  cv::Mat* mat;
  explicit ImageTextObject
  (QWidget *parent = nullptr, cv::Mat* __mat = nullptr);
  ~ImageTextObject();
  ImageTextObject(QWidget *parent,
                  ImageTextObject& old,
                  Ui::MainWindow* __ui,
                  cv::Mat* mat
                  );

  QPoint topLeft, bottomRight;
  QPair<QPoint, QPoint> lineSpace;

  void setText(QString __text);
  QString getText();
  void initSizeAndPos();
  void highlightSpaces();
  void scaleAndPosition(float scalar);
  void setImage(cv::Mat* __image);
  void showCVImage();
  void setFilepath(QString __filepath);
  void fillBackground();
  void scaleAndPosition(float x, float y);
  void selectHighlight();
  void highlight();
  void deselect();
  void showHighlights();
private:
  QPushButton* highlightButton;
  Ui::MainWindow* mUi;

  Ui::ImageTextObject *ui;
  QString filepath;
  QString text;

  cv::Scalar bgIntensity;

  cv::Mat QImageToMat();
  void determineBgColor();
};

#endif // IMAGETEXTOBJECT_H
