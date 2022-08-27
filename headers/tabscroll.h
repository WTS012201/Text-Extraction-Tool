#ifndef TABSCROLL_H
#define TABSCROLL_H

#include <QWidget>
#include "imageframe.h"


namespace Ui {
class TabScroll;
}

class TabScroll : public QWidget
{
  Q_OBJECT

public:
  explicit TabScroll(QWidget *parent = nullptr, ImageFrame* = nullptr);
  ~TabScroll();
  Ui::TabScroll* getUi();
  ImageFrame* iFrame;
private:
  Ui::TabScroll *ui;
};

#endif // TABSCROLL_H
