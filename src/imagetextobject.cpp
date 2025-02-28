﻿#include "../headers/imagetextobject.h"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "ui_imagetextobject.h"
#include <optional>

ImageTextObject::ImageTextObject(QWidget *parent, cv::Mat *__mat)
    : QWidget(parent), wasSelected{false}, isSelected{false},
      isPersistent{false}, colorSet{false}, drag{false}, fontSize{14},
      mat{__mat}, highlightButton{nullptr}, ui(new Ui::ImageTextObject),
      colorStyle{YELLOW_HIGHLIGHT} {
  ui->setupUi(this);
}

bool ImageTextObject::moving = false;

ImageTextObject::ImageTextObject(QWidget *parent, const ImageTextObject &old,
                                 Ui::MainWindow *__ui, cv::Mat *__mat,
                                 Options *__options)
    : QWidget(parent), mat{__mat}, options{__options}, mUi{__ui},
      ui(new Ui::ImageTextObject) {

  topLeft = old.topLeft;
  bottomRight = old.bottomRight;
  lineSpace = old.lineSpace;
  fontIntensity = old.fontIntensity;
  fontSize = old.fontSize;
  colorSet = old.colorSet;
  textMask = old.textMask;
  wasSelected = old.wasSelected;
  drag = old.drag;

  ui->setupUi(this);
  setText(old.getText());

  initSizeAndPos();
  determineBgColor();
  generatePalette();
  highlightSpaces();
}

ImageTextObject::ImageTextObject(QWidget *parent, ImageTextObject &&old,
                                 Ui::MainWindow *__ui, cv::Mat *__mat,
                                 Options *__options)
    : QWidget(parent), mat{__mat}, options{__options}, mUi{__ui},
      ui(new Ui::ImageTextObject) {

  topLeft = std::move(old.topLeft);
  bottomRight = std::move(old.bottomRight);
  lineSpace = std::move(old.lineSpace);
  fontIntensity = std::move(old.fontIntensity);
  fontSize = std::move(old.fontSize);
  colorSet = std::move(old.colorSet);
  textMask = std::move(old.textMask);
  drag = std::move(old.drag);
  wasSelected = std::move(old.wasSelected);

  ui->setupUi(this);
  setText(old.getText());

  initSizeAndPos();
  determineBgColor();
  generatePalette();
  highlightSpaces();
}

void ImageTextObject::setFilepath(QString __filepath) { filepath = __filepath; }

void ImageTextObject::setText(QString __text) { text = __text; }

QString ImageTextObject::getText() const { return text; }

ImageTextObject::~ImageTextObject() {
  delete ui;
  delete highlightButton;
}

void ImageTextObject::highlightSpaces() {
  Highlight *highlight = new Highlight{ui->frame};
  auto size = lineSpace.second - lineSpace.first;

  highlight->setCursor(Qt::CursorShape::PointingHandCursor);
  highlight->setMinimumSize(QSize{size.x(), size.y()});
  highlight->setStyleSheet(YELLOW_HIGHLIGHT);
  highlight->show();

  QObject::connect(highlight, &Highlight::clicked, this, [=] {
    if (isPersistent) {
      mUi->fontSizeInput->setText(QString::number(fontSize));
      mUi->textEdit->setText(text);
      highlight->setStyleSheet(GREEN_HIGHLIGHT);

      mUi->colorSelect->setStyleSheet(
          ImageTextObject::formatStyle(fontIntensity));

      emit selection();
    }
  });

  QObject::connect(highlight, &Highlight::pressed, this, [&] { drag = true; });
  QObject::connect(highlight, &Highlight::released, this,
                   [&] { drag = false; });

  highlightButton = highlight;
}

void ImageTextObject::bound() {
  // bound x
  if (topLeft.x() < 0) {
    topLeft.setX(0);
  } else if (bottomRight.x() >= mat->cols) {
    bottomRight.setX(mat->cols - 1);
  }

  // bound y
  if (topLeft.y() < 0) {
    topLeft.setY(0);
  } else if (bottomRight.y() >= mat->rows) {
    bottomRight.setY(mat->rows - 1);
  }
}

void ImageTextObject::initSizeAndPos() {
  auto size = bottomRight - topLeft;

  if (size.x() < 0 || size.y() < 0) {
    qDebug() << "failed to establish size";
    return;
  }

  this->setFixedSize(QSize{size.x(), size.y()});
  ui->frame->setFixedSize(QSize{size.x(), size.y()});
  this->adjustSize();
  ui->frame->adjustSize();
  this->move(topLeft);
  bound();
}

