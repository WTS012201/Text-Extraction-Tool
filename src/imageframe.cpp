#include "../headers/imageframe.h"
#include "../headers/tabscroll.h"
#include "headers/imagetextobject.h"
#include "qnamespace.h"

ImageFrame::ImageFrame(QWidget *parent, QWidget *__tab, Ui::MainWindow *__ui,
                       Options *__options)
    : selection{nullptr}, isProcessing{false}, spinner{nullptr}, tab{__tab},
      scalar{1.0}, scaleFactor{0.1}, rubberBand{nullptr}, options{__options},
      zoomChanged{false}, scene{new QGraphicsScene(this)}, ui{__ui},
      middleDown{false}, state{new State}, dropper{false} {
  initUi(parent);
  setWidgets();
  connections();

  ui->zoomFactor->deselect();
}

ImageFrame::~ImageFrame() {
  for (const auto &obj : state->textObjects) {
    delete obj;
  }
  for (const auto &state : undo) {
    delete state;
  }
  for (const auto &state : redo) {
    delete state;
  }

  delete scene;
  delete rubberBand;
}

cv::Mat ImageFrame::getImageMatrix() { return state->matrix; }

void ImageFrame::changeZoom() {
  if (!this->isEnabled())
    return;

  zoomChanged = true;
  double val = (ui->zoomFactor->text()).toDouble();

  scalar = (val < 0.1) ? 0.1 : val;
  scalar = (scalar > ZOOM_MAX) ? ZOOM_MAX : scalar;
  ui->zoomFactor->setText(QString::number(scalar));
  changeImage();
  for (auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
  }
}

inline cv::Mat QImageToCvMat(const QImage &inImage,
                             bool inCloneImageData = true) {
  switch (inImage.format()) {
  // 8-bit, 4 channel
  case QImage::Format_ARGB32:
  case QImage::Format_ARGB32_Premultiplied: {
    cv::Mat mat(inImage.height(), inImage.width(), CV_8UC4,
                const_cast<uchar *>(inImage.bits()),
                static_cast<size_t>(inImage.bytesPerLine()));
    return (inCloneImageData ? mat.clone() : mat);
  }

    // 8-bit, 3 channel
  case QImage::Format_RGB32: {
    if (!inCloneImageData) {
      qWarning() << "ASM::QImageToCvMat() - Conversion requires cloning so we "
                    "don't modify the original QImage data";
    }

    cv::Mat mat(inImage.height(), inImage.width(), CV_8UC4,
                const_cast<uchar *>(inImage.bits()),
                static_cast<size_t>(inImage.bytesPerLine()));

    cv::Mat matNoAlpha;

    cv::cvtColor(mat, matNoAlpha,
                 cv::COLOR_BGRA2BGR); // drop the all-white alpha channel
    return matNoAlpha;
  }

    // 8-bit, 3 channel
  case QImage::Format_RGB888: {
    if (!inCloneImageData) {
      qWarning() << "ASM::QImageToCvMat() - Conversion requires cloning so we "
                    "don't modify the original QImage data";
    }

    QImage swapped = inImage.rgbSwapped();
    return cv::Mat(swapped.height(), swapped.width(), CV_8UC3,
                   const_cast<uchar *>(swapped.bits()),
                   static_cast<size_t>(swapped.bytesPerLine()))
        .clone();
  }

    // 8-bit, 1 channel
  case QImage::Format_Indexed8: {
    cv::Mat mat(inImage.height(), inImage.width(), CV_8UC1,
                const_cast<uchar *>(inImage.bits()),
                static_cast<size_t>(inImage.bytesPerLine()));
    return (inCloneImageData ? mat.clone() : mat);
  }

  default:
    qWarning() << "ASM::QImageToCvMat() - QImage format not handled in switch:"
               << inImage.format();
    break;
  }

  return cv::Mat();
}

