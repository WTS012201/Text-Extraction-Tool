#include "../headers/imageframe.h"

ImageFrame::ImageFrame(QGraphicsView* __parent):
  parent{__parent}
{

}

void ImageFrame::setImage(QString imageName){
  currImage = imageName;
  imageName = ":/img/" + imageName;

  QPixmap image{imageName};
  scene = new QGraphicsScene(parent);
  scene->addPixmap(image);
  scene->setSceneRect(image.rect());

  parent->setScene(scene);
}
