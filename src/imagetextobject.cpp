#include "../headers/imagetextobject.h"
#include "ui_imagetextobject.h"

ImageTextObject::ImageTextObject(QWidget *parent, cv::Mat *__mat)
    : QWidget(parent), isSelected{false},
      highlightButton{nullptr}, isChanged{false}, mat{__mat},
      ui(new Ui::ImageTextObject), colorStyle{
                                       "background:  rgba(255, 243, 0, 100);"} {
  ui->setupUi(this);
}

ImageTextObject::ImageTextObject(QWidget *parent, ImageTextObject &old,
                                 Ui::MainWindow *__ui, cv::Mat *__mat)
    : QWidget(parent), mat{__mat}, mUi{__ui}, ui(new Ui::ImageTextObject) {
  topLeft = old.topLeft;
  bottomRight = old.bottomRight;
  lineSpace = old.lineSpace;
  ui->setupUi(this);
  setText(old.getText());

  initSizeAndPos();
  highlightSpaces();
  determineBgColor();
  determineFontColor();
}

void ImageTextObject::setFilepath(QString __filepath) { filepath = __filepath; }

void ImageTextObject::setText(QString __text) { text = __text; }

QString ImageTextObject::getText() { return text; }

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

  QObject::connect(highlight, &QPushButton::clicked, this, [=](...) {
    if (isChanged) {
      /* int fsEstimate = bottomRight.y() - topLeft.y(); */
      /*  */
      /* mUi->fontSizeInput->setText(QString::number(fsEstimate)); */
      mUi->fontSizeInput->setText(QString::number(14));
      mUi->textEdit->setText(text);
      highlight->setStyleSheet(GREEN_HIGHLIGHT);
      emit selection();
    }
  });
  highlightButton = highlight;
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
}

void ImageTextObject::reposition(QPoint shift) {
  auto newPosTL = topLeft + shift;
  auto newPosBR = bottomRight + shift;
  auto frameSize = QSize(mat->cols, mat->rows);

  // bound x
  if (newPosTL.x() < 0) {
    newPosTL = QPoint{0, newPosTL.y()};
    newPosBR = bottomRight;
  } else if (newPosBR.x() >= frameSize.width()) {
    newPosTL = topLeft;
    newPosBR = QPoint{frameSize.width() - 1, newPosBR.y()};
  }

  // bound y
  if (newPosTL.y() < 0) {
    newPosTL = QPoint{newPosTL.x(), 0};
    newPosBR = bottomRight;
  } else if (newPosBR.y() >= frameSize.height()) {
    newPosTL = topLeft;
    newPosBR = QPoint{newPosBR.x(), frameSize.height() - 1};
  }

  topLeft = newPosTL;
  bottomRight = newPosBR;
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

  auto tempBR = topLeft + QPoint{sizeX, sizeY};
  lineSpace = QPair<QPoint, QPoint>(topLeft, tempBR);
  auto size = tempBR - topLeft;

  this->setFixedSize(QSize{size.x(), size.y()});
  ui->frame->setFixedSize(QSize{size.x(), size.y()});
  this->adjustSize();
  ui->frame->adjustSize();

  this->move(topLeft * mUi->zoomFactor->text().toDouble());
}

void ImageTextObject::showCVImage() {
  cv::namedWindow("image");
  cv::imshow("image", *mat);
  cv::waitKey();
}

// grabs most common border color

void ImageTextObject::determineFontColor() {
  cv::Scalar intensity;

  if (!(mat->type() & CV_8UC3)) {
    qDebug() << "Image must have 3 channels";
    return;
  }
  auto left{topLeft.x()}, top{topLeft.y()};
  auto right{bottomRight.x()}, bottom{bottomRight.y()};
  QHash<QcvScalar, int> scalars;
  int max = 0;

  for (auto i = left; i <= right; i++) {
    for (auto j = top; j <= bottom; j++) {
      QcvScalar key = QcvScalar{mat->at<cv::Vec3b>(cv::Point{i, j})};
      if (key == bgIntensity) {
        continue;
      }
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
  fontIntensity = intensity;
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

  (left > 0) ? left -= 1 : left;
  (right < mat->cols - 1) ? right += 1 : right;

  (top > 0) ? top -= 1 : top;
  (bottom < mat->rows - 1) ? bottom += 1 : bottom;

  // Top / Bottom
  for (auto i = left; i <= right; i++) {
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
  for (auto i = top; i <= bottom; i++) {
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
      auto &scalarRef = mat->at<cv::Vec3b>(cv::Point{j, i});
      scalarRef = bg;
    }
  }
}

void ImageTextObject::highlight() {
  highlightButton->isHidden() ? highlightButton->show()
                              : highlightButton->hide();
}

void ImageTextObject::showHighlights() {
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
