#include "../headers/imagetextobject.h"
#include "ui_imagetextobject.h"

ImageTextObject::ImageTextObject(
    QWidget *parent, cv::Mat* __mat) :
  QWidget(parent), ui(new Ui::ImageTextObject),
  mat{*__mat}
{
  ui->setupUi(this);
}

ImageTextObject::ImageTextObject(
    QWidget *parent, ImageTextObject& old,
    Ui::MainWindow* __ui, cv::Mat* __mat):
  QWidget(parent), mUi{__ui},
  ui(new Ui::ImageTextObject), mat{*__mat}
{
  ui->setupUi(this);
  setText(old.getText());

  setLineSpaces(old.getLineSpaces());
  highlightSpaces();
  initSizeAndPos();
  determineBgColor();
//  fillText();
}

void ImageTextObject::setFilepath(QString __filepath){
  filepath = __filepath;
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
          this, [=](){
              mUi->textEdit->setText(text);
              emit selection();
            }
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

void ImageTextObject::showCVImage(){
  cv::namedWindow("image");
  cv::imshow("image", mat);
  cv::waitKey();
}

void ImageTextObject::determineBgColor(){
  cv::Scalar intensity;
  // adjust method for this later
  if(!(mat.type() & CV_8UC3)){
    qDebug() << "Image must have 3 channels";
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
    QcvScalar key = QcvScalar{
        mat.at<cv::Vec3b>(cv::Point{i, top})
    };
    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      ++scalars[key];
    }

    key = QcvScalar{
        mat.at<cv::Vec3b>(cv::Point{i, bottom})
    };
    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      ++scalars[key];
    }
  }

//   Right / Left
  for(auto i = top; i <= bottom; i++){
    auto key = QcvScalar{
        mat.at<cv::Vec3b>(cv::Point{left, i})
    };
    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      ++scalars[key];
    }

    key = QcvScalar{
        mat.at<cv::Vec3b>(cv::Point{right, i})
    };
    if(!scalars.contains(key)){
      scalars[key] = 1;
    } else{
      ++scalars[key];
    }
  }

  int max = 0;

  for(const auto& key : scalars.keys()){
    if(scalars[key] > max){
      max = scalars[key];
      intensity = key;
    }
  }
  bgIntensity = intensity;
}


void ImageTextObject::fillText(){
  auto left{topLeft.x()}, top{topLeft.y()};
  auto right{bottomRight.x()}, bottom{bottomRight.y()};
  cv::Vec3b bg;

  bg.val[0] = bgIntensity.val[0];
  bg.val[1] = bgIntensity.val[1];
  bg.val[2] = bgIntensity.val[2];

  (left > 0) ? left -= 1 : left;
  (right < mat.cols - 1) ? right += 1 : right;

  (top > 0) ? top -= 1 : top;
  (bottom < mat.rows - 1) ? bottom += 1 : bottom;

  for(auto i = top; i <= bottom; i++){
    for(auto j = left; j <= right; j++){
      auto& scalarRef = mat.at<cv::Vec3b>(cv::Point{j,i});
      scalarRef = bg;
    }
  }
//  showCVImage();
}
