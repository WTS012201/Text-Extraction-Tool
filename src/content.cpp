#include "../headers/content.h"
#include "ui_content.h"

Content::Content(QWidget *parent) :
  QWidget(parent), textObject{nullptr},
  ui(new Ui::Content)
{
  ui->setupUi(this);
  connections();
}

Content::~Content()
{
  delete ui;
}

QLabel* Content::getLabel(){
  return ui->contentLabel;
}

void Content::setTextObject(ImageTextObject* __textObject){
  textObject = __textObject;
}

void Content::connections(){
  connect(ui->highlightButton, &QPushButton::clicked, this, [&](){
    if(!textObject){
      qDebug() << "Text Object null";
      return;
    }
    textObject->show();
    textObject->highlight();
    QString text = ui->highlightButton->text();
    if(text == "Highlight: On"){
      ui->highlightButton->setText("Highlight: Off");
    } else{
      ui->highlightButton->setText("Highlight: On");
    }
  });
}
