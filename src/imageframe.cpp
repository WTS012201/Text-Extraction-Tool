#include "../headers/imageframe.h"

ImageFrame::ImageFrame(QWidget* parent):
  scene{nullptr}
{
  initUi(parent);
}

void ImageFrame::initUi(QWidget* parent){
  parent->setContentsMargins(0,0,0,0);
  this->setParent(parent);
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void ImageFrame::mousePressEvent(QMouseEvent* event) {
  qDebug() << "TEST\n";
}

void ImageFrame::setImage(QString imageName){
  currImage = imageName;

  QPixmap image{imageName};
  if(scene){
    delete scene;
  }
  scene = new QGraphicsScene(this);
  scene->addPixmap(image);
  scene->setSceneRect(image.rect());

  this->setScene(scene);
  this->setMinimumSize(image.size());
}
