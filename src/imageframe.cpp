#include "../headers/imageframe.h"
#include "../headers/tabscroll.h"
#include "headers/imagetextobject.h"
#include "qlistwidget.h"
#include <bits/chrono.h>

ImageFrame::ImageFrame(QWidget *parent, QWidget *__tab, Ui::MainWindow *__ui,
                       Options *__options)
    : selection{nullptr}, isProcessing{false}, isDrag{false}, hideAll{false},
      disableMove(false), stagedState{nullptr}, scalar{1.0},
      scaleIncrement{0.1}, tab{__tab}, rubberBand{nullptr},
      scene{new QGraphicsScene(this)}, options{__options}, ui{__ui},
      spinner{nullptr}, dropper{false}, middleDown{false}, zoomChanged{false},
      state{new State} {

  qApp->installEventFilter(this);
  initUi(parent);
  setWidgets();
  connections();
  ui->zoomFactor->deselect();
}

ImageFrame::~ImageFrame() {
  ui->listWidget->clear();

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

static inline cv::Mat QImageToCvMat(const QImage &inImage,
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
  if (isProcessing) {
    return;
  }

  for (const auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  State *oldState = new State{state->textObjects, cv::Mat{}, selection};

  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState); // scene dims
  redo = QStack<State *>{};
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
  selection = nullptr;

  extract(&mat);
  populateTextObjects();
}

void ImageFrame::changeImage(QImage *img) {
  if (state->matrix.empty()) {
    return;
  }

  try {
    display = cv::Mat{};
    state->matrix.copyTo(display);
    cv::resize(display, display, {}, scalar, scalar, cv::INTER_AREA);
  } catch (cv::Exception &e) {
    qDebug() << e.what() << "In changeImage:";
    delete scene;
    scene = new QGraphicsScene(this);
    return;
  }

  if (!img) {
    img = new QImage{(uchar *)display.data, display.cols, display.rows,
                     (int)display.step, QImage::Format_BGR888};
  }

  auto imagePixmap = QPixmap::fromImage(*img);

  delete scene;
  scene = new QGraphicsScene(this);
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
      static_cast<int>(selection->fontIntensity[2]),
      static_cast<int>(selection->fontIntensity[1]),
      static_cast<int>(selection->fontIntensity[0]),
  };

  QVector<ImageTextObject *> oldObjs = state->textObjects;
  state->textObjects.remove(state->textObjects.indexOf(selection));
  selection->hide();
  selection->setDisabled(true);
  auto *oldSelection = selection;
  selection =
      new ImageTextObject{this, *selection, ui, &state->matrix, options};
  state->selection = selection;
  selection->setHighlightColor(GREEN_HIGHLIGHT);
  selection->isPersistent = true;
  connectSelection(selection);

  state->textObjects.push_back(selection);
  State *oldState = new State{oldObjs, cv::Mat{}, oldSelection};
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);
  redo = QStack<State *>{};

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

  auto fontSizeStr = ui->fontSizeInput->text();
  if (fontSizeStr.isEmpty() || fontSizeStr.toInt() == 0) {
    fontSizeStr = QString::number(selection->fontSize);
    ui->fontSizeInput->setText(fontSizeStr);
  }
  int fontSize = fontSizeStr.toInt();

  QString label = ui->textEdit->toPlainText();
  selection->setText(label);

  QFont font{ui->fontBox->itemText(ui->fontBox->currentIndex()), fontSize};
  auto spacing = ui->letterSpacing->text();
  if (spacing.isEmpty() || spacing.toInt() == 0) {
    spacing = "0";
    ui->letterSpacing->setText(spacing);
  }
  font.setLetterSpacing(QFont::AbsoluteSpacing, spacing.toInt());
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

  // Scale back to normal size before resizing
  selection->scaleAndPosition(1);
  selection->topLeft.setY(selection->topLeft.y());
  selection->bottomRight = wh;
  QRect rect{selection->topLeft, selection->bottomRight};

  double newWidth = rect.width() * 1.0 / oldRect.width();
  double newHeight =
      (label.count("\n") + 1) * rect.height() * 1.0 / oldRect.height();
  selection->scaleAndPosition(newWidth, newHeight);
  // Scale to current size
  selection->scaleAndPosition(scalar);

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

  // last line
  sub = label.mid(k, label.size() - k);
  QPoint translateY{0, (i * dy)};

  QRect subrect{translateY + selection->topLeft,
                translateY + selection->bottomRight};
  p.drawText(subrect, sub, Qt::AlignLeft | Qt::AlignLeft);
  k = ++j;
  i++;
  // ------

  p.restore();
  p.end();

  auto swap = img->convertToFormat(QImage::Format_RGB888);
  state->matrix = QImageToCvMat(swap);
  selection->isPersistent = true;
  selection->showHighlight();
  selection->mat = &state->matrix;
  selection->fontIntensity = colorSelection;
  delete img;

  renderListView();
  changeImage();
}

