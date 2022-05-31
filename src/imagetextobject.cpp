#include "../headers/imagetextobject.h"
#include "ui_imagetextobject.h"

ImageTextObject::ImageTextObject(
    QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ImageTextObject)
{
  ui->setupUi(this);
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
(QPair<cv::Point, cv::Point> space){
  lineSpace.push_back(space);
}

QVector<QPair<cv::Point, cv::Point>> ImageTextObject::getLineSpaces(){
  return lineSpace;
}
