#include "../headers/imageframe.h"
#include "ui_imageframe.h"

ImageFrame::ImageFrame(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ImageFrame)
{
 initUi(parent);
}

ImageFrame::~ImageFrame()
{
  delete ui;
}

void ImageFrame::initUi(QWidget *parent){
  ui->setupUi(this);
  ui->frame->hide();
  this->setMaximumSize(this->size());

}

void ImageFrame::setImage(QString imageName){
  currImage = imageName;
  imageName = ":/img/" + imageName;

  QPixmap image{imageName};

  this->resize(image.size() + QSize{25, 25});

  scene = new QGraphicsScene(this);
  scene->addPixmap(image);
  scene->setSceneRect(image.rect());

  ui->imageView->setScene(scene);
  ui->frame->show();
}
