#include "../headers/options.h"
#include "ui_options.h"
#include <tesseract/publictypes.h>

Options::Options(QWidget *parent) : QDialog(parent), ui(new Ui::Options) {
  ui->setupUi(this);
  //  connect(ui->partialBox, SIGNAL(activated(int)), this,
  //  SLOT(partialSelected()));
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

Options::~Options() { delete ui; }

tesseract::PageIteratorLevel Options::getRILSelection() {
  switch (ui->partialBox->currentIndex()) {
  case 0:
    return tesseract::RIL_WORD;
  case 1:
    return tesseract::RIL_BLOCK;
  case 2:
    return tesseract::RIL_TEXTLINE;
  case 3:
    return tesseract::RIL_PARA;
  case 4:
    return tesseract::RIL_SYMBOL;
  }

  return tesseract::RIL_TEXTLINE;
}

tesseract::OcrEngineMode Options::getOEMSelection() {
  switch (ui->engineMode->currentIndex()) {
  case 0:
    return tesseract::OEM_TESSERACT_ONLY;
  case 1:
    return tesseract::OEM_LSTM_ONLY;
  case 2:
    return tesseract::OEM_TESSERACT_LSTM_COMBINED;
  case 3:
    return tesseract::OEM_DEFAULT;
  }

  return tesseract::OEM_DEFAULT;
}

tesseract::PageSegMode Options::getPSMSelection() {
  switch (ui->pageSegMode->currentIndex()) {
  case 0:
    return tesseract::PSM_AUTO_OSD;
  case 1:
    return tesseract::PSM_AUTO;
  case 2:
    return tesseract::PSM_SINGLE_COLUMN;
  case 3:
    return tesseract::PSM_SINGLE_BLOCK_VERT_TEXT;
  case 4:
    return tesseract::PSM_SINGLE_BLOCK;
  case 5:
    return tesseract::PSM_SINGLE_LINE;
  case 6:
    return tesseract::PSM_SINGLE_WORD;
  case 7:
    return tesseract::PSM_SINGLE_CHAR;
  case 8:
    return tesseract::PSM_CIRCLE_WORD;
  case 9:
    return tesseract::PSM_SPARSE_TEXT;
  }

  return tesseract::PSM_AUTO;
}

void Options::on_pushButton_3_clicked() {
  ui->stackedWidget->setCurrentIndex(2);
}

void Options::on_pushButton_clicked() { ui->stackedWidget->setCurrentIndex(0); }

void Options::on_pushButton_2_clicked() {
  ui->stackedWidget->setCurrentIndex(1);
}
