﻿#include "../headers/imagetextobject.h"
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
  setText(old.getText());

  setLineSpaces(old.getLineSpaces());
  highlightSpaces();
  setPos();
}

void ImageTextObject::setText(QString __text){
  text = __text;
//  ui->lineEdit->setPlainText(text);
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
  topLeft = lineSpace.first().first;
  bottomRight = lineSpace.last().second;

  auto size = bottomRight - topLeft;

  if(size.x() < 0 || size.y() < 0){
    qDebug() << "failed to establish size";
    return;
  }

  this->setFixedSize(QSize{size.x(), size.y()});
  ui->frame->setFixedSize(QSize{size.x(), size.y()});
  this->adjustSize();
  ui->frame->adjustSize();
  move(topLeft);
}

void ImageTextObject::highlightSpaces(){

  for(auto space : lineSpace){
    auto highlight = new QFrame{ui->frame};
    auto size = space.second - space.first;

    if(size.x() < 0 || size.y() < 0){
      delete highlight;
      continue;
    }

    highlight->setMinimumSize(QSize{size.x(), size.y()});
    highlight->setStyleSheet("background:  rgba(255, 243, 0, 100);");
    highlight->show();

//    qDebug() << "";
  }
}