void ImageFrame::pasteImage(QImage *img) {
  for (auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  State *oldState = new State{state->textObjects, cv::Mat{}, selection};

  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState); // scene dims
  state->textObjects.erase(state->textObjects.begin(),
                           state->textObjects.end());

  scalar = 1.0;
  auto imagePixmap = QPixmap::fromImage(*img);

  scene->addPixmap(imagePixmap);
  scene->setSceneRect(imagePixmap.rect());
  scene->update();
  this->setScene(scene);

  this->setMinimumSize(imagePixmap.size());
  this->setMaximumSize(imagePixmap.size());
  auto mat = QImageToCvMat(*img);
  extract(&mat);
  populateTextObjects();
}

void ImageFrame::changeImage(QImage *img) {
  try {
    state->matrix.copyTo(display);
    cv::resize(display, display, cv::Size{}, scalar, scalar, cv::INTER_CUBIC);
  } catch (cv::Exception &e) {
    // if no image in first state, have to do this
    delete scene;
    scene = new QGraphicsScene(this);
    return;
  }

  if (!img) {
    img = new QImage{(uchar *)display.data, display.cols, display.rows,
                     (int)display.step, QImage::Format_BGR888};
  }

  auto imagePixmap = QPixmap::fromImage(*img);

  scene->addPixmap(imagePixmap);
  scene->setSceneRect(imagePixmap.rect());
  scene->update();
  this->setScene(scene);

  this->setMinimumSize(imagePixmap.size());
  this->setMaximumSize(imagePixmap.size());
  delete img;
}

void ImageFrame::changeText() {
  if (!this->isEnabled())
    return;

  if (!selection) {
    qDebug() << "No selection";
    return;
  }
  const cv::Scalar colorSelection = selection->fontIntensity;
  QColor color{
      (int)selection->fontIntensity[2],
      (int)selection->fontIntensity[1],
      (int)selection->fontIntensity[0],
  };
  QVector<ImageTextObject *> oldObjs = state->textObjects;
  state->textObjects.remove(state->textObjects.indexOf(selection));
  selection->hide();
  selection->setDisabled(true);
  selection = new ImageTextObject{this, *selection, ui, &state->matrix};
  selection->setHighlightColor(GREEN_HIGHLIGHT);
  selection->isChanged = true;
  connectSelection(selection);

  state->textObjects.push_back(selection);
  State *oldState = new State{oldObjs, cv::Mat{}, selection};
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);

  selection->fillBackground();

  QImage *img;
  if (!zoomChanged) {
    state->matrix.copyTo(display);
    img = new QImage{(uchar *)display.data, display.cols, display.rows,
                     (int)display.step, QImage::Format_BGR888};
  } else {
    img = new QImage{(uchar *)state->matrix.data, state->matrix.cols,
                     state->matrix.rows, (int)state->matrix.step,
                     QImage::Format_BGR888};
  }
  QPainter p;
  if (!p.begin(img)) {
    qDebug() << "error with painter";
    return;
  }
  int fontSize = ui->fontSizeInput->text().toInt();
  QString label = ui->textEdit->toPlainText();
  selection->setText(label);
  QFont font{ui->fontBox->itemText(ui->fontBox->currentIndex()), fontSize};
  p.setFont(font);

  /* take max horizontal length */
  QFontMetrics fm{p.font()};
  auto j = 0, k = 0, max = 0;
  while ((j = label.indexOf("\n", j)) != -1) {
    auto sub = label.mid(k, j - k);
    max = qMax(max, fm.horizontalAdvance(sub));
    k = ++j;
  }
  auto sub = label.mid(k, label.size() - k);
  max = qMax(max, fm.horizontalAdvance(sub));

  double x = max;
  double y = fm.height();

  QPoint wh{selection->topLeft.x() + (int)x, selection->topLeft.y() + (int)y};
  QRect oldRect{selection->topLeft, selection->bottomRight};

  auto offset = wh.y() - selection->bottomRight.y();
  selection->topLeft.setY(selection->topLeft.y() - offset);
  selection->bottomRight = wh;
  selection->bottomRight.setY(selection->bottomRight.y() - offset);
  QRect rect{selection->topLeft, selection->bottomRight};

  double newWidth = rect.width() * 1.0 / oldRect.width();
  double newHeight =
      (label.count("\n") + 1) * rect.height() * 1.0 / oldRect.height();
  selection->scaleAndPosition(newWidth, newHeight);

  p.save();
  p.setPen(color);

  auto dy = rect.height();
  auto i = 0;
  j = 0, k = 0, max = 0;
  while ((j = label.indexOf("\n", j)) != -1) {
    auto sub = label.mid(k, j - k);
    QPoint translateY{0, (i * dy)};

    QRect subrect{translateY + selection->topLeft,
                  translateY + selection->bottomRight};
    p.drawText(subrect, sub, Qt::AlignLeft | Qt::AlignLeft);
    k = ++j;
    i++;
  }
  sub = label.mid(k, label.size() - k);
  QPoint translateY{0, (i * dy)};

  QRect subrect{translateY + selection->topLeft,
                translateY + selection->bottomRight};
  p.drawText(subrect, sub, Qt::AlignLeft | Qt::AlignLeft);
  k = ++j;
  i++;

  p.restore();
  p.end();

  // update state for changes
  state->matrix =
      cv::Mat{img->height(), img->width(), CV_8UC3, (void *)img->constBits(),
              (size_t)img->bytesPerLine()};
  selection->isChanged = true;
  selection->showHighlights();
  selection->mat = &state->matrix;
  selection->fontIntensity = colorSelection;
  delete img;

  changeImage();
}

