#include "../headers/colortray.h"
#include "ui_colortray.h"

ColorTray::ColorTray(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::ColorTray){
  ui->setupUi(this);

  connections();
}

ColorTray::~ColorTray(){
  delete ui;
}

void ColorTray::connections(){
  QObject::connect(ui->colorRed, &QSlider::sliderMoved, this, [&](){
    QSlider* slider = qobject_cast<QSlider*>(sender());
    color.setRed(slider->value());
    changeBgColor();
  });
  QObject::connect(ui->colorGreen, &QSlider::sliderMoved, this, [&](){
    QSlider* slider = qobject_cast<QSlider*>(sender());
    color.setGreen(slider->value());
    changeBgColor();
  });
  QObject::connect(ui->colorBlue, &QSlider::sliderMoved, this, [&](){
    QSlider* slider = qobject_cast<QSlider*>(sender());
    color.setBlue(slider->value());
    changeBgColor();
  });
  color.setRed(ui->colorRed->value());
  color.setGreen(ui->colorGreen->value());
  color.setBlue(ui->colorBlue->value());
}

void ColorTray::changeBgColor(){
  QString style = "background-color: rgb(";
  style += QString::number(color.red()) + ',';
  style += QString::number(color.green()) + ',';
  style += QString::number(color.blue()) + ')';
  ui->colorView->setStyleSheet(style);
}