void ImageFrame::connections() {
  connect(ui->listWidget, &QListWidget::itemPressed, this,
          [&](QListWidgetItem *) {
            for (auto i = 0; i < ui->listWidget->count(); i++) {
              const auto &item = ui->listWidget->item(i);
              const auto &obj = state->textObjects[i];
              if (item->isSelected()) {
                obj->selectHighlight();
              } else {
                obj->deselect();
              }
            }
          });
  connect(this, &ImageFrame::customContextMenuRequested, this,
          [&](const QPoint &pos) {
            QMenu contextMenu{"Context menu", this};

            for (const auto &a : ui->menuEdit->actions()) {
              contextMenu.addAction(a);
              contextMenu.addSeparator();
            }

            contextMenu.exec(mapToGlobal(pos));
          });

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
  connect(ui->dropper, &QPushButton::pressed, this, [&] {
    this->setCursor(Qt::CursorShape::CrossCursor);
    if (!hideAll)
      hideHighlights();
    dropper = true;
  });
  connect(ui->fontSizeInput, &QLineEdit::editingFinished, this, [&] {
    if (selection) {
      auto str = ui->fontSizeInput->text();
      if (str.isEmpty() || str.toInt() == 0) {
        return;
      }
      selection->fontSize = str.toInt();
    }
  });
  connect(ui->changeButton, &QPushButton::pressed, this,
          &ImageFrame::changeText);
  connect(ui->zoomFactor, &QLineEdit::editingFinished, this,
          &ImageFrame::changeZoom);
  connect(ui->find, &QLineEdit::editingFinished, this,
          &ImageFrame::findSubstrings);
}

void ImageFrame::removeSelection() {
  if (!this->isEnabled() || !tab) {
    return;
  }

  for (const auto &obj : state->textObjects) {
    if (obj->isSelected) {
      obj->deselect();
      obj->hide();
      obj->isPersistent = false;
      ui->listWidget->item(itemListMap[obj])->setSelected(false);
      if (obj == selection) {
        selection = nullptr;
      }
    }
  }
}

void ImageFrame::findSubstrings() {
  QString query = ui->find->text();

  if (query.isEmpty()) {
    for (const auto &obj : state->textObjects) {
      if (obj->getHighlightColor() == PURPLE_HIGHLIGHT) {
        if (obj->wasSelected) {
          obj->setHighlightColor(YELLOW_HIGHLIGHT);
          obj->wasSelected = false;
        } else {
          obj->setHighlightColor(BLUE_HIGHLIGHT);
          obj->isPersistent = false;
        }
      }
    }
    return;
  }

  for (const auto &obj : state->textObjects) {
    if (obj->getText().contains(query, Qt::CaseInsensitive)) {
      obj->wasSelected = obj->getHighlightColor() == YELLOW_HIGHLIGHT;
      obj->setHighlightColor(PURPLE_HIGHLIGHT);
      obj->showHighlight();
      obj->isPersistent = true;
    }
  }
}

void ImageFrame::highlightSelection() {
  if (!this->isEnabled() || !tab) {
    return;
  }

  for (const auto &obj : state->textObjects) {
    if (obj->isSelected) {
      obj->setHighlightColor(YELLOW_HIGHLIGHT);
      obj->isPersistent = true;
      ui->listWidget->item(itemListMap[obj])->setSelected(true);
      obj->deselect();
    }
  }
}