void ImageFrame::connections() {
  connect(spinner, &QMovie::frameChanged, this, [&] {
    ui->tab->setTabIcon(ui->tab->indexOf(tab), QIcon{spinner->currentPixmap()});
  });
  connect(this, &ImageFrame::processing, this, [&] {
    isProcessing = !isProcessing;

    if (!this->isEnabled())
      return;
    if (!isProcessing) {
      populateTextObjects();
      spinner->stop();
      ui->tab->setTabIcon(ui->tab->indexOf(tab), QIcon{});
    } else {
      spinner->start();
    }
  });
  connect(ui->dropper, &QToolButton::pressed, this, [&] { dropper = true; });
  connect(ui->highlightAll, &QPushButton::pressed, this,
          &ImageFrame::highlightSelection);
  connect(ui->changeButton, &QPushButton::pressed, this,
          &ImageFrame::changeText);
  connect(ui->zoomFactor, &QLineEdit::editingFinished, this,
          &ImageFrame::changeZoom);
  connect(ui->find, &QLineEdit::editingFinished, this,
          &ImageFrame::findSubstrings);
  connect(ui->removeSelection, &QPushButton::pressed, this,
          &ImageFrame::removeSelection);
}

void ImageFrame::removeSelection() {
  for (auto &obj : state->textObjects) {
    if (obj->isSelected) {
      obj->deselect();
      obj->hide();
      obj->isChanged = false;
    }
  }
}

void ImageFrame::findSubstrings() {
  QString query = ui->find->text();

  if (query.isEmpty()) {

    for (auto &obj : state->textObjects) {
      if (obj->getHighlightColor() == PURPLE_HIGHLIGHT) {
        obj->setHighlightColor(BLUE_HIGHLIGHT);
        obj->isChanged = false;
      }
    }
    return;
  }

  for (auto &obj : state->textObjects) {
    if (obj->getText().contains(query, Qt::CaseInsensitive)) {
      obj->setHighlightColor(PURPLE_HIGHLIGHT);
      obj->showHighlights();
      obj->isChanged = true;
    } else {
      obj->setHighlightColor(YELLOW_HIGHLIGHT);
    }
  }
}

void ImageFrame::highlightSelection() {
  if (!this->isEnabled())
    return;

  for (auto &obj : state->textObjects) {
    if (obj->isSelected) {
      obj->setHighlightColor(YELLOW_HIGHLIGHT);
      obj->isChanged = true;
      obj->deselect();
    }
  }
}

void ImageFrame::setWidgets() {
  ui->zoomFactor->hide();
  ui->zoomLabel->hide();
}

void ImageFrame::initUi(QWidget *parent) {
  parent->setContentsMargins(0, 0, 0, 0);
  spinner = new QMovie(":/img/spinner.gif");

  this->setParent(parent);
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->hide();
}

