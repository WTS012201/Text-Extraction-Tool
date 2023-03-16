#ifndef IMAGETEXTOBJECT_H
#define IMAGETEXTOBJECT_H

#include "../headers/options.h"
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
#include <QMouseEvent>
#include <QPair>
#include <QPushButton>
#include <QTextEdit>
#include <QVector>
#include <QWidget>
#include <cmath>
#include <opencv2/photo.hpp>
#include <optional>
#include <queue>

constexpr char BLUE_HIGHLIGHT[] = "background:  rgba(37,122,253,100);";
constexpr char YELLOW_HIGHLIGHT[] = "background:  rgba(255, 243, 0, 100);";
constexpr char PURPLE_HIGHLIGHT[] = "background:  rgba(255, 0, 243, 100);";
constexpr char GREEN_HIGHLIGHT[] = "background:  rgba(0, 255, 0, 100);";
constexpr int PALETTE_LIMIT = 10;
constexpr double INVERT_MASK_THRESH = 0.75;

class QcvScalar : public cv::Scalar {
  friend inline bool operator==(const QcvScalar &e1,
                                const QcvScalar &e2) noexcept {
    bool ret = (e1.val[0] == e2.val[0]);
    ret &= (e1.val[1] == e2.val[1]);
    ret &= (e1.val[2] == e2.val[2]);
    return ret;
  }

  friend inline uint qHash(const QcvScalar &key, uint seed) noexcept {
    QtPrivate::QHashCombine hash;
    seed = hash(seed, key.val[0]);
    seed = hash(seed, key.val[1]);
    seed = hash(seed, key.val[2]);
    return seed;
  }
};

class Highlight : public QPushButton {
  Q_OBJECT
signals:
  void drag(const QPoint &) const;

public:
  Highlight(QWidget *parent) : QPushButton{parent} {};

private:
  void mouseMoveEvent(QMouseEvent *) { emit drag(QCursor::pos()); }
};

namespace Ui {
class ImageTextObject;
}

class ImageTextObject : public QWidget {
  Q_OBJECT

signals:
  void selection();

public:
  bool wasSelected, isSelected, isChanged, colorSet, drag;
  int fontSize;
  cv::Scalar bgIntensity, fontIntensity;
  cv::Mat *mat;

  explicit ImageTextObject(QWidget *parent = nullptr, cv::Mat *__mat = nullptr);
  ~ImageTextObject();
  ImageTextObject(QWidget *parent, const ImageTextObject &old,
                  Ui::MainWindow *__ui, cv::Mat *mat, Options *options);
  ImageTextObject(QWidget *parent, ImageTextObject &&old, Ui::MainWindow *__ui,
                  cv::Mat *mat, Options *options);

  Highlight *highlightButton;
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

  std::optional<QPair<cv::Mat, cv::Mat>> fillBackground(bool move = false);
  void scaleAndPosition(double x, double y);
  void selectHighlight();
  void highlight();
  void deselect();
  void showHighlight();
  void setHighlightColor(QString colorStyle);
  void reposition(QPoint shift, bool relative = true);
  QString getHighlightColor();
  void reset();
  void unstageMove();
  static QString formatStyle(cv::Scalar);
  cv::Mat generateTextMask(const cv::Rect &roi);

private:
  static bool moving;
  Options *options;
  Ui::MainWindow *mUi;
  cv::Mat draw;
  std::optional<QPair<cv::Mat, cv::Mat>> textMask;

  Ui::ImageTextObject *ui;
  QString filepath;
  QString text;
  QString colorStyle;

  cv::Mat QImageToMat();
  void determineBgColor();
  void generatePalette();
  void bound();
  void neighboringFill();

  std::optional<QPair<cv::Mat, cv::Mat>> inpaintingFill(bool move);
};

#endif // IMAGETEXTOBJECT_H
