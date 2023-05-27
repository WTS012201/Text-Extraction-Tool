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
  QObject::connect(ui->colorRed, &QSlider::actionTriggered, this, [&](){
    QSlider* slider = qobject_cast<QSlider*>(sender());
    color.setRed(slider->value());
    changeBgColor();
  });

  QObject::connect(ui->colorGreen, &QSlider::actionTriggered, this, [&](){
    QSlider* slider = qobject_cast<QSlider*>(sender());
    color.setGreen(slider->value());
    changeBgColor();
  });

  QObject::connect(ui->colorBlue, &QSlider::actionTriggered, this, [&](){
    QSlider* slider = qobject_cast<QSlider*>(sender());
    color.setBlue(slider->value());
    changeBgColor();
  });

  QObject::connect(ui->redEdit, &QLineEdit::textEdited, this, [&]{
    color.setRed(ui->redEdit->text().toInt());
    ui->colorRed->setValue(color.red());
    changeBgColor();
  });

  QObject::connect(ui->blueEdit, &QLineEdit::textEdited, this, [&]{
    color.setBlue(ui->blueEdit->text().toInt());
    ui->colorBlue->setValue(color.blue());
    changeBgColor();
  });

  QObject::connect(ui->greenEdit, &QLineEdit::textEdited, this, [&]{
    color.setGreen(ui->greenEdit->text().toInt());
    ui->colorGreen->setValue(color.green());
    changeBgColor();
  });

  color.setRed(ui->colorRed->value());
  color.setGreen(ui->colorGreen->value());
  color.setBlue(ui->colorBlue->value());

  changeBgColor();
}

void ColorTray::changeBgColor(){
  ui->redEdit->setText(QString::number(color.red()));
  ui->greenEdit->setText(QString::number(color.green()));
  ui->blueEdit->setText(QString::number(color.blue()));

  QString style = "background-color: rgb(";
  style += QString::number(color.red()) + ',';
  style += QString::number(color.green()) + ',';
  style += QString::number(color.blue()) + ')';
  ui->colorView->setStyleSheet(style);
}

void ColorTray::setColor(cv::Scalar scalar){
  color = QColor{(int)scalar[2], (int)scalar[1], (int)scalar[0]};
  ui->colorRed->setValue(color.red());
  ui->colorGreen->setValue(color.green());
  ui->colorBlue->setValue(color.blue());
  changeBgColor();
}
