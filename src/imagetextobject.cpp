#include "../headers/imagetextobject.h"
#include "ui_imagetextobject.h"

ImageTextObject::ImageTextObject(QWidget *parent, cv::Mat *__mat)
    : QWidget(parent), isSelected{false}, isChanged{false}, colorSet{false},
      fontSize{14}, mat{__mat}, highlightButton{nullptr},
      ui(new Ui::ImageTextObject), colorStyle{YELLOW_HIGHLIGHT} {
  ui->setupUi(this);
}

ImageTextObject::ImageTextObject(QWidget *parent, const ImageTextObject &old,
                                 Ui::MainWindow *__ui, cv::Mat *__mat)
    : QWidget(parent), mat{__mat}, mUi{__ui}, ui(new Ui::ImageTextObject) {
  topLeft = old.topLeft;
  bottomRight = old.bottomRight;
  lineSpace = old.lineSpace;
  fontIntensity = old.fontIntensity;
  fontSize = old.fontSize;
  colorSet = old.colorSet;

  ui->setupUi(this);
  setText(old.getText());

  initSizeAndPos();
  highlightSpaces();
  determineBgColor();
  generatePalette();
}

void ImageTextObject::setFilepath(QString __filepath) { filepath = __filepath; }

void ImageTextObject::setText(QString __text) { text = __text; }

QString ImageTextObject::getText() const { return text; }

ImageTextObject::~ImageTextObject() {
  delete ui;
  delete highlightButton;
}

void ImageTextObject::highlightSpaces() {
  QPushButton *highlight = new QPushButton{ui->frame};
  auto size = lineSpace.second - lineSpace.first;

  highlight->setCursor(Qt::CursorShape::PointingHandCursor);
  highlight->setMinimumSize(QSize{size.x(), size.y()});
  highlight->setStyleSheet(YELLOW_HIGHLIGHT);
  highlight->show();

  QObject::connect(highlight, &QPushButton::clicked, this, [=] {
    if (isChanged) {
      mUi->fontSizeInput->setText(QString::number(fontSize));
      mUi->textEdit->setText(text);
      highlight->setStyleSheet(GREEN_HIGHLIGHT);
      emit selection(this);
    }
  });
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

void ImageTextObject::reposition(QPoint shift) {
  auto newPosTL = topLeft + shift;
  auto newPosBR = bottomRight + shift;
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

  auto region = cv::Rect{cv::Point{topLeft.x(), topLeft.y()},
                         cv::Point{bottomRight.x(), bottomRight.y()}};
  cv::Mat draw;
  (*mat)(region).copyTo(draw);
  fillBackground();
  topLeft = newPosTL;
  bottomRight = newPosBR;

  draw.copyTo(mat->rowRange(topLeft.y(), bottomRight.y())
                  .colRange(topLeft.x(), bottomRight.x()));
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
  QHash<QcvScalar, int> scalars;
  int max = 0;

  if (left == right || top == bottom) {
    return;
  }

  for (auto i = left; i < right; i++) {
    for (auto j = top; j < bottom; j++) {
      QcvScalar key = QcvScalar{mat->at<cv::Vec3b>(cv::Point{i, j})};
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
  typedef QHash<QcvScalar, int>::iterator T;

  auto cmp = [](T lhs, T rhs) -> bool { return lhs.value() > rhs.value(); };
  std::priority_queue<T, QVector<T>, decltype(cmp)> pq(cmp);

  for (T it = scalars.begin(); it != scalars.end(); ++it) {
    pq.push(it);
  }

  for (auto i = 0; i < PALETTE_LIMIT && !pq.empty(); i++) {
    __colorPalette.push_back(pq.top().key());
    pq.pop();
  }

  colorPalette = __colorPalette;
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

void ImageTextObject::fillBackground() {
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
  if (isChanged) {
    return;
  }
  highlightButton->hide();
}

void ImageTextObject::reset() {
  deselect();
  hide();
  isChanged = false;
}

QString ImageTextObject::formatStyle(cv::Scalar color) {

  QString style = "background-color: rgb(";
  style += QString::number(color[2]) + ',';
  style += QString::number(color[1]) + ',';
  style += QString::number(color[0]) + ')';

  return style;
}
