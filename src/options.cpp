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

tesseract::PageIteratorLevel Options::getPartialSelection(){
  switch(ui->partialBox->currentIndex()){
  case 0:
    return tesseract::RIL_TEXTLINE;
  case 1:
    return tesseract::RIL_BLOCK;
  case 2:
    return tesseract::RIL_WORD;
  case 3:
    return tesseract::RIL_PARA;
  case 4:
    return tesseract::RIL_SYMBOL;
  }
  return tesseract::RIL_TEXTLINE;
}