void ImageTextObject::reposition(QPoint shift, bool relative) {
  auto newPosTL = relative ? topLeft + shift : shift;
  auto newPosBR =
      relative ? bottomRight + shift : shift + (bottomRight - topLeft);
  auto frameSize = QSize{mat->cols, mat->rows};

  // bound x
  if (newPosTL.x() < 0) {
    QPoint shiftBack{newPosTL.x(), 0};
    newPosTL -= shiftBack;
    newPosBR -= shiftBack;
  } else if (newPosBR.x() >= frameSize.width()) {
    QPoint shiftBack{newPosBR.x() - frameSize.width() + 1, 0};
    newPosTL -= shiftBack;
    newPosBR -= shiftBack;
  }

  // bound y
  if (newPosTL.y() < 0) {
    QPoint shiftBack{0, newPosTL.y()};
    newPosTL -= shiftBack;
    newPosBR -= shiftBack;
  } else if (newPosBR.y() >= frameSize.height()) {
    QPoint shiftBack{0, newPosBR.y() - frameSize.height() + 1};
    newPosTL -= shiftBack;
    newPosBR -= shiftBack;
  }

  if (!moving) {
    if (options->getFillMethod() == Options::NEIGHBOR) {
      auto region = cv::Rect{cv::Point{topLeft.x(), topLeft.y()},
                             cv::Point{bottomRight.x(), bottomRight.y()}};
      try {
        (*mat)(region).copyTo(draw);
      } catch (const cv::Exception &e) {
        qDebug() << e.what();
      }
    } else {
      textMask = fillBackground(true);
    }
    moving = true;
  }

  topLeft = newPosTL;
  bottomRight = newPosBR;
}

// subtract and fill text in new position
void ImageTextObject::unstageMove() {
  cv::Mat drawArea = mat->rowRange(topLeft.y(), bottomRight.y())
                         .colRange(topLeft.x(), bottomRight.x());
  moving = false;

  if (textMask) {
    auto text = textMask.value().first;
    auto mask = textMask.value().second;

    cv::bitwise_not(mask, mask);
    cv::bitwise_and(mask, drawArea, draw);

    draw += text;
  }
  draw.copyTo(drawArea);

  draw = cv::Mat{};
  textMask = std::optional<QPair<cv::Mat, cv::Mat>>{};
}

void ImageTextObject::scaleAndPosition(double scalar) {
  auto size = scalar * (lineSpace.second - lineSpace.first);
  highlightButton->setMinimumSize(QSize{size.x(), size.y()});
  auto tempBR = bottomRight * scalar;
  auto tempTL = topLeft * scalar;

  lineSpace = QPair<QPoint, QPoint>(tempTL, tempBR);
  auto boxSize = tempBR - tempTL;

  this->setFixedSize(QSize{boxSize.x(), boxSize.y()});
  ui->frame->setFixedSize(QSize{boxSize.x(), boxSize.y()});
  this->adjustSize();
  ui->frame->adjustSize();
  this->move(tempTL);
}

void ImageTextObject::scaleAndPosition(double sx, double sy) {
  int sizeX = sx * (lineSpace.second.x() - lineSpace.first.x());
  int sizeY = sy * (lineSpace.second.y() - lineSpace.first.y());
  highlightButton->setMinimumSize(QSize{sizeX, sizeY});

  auto diff = topLeft + QPoint{sizeX, sizeY};
  bottomRight = diff; // added this
  lineSpace = QPair<QPoint, QPoint>(topLeft, diff);
  auto size = diff - topLeft;

  this->setFixedSize(QSize{size.x(), size.y()});
  ui->frame->setFixedSize(QSize{size.x(), size.y()});
  this->adjustSize();
  ui->frame->adjustSize();

  auto scaleString = mUi->zoomFactor->placeholderText();
  scaleString = scaleString.remove('%');
  this->move(topLeft * scaleString.toDouble() / 100);
  bound();
}

