#ifndef IMAGETEXTOBJECT_H
#define IMAGETEXTOBJECT_H

#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFrame>
#include <QHash>
#include <QImage>
#include <QList>
#include <QPair>
#include <QPushButton>
#include <QTextEdit>
#include <QVector>
#include <QWidget>
#include <cmath>
#include <queue>

constexpr char BLUE_HIGHLIGHT[] = "background:  rgba(37,122,253,100);";
constexpr char YELLOW_HIGHLIGHT[] = "background:  rgba(255, 243, 0, 100);";
constexpr char PURPLE_HIGHLIGHT[] = "background:  rgba(255, 0, 243, 100);";
constexpr char GREEN_HIGHLIGHT[] = "background:  rgba(0, 255, 0, 100);";
constexpr int PALETTE_LIMIT = 5;

class QcvScalar : public cv::Scalar {
  friend inline bool operator==(const QcvScalar &e1,
                                const QcvScalar &e2) noexcept {
    bool rVal = (e1.val[0] == e2.val[0]);
    rVal &= (e1.val[1] == e2.val[1]);
    rVal &= (e1.val[2] == e2.val[2]);
    return rVal;
  }

  friend inline uint qHash(const QcvScalar &key, uint seed) noexcept {
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

class ImageTextObject : public QWidget {
  Q_OBJECT

signals:
  void selection();

public:
  bool isSelected, isChanged, colorSet;
  int fontSize;
  cv::Scalar bgIntensity, fontIntensity;
  cv::Mat *mat;
  explicit ImageTextObject(QWidget *parent = nullptr, cv::Mat *__mat = nullptr);
  ~ImageTextObject();
  ImageTextObject(QWidget *parent, const ImageTextObject &old,
                  Ui::MainWindow *__ui, cv::Mat *mat);

  QPoint topLeft, bottomRight;
  QPair<QPoint, QPoint> lineSpace;
  QVector<cv::Scalar> colorPalette;

  void setText(QString __text);
  QString getText() const;
  void initSizeAndPos();
  void highlightSpaces();
  void scaleAndPosition(double scalar);
  void setImage(cv::Mat *__image);
  void setFilepath(QString __filepath);
  void fillBackground();
  void scaleAndPosition(double x, double y);
  void selectHighlight();
  void highlight();
  void deselect();
  void showHighlight();
  void setHighlightColor(QString colorStyle);
  void reposition(QPoint shift);
  QString getHighlightColor();
  void reset();
  static QString formatStyle(cv::Scalar);

private:
  QPushButton *highlightButton;
  Ui::MainWindow *mUi;

  Ui::ImageTextObject *ui;
  QString filepath;
  QString text;
  QString colorStyle;

  cv::Mat QImageToMat();
  void determineBgColor();
  void generatePalette();
};

#endif // IMAGETEXTOBJECT_H
