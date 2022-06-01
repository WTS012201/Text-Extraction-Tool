#include "../headers/imagetextobject.h"
#include "ui_imagetextobject.h"

ImageTextObject::ImageTextObject(
    QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ImageTextObject)
{
  ui->setupUi(this);
}

ImageTextObject::ImageTextObject(QWidget *parent, ImageTextObject& old) :
  QWidget(parent),
  ui(new Ui::ImageTextObject)
{
  ui->setupUi(this);
  this->setText(old.getText());
  this->setLineSpaces(old.getLineSpaces());
  this->setPos();
}

void ImageTextObject::setText(QString __text){
  text = __text;
  ui->lineEdit->setText(text);
}

QString ImageTextObject::getText(){
  return text;
}

ImageTextObject::~ImageTextObject()
{
  delete ui;
}

void ImageTextObject::addLineSpace
(QPair<QPoint, QPoint> space){
  lineSpace.push_back(space);
}

void ImageTextObject::setLineSpaces(
    QVector<QPair<QPoint, QPoint>> spaces){
  lineSpace = spaces;
}

QVector<QPair<QPoint, QPoint>> ImageTextObject::getLineSpaces(){
  return lineSpace;
}

void ImageTextObject::setPos(){
  move(lineSpace.first().first);
}