// grabs most frequent colors;
void ImageTextObject::generatePalette() {
  cv::Scalar intensity;

  if (!(mat->type() & CV_8UC3)) {
    qDebug() << "Image must have 3 channels";
    return;
  }

  auto left{topLeft.x()}, top{topLeft.y()};
  auto right{bottomRight.x()}, bottom{bottomRight.y()};
  if (left == right || top == bottom) {
    return;
  }

  auto region = cv::Rect{cv::Point{topLeft.x(), topLeft.y()},
                         cv::Point{bottomRight.x(), bottomRight.y()}};
  cv::Mat mask = generateTextMask(region);
  auto ker = cv::getStructuringElement(cv::MORPH_RECT, {3, 3});
  cv::cvtColor(mask, mask, cv::COLOR_GRAY2BGR);
  cv::Mat text = mask & draw;

  QHash<QcvScalar, int> scalars;
  int max = 0;
  for (auto i = 0; i < right - left; i++) {
    for (auto j = 0; j < bottom - top; j++) {
      QcvScalar key = QcvScalar{text.at<cv::Vec3b>({i, j})};
      if (!scalars.contains(key)) {
        scalars[key] = 1;
      } else {
        ++scalars[key];
      }
      if (max < scalars[key]) {
        max = scalars[key];
        intensity = key;
      }
    }
  }

  QVector<cv::Scalar> __colorPalette;
  typedef QHash<QcvScalar, int>::iterator I;

  auto cmp = [](I lhs, I rhs) -> bool { return lhs.value() < rhs.value(); };
  std::priority_queue<I, QVector<I>, decltype(cmp)> pq(cmp);

  for (I it = scalars.begin(); it != scalars.end(); ++it) {
    pq.push(it);
  }

  for (auto i = 0; i < PALETTE_LIMIT && !pq.empty(); i++) {
    auto key = pq.top().key();
    if (cv::Scalar{0, 0, 0} != key) {
      __colorPalette.push_back(key);
    }
    pq.pop();
  }

  colorPalette = __colorPalette;
  if (colorPalette.isEmpty()) {
    colorPalette.push_back(cv::Scalar{0, 0, 0});
  }
  fontIntensity = colorPalette.first();
}

void ImageTextObject::determineBgColor() {
  cv::Scalar intensity;

  if (!(mat->type() & CV_8UC3)) {
    qDebug() << "Image must have 3 channels";
    return;
  }
  auto left{topLeft.x()}, top{topLeft.y()};
  auto right{bottomRight.x()}, bottom{bottomRight.y()};
  QHash<QcvScalar, int> scalars;

  //  Traverse the outside of the box's edges
  (left > 0) ? left -= 1 : left;
  (right < mat->cols - 1) ? right += 1 : right;
  (top > 0) ? top -= 1 : top;
  (bottom < mat->rows - 1) ? bottom += 1 : bottom;

  (right == mat->cols) ? right -= 1 : right;
  (bottom == mat->rows) ? bottom -= 1 : bottom;

  // Top / Bottom
  for (auto i = left; i < right; i++) {
    QcvScalar key = QcvScalar{mat->at<cv::Vec3b>(cv::Point{i, top})};
    if (!scalars.contains(key)) {
      scalars[key] = 1;
    } else {
      ++scalars[key];
    }

    key = QcvScalar{mat->at<cv::Vec3b>(cv::Point{i, bottom})};
    if (!scalars.contains(key)) {
      scalars[key] = 1;
    } else {
      ++scalars[key];
    }
  }

  //   Right / Left
  for (auto i = top; i < bottom; i++) {
    auto key = QcvScalar{mat->at<cv::Vec3b>(cv::Point{left, i})};
    if (!scalars.contains(key)) {
      scalars[key] = 1;
    } else {
      ++scalars[key];
    }

    key = QcvScalar{mat->at<cv::Vec3b>(cv::Point{right, i})};
    if (!scalars.contains(key)) {
      scalars[key] = 1;
    } else {
      ++scalars[key];
    }
  }

  int max = 0;
  for (const auto &key : scalars.keys()) {
    if (scalars[key] > max) {
      max = scalars[key];
      intensity = key;
    }
  }
  bgIntensity = intensity;
}

std::optional<QPair<cv::Mat, cv::Mat>>
ImageTextObject::fillBackground(bool move) {
  if (options->getFillMethod() == Options::INPAINT) {
    return inpaintingFill(move);
  } else if (options->getFillMethod() == Options::NEIGHBOR) {
    neighboringFill();
  }
  return {};
}

double whiteComp(const cv::Mat &mat) {
  int whiteCount = 0;
  for (int i = 0; i < mat.rows; i++) {
    for (int j = 0; j < mat.cols; j++) {
      if (mat.at<uchar>(i, j) == 255) {
        whiteCount += 1;
      }
    }
  }

  return static_cast<double>(whiteCount) / (mat.rows * mat.cols);
}

cv::Mat ImageTextObject::generateTextMask(const cv::Rect &roi) {
  cv::Mat mask;

  try {
    (*mat)(roi).copyTo(draw);
  } catch (const cv::Exception &e) {
    qDebug() << e.what();
  }

  cv::cvtColor(draw, mask, cv::COLOR_BGR2GRAY);
  cv::threshold(mask, mask, 0, 255, cv::THRESH_OTSU);

  auto whites = whiteComp(mask);
  if (whites > INVERT_MASK_THRESH) {
    mask = ~mask;
  }

  return mask;
}

