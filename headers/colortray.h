#ifndef COLORTRAY_H
#define COLORTRAY_H

#include <QDialog>
#include "opencv2/opencv.hpp"

namespace Ui {
class ColorTray;
}

class ColorTray : public QDialog
{
  Q_OBJECT

public:
  explicit ColorTray(QWidget *parent = nullptr);
  void setColor(cv::Scalar scalar);
  ~ColorTray();
  QColor color;
private:
  Ui::ColorTray *ui;
  void connections();
  void changeBgColor();
};

#endif // COLORTRAY_H
