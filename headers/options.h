#ifndef OPTIONS_H
#define OPTIONS_H

#include <QDialog>

#include "tesseract/baseapi.h"

namespace Ui {
class Options;
}

class Options : public QDialog
{
  Q_OBJECT

public:
  explicit Options(QWidget *parent = nullptr);
  ~Options();
  tesseract::PageIteratorLevel getPartialSelection();
private:
  Ui::Options *ui;
};

#endif // OPTIONS_H
