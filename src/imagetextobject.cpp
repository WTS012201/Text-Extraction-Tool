#include "../headers/imagetextobject.h"
#include "ui_imagetextobject.h"

ImageTextObject::ImageTextObject(
    QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ImageTextObject)
{
  ui->setupUi(this);
}

ImageTextObject::ImageTextObject(
    QWidget *parent, ImageTextObject& old, QTextEdit* tEdit) :
  QWidget(parent), ui(new Ui::ImageTextObject),
  textEdit{tEdit}
{
  ui->setupUi(this);
  setText(old.getText());

  setLineSpaces(old.getLineSpaces());
  highlightSpaces();
  initSizeAndPos();
}

void ImageTextObject::setText(QString __text){
  text = __text;
}

QString ImageTextObject::getText(){
  return text;
}

ImageTextObject::~ImageTextObject()
{
  delete ui;
}

void ImageTextObject::addLineSpace
(QPair<QPoint, QPoint>* space){
  lineSpace.push_back(space);
}

void ImageTextObject::setLineSpaces(
    QVector<QPair<QPoint, QPoint>*> spaces){
  lineSpace = spaces;
}

QVector<QPair<QPoint, QPoint>*> ImageTextObject::getLineSpaces(){
  return lineSpace;
}


QPoint ImageTextObject::findTopLeftCorner(){
  QPoint minPoint = lineSpace.first()->first;

  for(auto& p : lineSpace){
    if(p->first.x() < minPoint.x()){
      minPoint.setX(p->first.x());
    }
    if(p->first.y() < minPoint.y()){
      minPoint.setY(p->first.y());
    }
  }

  return minPoint;
}

QPoint ImageTextObject::findBottomRightCorner(){
  QPoint maxPoint = lineSpace.first()->first;

  for(auto& p : lineSpace){
    if(p->second.x() > maxPoint.x()){
      maxPoint.setX(p->second.x());
    }
    if(p->second.y() > maxPoint.y()){
      maxPoint.setY(p->second.y());
    }
  }

  return maxPoint;
}

void ImageTextObject::initSizeAndPos(){
  topLeft = findTopLeftCorner();
  bottomRight = findBottomRightCorner();
  auto size = bottomRight - topLeft;

  if(size.x() < 0 || size.y() < 0){
    qDebug() << "failed to establish size";
    return;
  }

  this->setFixedSize(QSize{size.x(), size.y()});
  ui->frame->setFixedSize(QSize{size.x(), size.y()});
  this->adjustSize();
  ui->frame->adjustSize();
  this->move(topLeft);
}

void ImageTextObject::highlightSpaces(){
  for(auto space : lineSpace){
    auto highlight = new QPushButton{ui->frame};
    auto size = space->second - space->first;

    if(size.x() < 0 || size.y() < 0){
      qDebug() << "failed to establish size";
      delete highlight;
      continue;
    }

    highlight->setCursor(Qt::CursorShape::PointingHandCursor);
    highlight->setMinimumSize(QSize{size.x(), size.y()});
    highlight->setStyleSheet("background:  rgba(255, 243, 0, 100);");
    highlight->show();

    QObject::connect(
          highlight, &QPushButton::clicked,
          this, [=](){textEdit->setText(text);}
          );
    highlights[space] = highlight;
  }
}

void ImageTextObject::scaleAndPosition(float scalar){
  for(auto& space : lineSpace){
    QPushButton* highlight = highlights[space];
    auto size = scalar*(space->second - space->first);
    highlight->setMinimumSize(QSize{size.x(), size.y()});
  }
  auto tempBR = bottomRight * scalar;
  auto tempTL = topLeft * scalar;

  auto size = tempBR - tempTL;

  this->setFixedSize(QSize{size.x(), size.y()});
  ui->frame->setFixedSize(QSize{size.x(), size.y()});
  this->adjustSize();
  ui->frame->adjustSize();
  this->move(tempTL);
}

void ImageTextObject::setImage(cv::Mat* __image){
  image = __image;
  determineBgColor();
//  showCVImage();
}

void ImageTextObject::showCVImage(){
  cv::namedWindow("image");
  cv::imshow("image", *image);
  cv::waitKey();
}

void ImageTextObject::determineBgColor(){
  cv::Scalar intensity;
  cv::Mat& mat = *image;

  // adjust method for this later
  if(!(mat.type() & CV_8UC3)){
    qDebug() << "change channel to CV_8UC3";
    return;
  }

  auto left{topLeft.x()}, top{topLeft.y()};
  auto right{bottomRight.x()}, bottom{bottomRight.y()};
  QHash<QcvScalar, int> scalars;

  (left > 0) ? left -= 1 : left;
  (right < mat.cols - 1) ? right += 1 : right;

  (top > 0) ? top -= 1 : top;
  (bottom < mat.rows - 1) ? bottom += 1 : bottom;

  // Top / Bottom
  for(auto i = left; i <= right; i++){
    auto key = QcvScalar{mat.at<cv::Vec3b>(i, top)};

    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      scalars[key]++;
    }

    key = QcvScalar{mat.at<cv::Vec3b>(i, bottom)};
    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      scalars[key]++;
    }
  }

  // Right / Left
  for(auto i = top; i <= bottom; i++){
    auto key = QcvScalar{mat.at<cv::Vec3b>(left, i)};
    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      scalars[key]++;
    }

    key = QcvScalar{mat.at<cv::Vec3b>(right, i)};
    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      scalars[key]++;
    }
  }

  int max = 0;
  for(const auto& key : scalars.keys()){
    if(!(key.val[0] && key.val[1] && key.val[2]))
      continue;
    qDebug() << "vals: ";
    qDebug() << key.val[0];
    qDebug() << key.val[1];
    qDebug() << key.val[2];
    qDebug() << "amount: " << scalars[key];
    if(scalars[key] > max){
      max = scalars[key];
      intensity = key;
    }
  }
  bgIntensity = intensity;
  qDebug() << bgIntensity.val[0];
  qDebug() << bgIntensity.val[1];
  qDebug() << bgIntensity.val[2];
}