void ImageFrame::zoomIn() {
  if (!this->isEnabled())
    return;

  zoomChanged = true;
  auto sa =
      qobject_cast<TabScroll *>(ui->tab->currentWidget())->getScrollArea();
  auto hb = sa->horizontalScrollBar();
  auto vb = sa->verticalScrollBar();

  int prevH = hb->value() - hb->maximum();
  int prevV = vb->value() - vb->maximum();

  (scalar + scaleFactor > ZOOM_MAX) ? scalar = ZOOM_MAX : scalar += scaleFactor;
  ui->zoomFactor->setText(QString::number(scalar));
  changeImage();
  for (auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
  }

  hb->setValue(prevH + hb->maximum());
  vb->setValue(prevV + vb->maximum());
}

void ImageFrame::zoomOut() {
  if (!this->isEnabled())
    return;

  zoomChanged = true;
  auto sa =
      qobject_cast<TabScroll *>(ui->tab->currentWidget())->getScrollArea();
  auto hb = sa->horizontalScrollBar();
  auto vb = sa->verticalScrollBar();

  int prevH = hb->value() - hb->maximum();
  int prevV = vb->value() - vb->maximum();

  (scalar - scaleFactor < 0.1) ? scalar = 0.1 : scalar -= scaleFactor;
  ui->zoomFactor->setText(QString::number(scalar));
  changeImage();
  for (auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
  }

  hb->setValue(prevH + hb->maximum());
  vb->setValue(prevV + vb->maximum());
}

void ImageFrame::mousePressEvent(QMouseEvent *event) {
  if (dropper) {
    auto point = event->pos();
    auto color = display.at<cv::Vec3b>(cv::Point{point.x(), point.y()});

    QString style = ImageTextObject::formatStyle(color);
    ui->dropper->setStyleSheet(style);

    emit colorSelected(color);
  }

  if (!keysPressed[Qt::Key_Control] && !dropper) {
    for (auto &obj : state->textObjects) {
      obj->deselect();
      if (!obj->isChanged) {
        obj->hide();
      }
    }
  }

  origin = event->pos();
  if (!rubberBand) {
    rubberBand = new QRubberBand{QRubberBand::Rectangle, this};
  }
  rubberBand->setGeometry(QRect{origin, QSize{}});
  rubberBand->show();
}

void ImageFrame::wheelEvent(QWheelEvent *event) {
  if (event->angleDelta().y() > 0 && (event->buttons() & Qt::MiddleButton))
    zoomIn();
  else if (event->angleDelta().y() < 0 && (event->buttons() & Qt::MiddleButton))
    zoomOut();
  else {
    QGraphicsView::wheelEvent(event);
  }
}

void ImageFrame::mouseMoveEvent(QMouseEvent *event) {
  rubberBand->setGeometry(QRect{origin, event->pos()}.normalized());
}

void ImageFrame::mouseReleaseEvent(QMouseEvent *event) {
  if (!this->isEnabled())
    return;
  if (state->textObjects.isEmpty())
    return;
  if (dropper) {
    dropper = false;
    return;
  }
  rubberBand->hide();
  auto dest = event->pos();

  QPoint tl, br;
  if (origin.x() < dest.x()) {
    tl.setX(origin.x());
    br.setX(dest.x());
  } else {
    tl.setX(dest.x());
    br.setX(origin.x());
  }

  if (origin.y() < dest.y()) {
    tl.setY(origin.y());
    br.setY(dest.y());
  } else {
    tl.setY(dest.y());
    br.setY(origin.y());
  }

  QPair<QPoint, QPoint> box{tl / scalar, br / scalar};
  inliers(box);
}

void ImageFrame::inliers(QPair<QPoint, QPoint> boundingBox) {
  auto a = boundingBox.first;
  auto b = boundingBox.second;

  for (auto &obj : state->textObjects) {
    if (!obj->isEnabled()) {
      continue;
    }
    auto tl = obj->topLeft;
    auto br = obj->bottomRight;

    bool xOverlap = !((br.x() < a.x()) || (tl.x() > b.x()));
    bool yOverlap = !((br.y() < a.y()) || (tl.y() > b.y()));

    if (xOverlap && yOverlap) {
      obj->selectHighlight();
    }
  }
}