void ImageFrame::setWidgets() {
  this->setContextMenuPolicy(Qt::CustomContextMenu);
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

void ImageFrame::changeZoom() {
  if (!this->isEnabled())
    return;

  zoomChanged = true;
  double val = (ui->zoomFactor->text()).toDouble();
  if (val == 0) {
    val = scalar;
  } else {
    val /= 100;
  }

  scalar = (val < 0.1) ? 0.1 : val;
  scalar = (scalar > ZOOM_MAX) ? ZOOM_MAX : scalar;
  ui->zoomFactor->setText("");
  ui->zoomFactor->setPlaceholderText(QString::number(100 * scalar) + "%");
  changeImage();
  for (auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
  }
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

  (scalar + scaleIncrement > ZOOM_MAX) ? scalar = ZOOM_MAX
                                       : scalar += scaleIncrement;
  ui->zoomFactor->setText("");
  ui->zoomFactor->setPlaceholderText(QString::number(100 * scalar) + "%");
  changeImage();
  for (const auto &obj : state->textObjects) {
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

  (scalar - scaleIncrement < 0.1) ? scalar = 0.1 : scalar -= scaleIncrement;

  ui->zoomFactor->setText("");
  ui->zoomFactor->setPlaceholderText(QString::number(100 * scalar) + "%");
  changeImage();
  for (const auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
  }

  hb->setValue(prevH + hb->maximum());
  vb->setValue(prevV + vb->maximum());
}

void ImageFrame::mousePressEvent(QMouseEvent *event) {
  if (event->button() & Qt::RightButton) {
    return;
  }

  if (dropper) {
    auto point = event->pos();
    auto color = display.at<cv::Vec3b>(cv::Point{point.x(), point.y()});

    this->setCursor(Qt::CursorShape::ArrowCursor);
    hideHighlights();
    emit colorSelected(color);
  }

  if (!keysPressed[Qt::Key_Control] && !dropper) {
    for (const auto &obj : state->textObjects) {
      if (obj == selection) {
        selection->setHighlightColor(GREEN_HIGHLIGHT);
        if (!hideAll) {
          selection->showHighlight();
        }
        continue;
      }
      obj->deselect();
      if (!obj->isPersistent) {
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

bool ImageFrame::eventFilter(QObject *obj, QEvent *event) {
  if (obj->objectName() != this->objectName()) {
    return false;
  }

  if (dropper && event->type() == QEvent::MouseMove) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    auto pos = mouseEvent->pos();
    if (pos.x() < 0 || pos.x() >= display.cols) {
      return false;
    }
    if (pos.y() < 0 || pos.y() >= display.rows) {
      return false;
    }

    auto color = display.at<cv::Vec3b>(cv::Point{pos.x(), pos.y()});

    QString style = ImageTextObject::formatStyle(color);
    ui->colorSelect->setStyleSheet(style);
  }

  return false;
}

void ImageFrame::mouseMoveEvent(QMouseEvent *event) {
  auto pos = event->pos();

  rubberBand->setGeometry(QRect{origin, pos}.normalized());
}

void ImageFrame::mouseReleaseEvent(QMouseEvent *event) {
  if (state->textObjects.isEmpty() || !this->isEnabled())
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

  for (const auto &obj : state->textObjects) {
    if (!obj->isEnabled()) {
      continue;
    }
    auto tl = obj->topLeft;
    auto br = obj->bottomRight;

    bool xOverlap = !((br.x() < a.x()) || (tl.x() > b.x()));
    bool yOverlap = !((br.y() < a.y()) || (tl.y() > b.y()));

    if (xOverlap && yOverlap) {
      obj->selectHighlight();
      ui->listWidget->item(itemListMap[obj])->setSelected(true);
    } else if (!obj->isSelected && !obj->isPersistent) {
      ui->listWidget->item(itemListMap[obj])->setSelected(false);
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

cv::Scalar ImageFrame::defaultColor;

void ImageFrame::connectSelection(ImageTextObject *obj) {
  QObject::connect(obj, &ImageTextObject::selection, this, [&]() {
    if (isDrag) {
      isDrag = false;
      return;
    }

    selection = qobject_cast<ImageTextObject *>(sender());
    for (const auto &tempObj : state->textObjects) {
      if (tempObj == selection) {
        continue;
      }
      tempObj->setHighlightColor(YELLOW_HIGHLIGHT);
      tempObj->deselect();
    }

    QString style = ImageTextObject::formatStyle(selection->fontIntensity);
    ui->colorSelect->setStyleSheet(style);
  });

  Highlight *hb = obj->highlightButton;
  auto release = [&] {
    if (!selection) {
      return;
    }

    selection->setHighlightColor(YELLOW_HIGHLIGHT);
    selection->deselect();

    auto prev = selection;
    selection = qobject_cast<ImageTextObject *>(sender()->parent()->parent());

    if (!selection->drag && prev == selection) {
      move(QPoint{0, 0}, false);
      stageState(true);
    }
  };

  auto shift = [&](const QPoint &pos) {
    if (!selection) {
      return;
    }

    auto relPos = QWidget::mapFromGlobal(pos) / scalar;
    if (selection->drag) {
      move(relPos, true);
    }

    if (relPos.y() >= display.rows || relPos.x() >= display.cols) {
      selection->drag = false;
    } else if (relPos.y() < 0 || relPos.x() < 0) {
      selection->drag = false;
    }
  };

  QObject::connect(hb, &Highlight::drag, this, shift);
  QObject::connect(hb, &Highlight::released, this, release);
}

ImageFrame::State *&ImageFrame::getState() { return state; }

void ImageFrame::populateTextObjects() {
  QVector<ImageTextObject *> tempObjects;

  for (const auto &obj : state->textObjects) {
    ImageTextObject *temp =
        new ImageTextObject{this, std::move(*obj), ui, &state->matrix, options};
    temp->hide();
    tempObjects.push_back(temp);
    connectSelection(temp);
  }

  state->textObjects = tempObjects;
  renderListView();
  removeSelection();
}

QString ImageFrame::collect(const cv::Mat &matrix) {
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
      QString string = ri->GetUTF8Text(RIL);
      if (string.trimmed() == "") {
        continue;
      }
      ri->BoundingBox(RIL, &x1, &y1, &x2, &y2);

      x1 = x1 < 0 ? 0 : x1;
      x1 = x1 > matrix.cols ? matrix.cols - 1 : x1;
      x2 = x2 < 0 ? 0 : x2;
      x2 = x2 > matrix.cols ? matrix.cols - 1 : x2;

      y1 = y1 < 0 ? 0 : y1;
      y1 = y1 > matrix.cols ? matrix.cols - 1 : y1;
      y2 = y2 < 0 ? 0 : y2;
      y2 = y2 > matrix.cols ? matrix.cols - 1 : y2;

      QPoint p1{x1, y1}, p2{x2, y2};
      ImageTextObject *textObject = new ImageTextObject{nullptr};

      textObject->setText(string);
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
  if (undo.empty() || isProcessing || !tab) {
    return;
  }

  if (selection) {
    selection->setHighlightColor(YELLOW_HIGHLIGHT);
  }
  state->selection = selection;

  for (const auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  redo.push(state);
  state = undo.pop();

  for (const auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
    obj->setDisabled(false);

    if (obj->isPersistent) {
      obj->show();
    }
    obj->mat = &state->matrix;
  }

  if ((selection = state->selection)) {
    selection->setHighlightColor(GREEN_HIGHLIGHT);
    selection->showHighlight();
    selection->mat = &state->matrix;

    ui->textEdit->setText(selection->getText());
    ui->fontSizeInput->setText(QString::number(selection->fontSize));
    emit selection->highlightButton->clicked();
  }

  renderListView();
  changeImage();
}

void ImageFrame::redoAction() {
  if (redo.empty() || isProcessing || !tab) {
    return;
  }

  if (selection) {
    selection->setHighlightColor(YELLOW_HIGHLIGHT);
  }

  for (const auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  undo.push(state);
  state = redo.pop();

  for (const auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
    obj->setDisabled(false);
    if (obj->isPersistent) {
      obj->show();
    }
    obj->mat = &state->matrix;
  }

  if ((selection = state->selection)) {
    selection->setHighlightColor(GREEN_HIGHLIGHT);
    selection->showHighlight();
    selection->mat = &state->matrix;
    emit selection->highlightButton->clicked();
  }

  renderListView();
  changeImage();
}

void ImageFrame::groupSelections() {
  if (!this->isEnabled() || !tab) {
    return;
  }
  if (state->textObjects.isEmpty()) {
    return;
  }

  QString contiguousStr{""};
  QPoint newTL{-1, -1}, newBR{-1, -1};

  QVector<ImageTextObject *> oldObjs = state->textObjects;
  State *oldState = new State{oldObjs, cv::Mat{}, selection};
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);
  redo = QStack<State *>{};

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

  textObject =
      new ImageTextObject{this, *textObject, ui, &state->matrix, options};
  textObject->reset();
  textObject->selectHighlight();
  textObject->scaleAndPosition(scalar);
  /* selection = textObject; */

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
  renderListView();
}

void ImageFrame::renderListView() {
  ui->listWidget->clear();
  itemListMap.clear();
  objectFromItemsMap.clear();
  for (const auto &obj : state->textObjects) {
    ui->listWidget->addItem(obj->getText());
    const auto currItem = ui->listWidget->item(itemListMap[obj]);
    itemListMap[obj] = ui->listWidget->count() - 1;
    objectFromItemsMap[currItem] = obj;
    currItem->setSelected(obj->isSelected || obj->isPersistent);
  }
}

void ImageFrame::deleteSelection() {
  if (!selection || !this->isEnabled()) {
    qDebug() << "No selection";
    return;
  }
  if (state->textObjects.isEmpty()) {
    return;
  }

  auto idx = state->textObjects.indexOf(selection);
  selection->reset();
  QVector<ImageTextObject *> oldObjs = state->textObjects;
  State *oldState = new State{oldObjs, cv::Mat{}, selection};
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);
  redo = QStack<State *>{};

  state->textObjects.remove(idx);
  renderListView();
  selection = nullptr;
}

void ImageFrame::keyReleaseEvent(QKeyEvent *event) {
  keysPressed[event->key()] = false;
  if (!keysPressed[Qt::Key_Shift] && stagedState) {
    stageState();
  }
}

void ImageFrame::stageState(bool drag) {
  if (options->getFillMethod() == Options::NEIGHBOR) {
    stagedState->selection->fillBackground();
  }
  undo.push(stagedState);
  redo = QStack<State *>{};
  stagedState = nullptr;
  selection->unstageMove();
  isDrag = drag;

  if (options->getFillMethod() == Options::NEIGHBOR) {
    changeText();
    undo.pop();
  }
  changeImage();
}

void ImageFrame::configureDragSelection() {
  selection->hide();
  selection->setDisabled(true);
  state->textObjects.remove(state->textObjects.indexOf(selection));

  auto colors = selection->colorPalette;
  auto fontIntensity = selection->fontIntensity;
  selection = new ImageTextObject{this, std::move(*selection), ui,
                                  &state->matrix, options};
  selection->colorPalette = colors;
  selection->fontIntensity = fontIntensity;

  selection->setHighlightColor(GREEN_HIGHLIGHT);
  selection->isPersistent = true;
  selection->drag = false;
  selection->showHighlight();
  connectSelection(selection);
  state->textObjects.push_back(selection);
  state->selection = selection;
}

void ImageFrame::move(QPoint shift, bool drag) {
  if (!selection || !this->isEnabled()) {
    qDebug() << "No selection";
    return;
  }
  if (state->textObjects.isEmpty() || disableMove) {
    return;
  }

  static QPoint before{0, 0};
  if (!stagedState) {
    before = selection->topLeft;
    QVector<ImageTextObject *> oldObjs = state->textObjects;
    State *oldState = new State{oldObjs, cv::Mat{}, selection};
    state->matrix.copyTo(oldState->matrix);
    stagedState = oldState;
  }

  if (!drag) {
    configureDragSelection();
  }

  if (shift == QPoint{0, 0}) {
    stagedState->selection->scaleAndPosition(scalar);
    stagedState->selection->reposition(before - selection->topLeft);
    selection->scaleAndPosition(scalar);
    return;
  }

  if (drag) {
    selection->reposition(shift, false);
    selection->scaleAndPosition(scalar);
  } else {
    selection->reposition(shift);
    selection->scaleAndPosition(scalar);
  }
}

void ImageFrame::hideHighlights() {
  ui->hide->setIcon(QIcon(hideAll ? ":/img/hide.png" : ":/img/show.png"));
  ui->hide->setToolTip(hideAll ? "Hide all highlights" : "Show all highlights");
  if ((hideAll = !hideAll)) {
    ui->actionHide_All->setText("Show All (Ctrl + T)");
  } else {
    ui->actionHide_All->setText("Hide All (Ctrl + T)");
  }

  for (const auto &obj : state->textObjects) {
    if (hideAll) {
      obj->hide();
      if (!obj->isPersistent) {
        obj->deselect();
      }
    } else {
      obj->show();
    }
    obj->setDisabled(hideAll);
  }
}
