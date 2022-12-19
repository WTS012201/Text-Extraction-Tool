#include "../headers/options.h"
#include "ui_options.h"

Options::Options(QWidget *parent) :
  QDialog(parent), ui(new Ui::Options)
{
  ui->setupUi(this);
//  connect(ui->partialBox, SIGNAL(activated(int)), this, SLOT(partialSelected()));
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

Options::~Options()
{
  delete ui;
}

tesseract::PageIteratorLevel Options::getPILSelection(){
  switch(ui->partialBox->currentIndex()){
  case 2:
    return tesseract::RIL_TEXTLINE;
  case 1:
    return tesseract::RIL_BLOCK;
  case 0:
    return tesseract::RIL_WORD;
  case 3:
    return tesseract::RIL_PARA;
  case 4:
    return tesseract::RIL_SYMBOL;
  }
  return tesseract::RIL_TEXTLINE;
}

void Options::on_pushButton_3_clicked(){
  ui->stackedWidget->setCurrentIndex(2);
}

void Options::on_pushButton_clicked()
{
  ui->stackedWidget->setCurrentIndex(0);
}


void Options::on_pushButton_2_clicked()
{
  ui->stackedWidget->setCurrentIndex(1);
}