void ImageFrame::setImage(QString imageName) {
  filepath = imageName;
  scalar = 1.0;

  QPixmap imagePixmap{imageName};

  scene->addPixmap(imagePixmap);
  scene->setSceneRect(imagePixmap.rect());
  scene->update();
  this->setScene(scene);

  this->setMinimumSize(imagePixmap.size());
  this->setMaximumSize(imagePixmap.size());

  extract();
  populateTextObjects();
}

void ImageFrame::showAll() {
  ui->zoomFactor->show();
  ui->zoomLabel->show();
  this->show();
}

void ImageFrame::extract(cv::Mat *mat) {
  if (mat) {
    mat->copyTo(state->matrix);
  } else {
    try {
      state->matrix =
          cv::Mat{cv::imread(filepath.toStdString(), cv::IMREAD_COLOR)};
    } catch (...) {
      qDebug() << "error reading image";
      return;
    }
  }

  if (state->matrix.empty()) {
    qDebug() << "empty mat";
    return;
  }

  QFuture<void> future = QtConcurrent::run(
      [&](cv::Mat matrix) -> void {
        emit processing();
        rawText = collect(matrix);
        state->matrix.copyTo(display);
        emit processing();
      },
      state->matrix);

  showAll();
}

void ImageFrame::connectSelection(ImageTextObject *obj) {
  connect(obj, &ImageTextObject::selection, this, [&]() {
    selection = qobject_cast<ImageTextObject *>(sender());
    for (auto &tempObj : state->textObjects) {
      if (tempObj == selection) {
        continue;
      }
      tempObj->setHighlightColor(YELLOW_HIGHLIGHT);
      tempObj->deselect();
    }

    auto color = selection->fontIntensity;
    QString style = ImageTextObject::formatStyle(color);
    ui->dropper->setStyleSheet(style);
  });
}

ImageFrame::State *&ImageFrame::getState() { return state; }

void ImageFrame::populateTextObjects() {
  QVector<ImageTextObject *> tempObjects;

  for (auto obj : state->textObjects) {
    ImageTextObject *temp = new ImageTextObject{this, *obj, ui, &state->matrix};
    temp->hide();

    tempObjects.push_back(temp);

    connectSelection(temp);
    delete obj;
  }

  state->textObjects = tempObjects;
}

QString ImageFrame::collect(cv::Mat &matrix) {
  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();

  const auto RIL = options->getRIL();
  const auto OEM = options->getOEM();
  const auto PSM = options->getPSM();
  auto data = options->getDataFile().toLocal8Bit();

  api->Init(nullptr, data.data(), OEM);
  api->SetPageSegMode(PSM);
  api->SetImage(matrix.data, matrix.cols, matrix.rows, 3, matrix.step);
  api->Recognize(0);

  QString text = QString{api->GetUTF8Text()};
  tesseract::ResultIterator *ri = api->GetIterator();

  int x1, y1, x2, y2;

  if (ri != 0) {
    do {
      QString word = ri->GetUTF8Text(RIL);
      ri->BoundingBox(RIL, &x1, &y1, &x2, &y2);
      QPoint p1{x1, y1}, p2{x2, y2};
      ImageTextObject *textObject = new ImageTextObject{nullptr};

      textObject->setText(word);
      textObject->lineSpace = QPair<QPoint, QPoint>{p1, p2};
      textObject->topLeft = p1;
      textObject->bottomRight = p2;
      state->textObjects.push_back(textObject);
    } while (ri->Next(RIL));
  }

  api->End();
  delete api;
  return text;
}