std::optional<QPair<cv::Mat, cv::Mat>>
ImageTextObject::inpaintingFill(bool move) {
  cv::Mat gray, mask, dst;

  auto bTL = topLeft, bBR = bottomRight;
  auto borderWidth = 3;
  bool bX, tX, bY, tY;
  bX = tX = bY = tY = false;

  if (bTL.x() - borderWidth >= 0) {
    bTL.setX(bTL.x() - borderWidth);
    tX = true;
  }
  if (bTL.y() - borderWidth >= 0) {
    bTL.setY(bTL.y() - borderWidth);
    tY = true;
  }
  if (bBR.x() + borderWidth < mat->cols) {
    bBR.setX(bBR.x() + borderWidth);
    bX = true;
  }
  if (bBR.y() + borderWidth < mat->rows) {
    bBR.setY(bBR.y() + borderWidth);
    bY = true;
  }

  auto region =
      cv::Rect{cv::Point{bTL.x(), bTL.y()}, cv::Point{bBR.x(), bBR.y()}};
  mask = generateTextMask(region);
  auto ker = cv::getStructuringElement(cv::MORPH_RECT, {3, 3});
  cv::dilate(mask, gray, ker, {-1, -1}, 1);
  cv::inpaint(draw, gray, dst, 3, cv::INPAINT_NS);

  cv::Mat trimmed;
  if (move) {
    ker = cv::getStructuringElement(cv::MORPH_RECT, {2, 2});
    cv::erode(gray, gray, ker, {-1, -1}, 1);
    cv::cvtColor(gray, gray, cv::COLOR_GRAY2BGR);
    trimmed = draw & gray;
    trimmed = trimmed
                  .rowRange(tY ? borderWidth : 0,
                            bY ? trimmed.rows - borderWidth : trimmed.rows)
                  .colRange(tX ? borderWidth : 0,
                            bX ? trimmed.cols - borderWidth : trimmed.cols);
    gray = gray.rowRange(tY ? borderWidth : 0,
                         bY ? gray.rows - borderWidth : gray.rows)
               .colRange(tX ? borderWidth : 0,
                         bX ? gray.cols - borderWidth : gray.cols);
  }

  dst.copyTo(mat->rowRange(bTL.y(), bBR.y()).colRange(bTL.x(), bBR.x()));

  if (move)
    return QPair<cv::Mat, cv::Mat>{trimmed, gray};
  else
    return {};
}

void ImageTextObject::neighboringFill() {
  auto left{topLeft.x()}, top{topLeft.y()};
  auto right{bottomRight.x()}, bottom{bottomRight.y()};
  cv::Vec3b bg;

  bg.val[0] = bgIntensity.val[0];
  bg.val[1] = bgIntensity.val[1];
  bg.val[2] = bgIntensity.val[2];

  (left > 0) ? left -= 1 : left;
  (right < mat->cols - 1) ? right += 1 : right;

  (top > 0) ? top -= 1 : top;
  (bottom < mat->rows - 1) ? bottom += 1 : bottom;

  for (auto i = top; i <= bottom; i++) {
    for (auto j = left; j <= right; j++) {
      mat->at<cv::Vec3b>(cv::Point{j, i}) = bg;
    }
  }
}

void ImageTextObject::highlight() {
  highlightButton->isHidden() ? highlightButton->show()
                              : highlightButton->hide();
}

void ImageTextObject::showHighlight() {
  this->show();
  highlightButton->show();
  highlightButton->setStyleSheet(colorStyle);
}

void ImageTextObject::setHighlightColor(QString __colorStyle) {
  colorStyle = __colorStyle;
}

QString ImageTextObject::getHighlightColor() { return colorStyle; }

void ImageTextObject::selectHighlight() {
  this->show();
  isSelected = true;
  highlightButton->show();
  highlightButton->setStyleSheet(BLUE_HIGHLIGHT);
}

void ImageTextObject::deselect() {
  highlightButton->setStyleSheet(colorStyle);
  isSelected = false;
  if (isPersistent) {
    return;
  }
  highlightButton->hide();
}

void ImageTextObject::reset() {
  deselect();
  hide();
  isPersistent = false;
}

QString ImageTextObject::formatStyle(cv::Scalar color) {

  QString style = "background-color: rgb(";
  style += QString::number(color[2]) + ',';
  style += QString::number(color[1]) + ',';
  style += QString::number(color[0]) + ')';

  return style;
}
