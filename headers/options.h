#ifndef OPTIONS_H
#define OPTIONS_H

#include "tesseract/baseapi.h"
#include <QDialog>
#include <tesseract/publictypes.h>

namespace Ui {
class Options;
}

class Options : public QDialog {
  Q_OBJECT

public:
  explicit Options(QWidget *parent = nullptr);
  ~Options();
  tesseract::PageIteratorLevel getRIL();
  tesseract::OcrEngineMode getOEM();
  tesseract::PageSegMode getPSM();
  void setRIL(tesseract::PageIteratorLevel RIL);
  void setOEM(tesseract::OcrEngineMode OCR);
  void setPSM(tesseract::PageSegMode PSM);
  void setDataDir(QString dirName);
  void setDataFile(QString fileName);
  QString getDataDir();
  QString getDataFile();

private slots:
  void on_pushButton_3_clicked();

  void on_pushButton_clicked();

  void on_pushButton_2_clicked();

private:
  Ui::Options *ui;
};

#endif // OPTIONS_H
