#ifndef CONTENT_H
#define CONTENT_H

#include <QWidget>
#include <QLabel>

#include "imagetextobject.h"

namespace Ui {
class Content;
}

class Content : public QWidget
{
  Q_OBJECT

public:
  explicit Content(QWidget *parent = nullptr);
  ~Content();
  QLabel* getLabel();
  ImageTextObject* textObject;
  void setTextObject(ImageTextObject* textObject);
private:
  Ui::Content *ui;
  void connections();
};

#endif // CONTENT_H
