#include "../headers/options.h"
#include "ui_options.h"
#include <tesseract/publictypes.h>

Options::Options(QWidget *parent) : QDialog(parent), ui(new Ui::Options) {
  ui->setupUi(this);
  connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

Options::~Options() { delete ui; }

tesseract::PageIteratorLevel Options::getRIL() {
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

tesseract::OcrEngineMode Options::getOEM() {
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

tesseract::PageSegMode Options::getPSM() {
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

void Options::setRIL(tesseract::PageIteratorLevel RIL) {
  switch (RIL) {
  case tesseract::RIL_WORD:
    ui->partialBox->setCurrentIndex(0);
    return;
  case tesseract::RIL_BLOCK:
    ui->partialBox->setCurrentIndex(1);
    return;
  case tesseract::RIL_TEXTLINE:
    ui->partialBox->setCurrentIndex(2);
    return;
  case tesseract::RIL_PARA:
    ui->partialBox->setCurrentIndex(3);
    return;
  case tesseract::RIL_SYMBOL:
    ui->partialBox->setCurrentIndex(4);
    return;
  }
}

void Options::setOEM(tesseract::OcrEngineMode OCR) {
  ui->engineMode->setCurrentIndex(OCR);
}
void Options::setPSM(tesseract::PageSegMode PSM) {
  switch (PSM) {
  case tesseract::PSM_AUTO_OSD:
    ui->pageSegMode->setCurrentIndex(0);
    return;
  case tesseract::PSM_AUTO:
    ui->pageSegMode->setCurrentIndex(1);
    return;
  case tesseract::PSM_SINGLE_COLUMN:
    ui->pageSegMode->setCurrentIndex(2);
    return;
  case tesseract::PSM_SINGLE_BLOCK_VERT_TEXT:
    ui->pageSegMode->setCurrentIndex(3);
    return;
  case tesseract::PSM_SINGLE_BLOCK:
    ui->pageSegMode->setCurrentIndex(4);
    return;
  case tesseract::PSM_SINGLE_LINE:
    ui->pageSegMode->setCurrentIndex(5);
    return;
  case tesseract::PSM_SINGLE_WORD:
    ui->pageSegMode->setCurrentIndex(6);
    return;
  case tesseract::PSM_SINGLE_CHAR:
    ui->pageSegMode->setCurrentIndex(7);
    return;
  case tesseract::PSM_CIRCLE_WORD:
    ui->pageSegMode->setCurrentIndex(8);
    return;
  case tesseract::PSM_SPARSE_TEXT:
    ui->pageSegMode->setCurrentIndex(9);
    return;
  default:
    ui->pageSegMode->setCurrentIndex(1);
    return;
  }
}

void Options::setFillMethod(Options::fillMethod option) {
  ui->fillMethod->setCurrentIndex(static_cast<int>(option));
}

Options::fillMethod Options::getFillMethod() {
  return static_cast<Options::fillMethod>(ui->fillMethod->currentIndex());
}

void Options::on_pushButton_3_clicked() {
  ui->stackedWidget->setCurrentIndex(2);
}

void Options::on_pushButton_clicked() { ui->stackedWidget->setCurrentIndex(0); }

void Options::on_pushButton_2_clicked() {
  ui->stackedWidget->setCurrentIndex(1);
}

// MAKE SURE THEY EXIST
void Options::setDataDir(QString dirName) { ui->dataDir->setText(dirName); }

void Options::setDataFile(QString fileName) { ui->dataFile->setText(fileName); }

QString Options::getDataDir() { return ui->dataDir->text(); }

QString Options::getDataFile() { return ui->dataFile->text(); }
