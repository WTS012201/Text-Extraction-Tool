#ifndef TABSCROLL_H
#define TABSCROLL_H

#include "imageframe.h"
#include <QWidget>

namespace Ui {
class TabScroll;
}

class TabScroll : public QWidget {
  Q_OBJECT

public:
  explicit TabScroll(QWidget *parent = nullptr, ImageFrame * = nullptr);
  ~TabScroll();
  Ui::TabScroll *getUi();
  ImageFrame *iFrame;
  QScrollArea *getScrollArea();

private:
  Ui::TabScroll *ui;
};

#endif // TABSCROLL_H
