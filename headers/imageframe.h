#ifndef IMAGEFRAME_H
#define IMAGEFRAME_H

#include "imagetextobject.h"
#include "options.h"
#include "tesseract/baseapi.h"
#include "ui_mainwindow.h"

#include <QFuture>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QMovie>
#include <QPair>
#include <QPoint>
#include <QRubberBand>
#include <QScrollBar>
#include <QStack>
#include <QVector>
#include <QWidget>
#include <QtConcurrent/QtConcurrent>

class ImageFrame : public QGraphicsView {
  Q_OBJECT

public:
  QHash<int, bool> keysPressed;
  ImageTextObject *selection;
  ImageFrame(QWidget *parent = nullptr, QWidget *tab = nullptr,
             Ui::MainWindow *ui = nullptr, Options *options = nullptr);
  ~ImageFrame();
  void undoAction();
  void redoAction();
  void setImage(QString);
  void setWidgets();
  void setMode(tesseract::PageIteratorLevel __mode);
  void extract(cv::Mat *mat = nullptr);
  void clear();
  void pasteImage(QImage *img);
  void deleteSelection();
  cv::Mat getImageMatrix();

  typedef struct State {
    QVector<ImageTextObject *> textObjects;
    cv::Mat matrix;
  } State;
  State *&getState();
  QString rawText;
  bool isProcessing;
  double scalar;
  double scaleFactor;
  void move(QPoint shift);

public slots:
  void zoomIn();
  void zoomOut();
  void highlightSelection();
  void removeSelection();
  void groupSelections();

private slots:
  void changeZoom();
  void changeText();

signals:
  void processing();
  void colorSelected(cv::Scalar);

private:
  QString filepath;
  QRubberBand *rubberBand;
  QPoint origin;
  QGraphicsScene *scene;
  QWidget *parent;
  cv::Mat display;
  tesseract::PageIteratorLevel mode;
  Ui::MainWindow *ui;
  QMovie *spinner;
  QWidget *tab;

  bool dropper;
  bool middleDown;
  bool zoomChanged;

  QStack<State *> undo, redo;
  State *state;

  cv::Mat QImageToMat(QImage image);
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void connections();
  void initUi(QWidget *parent);
  void showAll();
  void setOptions(Options *options);
  void populateTextObjects();
  void findSubstrings();
  void changeImage(QImage *img = nullptr);
  QString collect(cv::Mat &matrix);
  void inliers(QPair<QPoint, QPoint>);
  void connectSelection(ImageTextObject *obj);
};

#endif // IMAGEFRAME_H