void ImageFrame::undoAction() {
  if (undo.empty() || isProcessing) {
    return;
  }
  for (auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  State *currState = new State{state->textObjects, cv::Mat{}, selection};

  state->matrix.copyTo(currState->matrix);

  redo.push(currState);
  state = undo.pop();

  for (auto &obj : state->textObjects) {
    obj->scaleAndPosition(ui->zoomFactor->text().toDouble());
    obj->show();
    obj->setDisabled(false);
  }
  selection = state->selection;

  changeImage();
}

void ImageFrame::redoAction() {
  if (redo.empty() || isProcessing) {
    return;
  }

  for (auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  undo.push(state);
  state = redo.pop();

  for (auto &obj : state->textObjects) {
    obj->scaleAndPosition(ui->zoomFactor->text().toDouble());
    obj->show();
    obj->setDisabled(false);
  }
  selection = state->selection;

  changeImage();
}

void ImageFrame::groupSelections() {
  if (!this->isEnabled())
    return;
  if (state->textObjects.isEmpty())
    return;

  QString contiguousStr{""};
  QPoint newTL{-1, -1}, newBR{-1, -1};

  QVector<ImageTextObject *> oldObjs = state->textObjects;
  State *oldState = new State{oldObjs, cv::Mat{}, selection};
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);

  ImageTextObject *textObject = new ImageTextObject{nullptr};
  QVector<ImageTextObject *> newTextObjects;

  int start = -1;
  for (auto i = 0; i < state->textObjects.size(); i++) {
    auto &obj = state->textObjects[i];

    if (obj->isSelected) {
      if (start == -1)
        start = i;

      if (newTL == QPoint{-1, -1})
        newTL = obj->topLeft;
      if (newBR == QPoint{-1, -1})
        newBR = obj->bottomRight;

      QString word = obj->getText();
      if (!word.endsWith('\n')) {
        word += " ";
      }
      contiguousStr += word;

      if (obj->topLeft.x() < newTL.x())
        newTL.setX(obj->topLeft.x());
      if (obj->topLeft.y() < newTL.y())
        newTL.setY(obj->topLeft.y());

      if (obj->bottomRight.x() > newBR.x())
        newBR.setX(obj->bottomRight.x());
      if (obj->bottomRight.y() > newBR.y())
        newBR.setY(obj->bottomRight.y());

      obj->reset();
    } else {
      newTextObjects.append(obj);
    }
  }

  textObject->setText(contiguousStr);
  textObject->lineSpace = QPair<QPoint, QPoint>{newTL, newBR};
  textObject->topLeft = newTL;
  textObject->bottomRight = newBR;

  textObject = new ImageTextObject{this, *textObject, ui, &state->matrix};
  textObject->reset();
  textObject->selectHighlight();
  textObject->scaleAndPosition(scalar);
  selection = textObject;

  // reindex after grouping
  QVector<ImageTextObject *> final;
  for (auto i = 0; i < newTextObjects.size(); i++) {
    if (i == start)
      final.append(textObject);
    final.append(newTextObjects[i]);
  }

  if (!newTextObjects.size() || start >= newTextObjects.size()) {
    final.append(textObject);
  }

  state->textObjects = final;
  connectSelection(textObject);
}

void ImageFrame::deleteSelection() {
  if (!selection) {
    qDebug() << "No selection";
    return;
  }
  if (!this->isEnabled())
    return;
  if (state->textObjects.isEmpty())
    return;

  auto idx = state->textObjects.indexOf(selection);
  selection->reset();
  QVector<ImageTextObject *> oldObjs = state->textObjects;
  State *oldState = new State{oldObjs, cv::Mat{}, selection};
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);

  state->textObjects.remove(idx);
}

void ImageFrame::move(QPoint shift) {
  if (!selection) {
    qDebug() << "No selection";
    return;
  }
  if (!this->isEnabled())
    return;
  if (state->textObjects.isEmpty())
    return;

  QVector<ImageTextObject *> oldObjs = state->textObjects;
  State *oldState = new State{oldObjs, cv::Mat{}, selection};
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);

  selection->fillBackground();
  selection->reposition(shift);
  changeText();

  auto curr = std::move(undo.pop()), prev = std::move(undo.pop());
  prev->textObjects = std::move(curr->textObjects);
  delete curr;

  undo.push(prev);
}
