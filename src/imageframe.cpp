#include "../headers/imageframe.h"
#include "../headers/tabscroll.h"

ImageFrame::ImageFrame(QWidget *parent, QWidget *__tab, Ui::MainWindow *__ui,
                       Options *__options)
    : selection{nullptr}, isProcessing{false},
      stagedState{nullptr}, scalar{1.0}, scaleIncrement{0.1}, tab{__tab},
      rubberBand{nullptr}, scene{new QGraphicsScene(this)}, options{__options},
      ui{__ui}, spinner{nullptr}, dropper{false}, middleDown{false},
      zoomChanged{false}, hideAll{false}, state{new State} {

  qApp->installEventFilter(this);
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
  try {
    display = cv::Mat{};
    state->matrix.copyTo(display);
    cv::resize(display, display, cv::Size{}, scalar, scalar, cv::INTER_AREA);
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
  selection = new ImageTextObject{this, *selection, ui, &state->matrix};
  selection->setHighlightColor(GREEN_HIGHLIGHT);
  selection->isChanged = true;
  connectSelection(selection);

  state->textObjects.push_back(selection);
  State *oldState = new State{oldObjs, cv::Mat{}, oldSelection};
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
  auto offset = wh.y() - selection->bottomRight.y();
  selection->topLeft.setY(selection->topLeft.y() - offset);
  selection->bottomRight = wh;
  selection->bottomRight.setY(selection->bottomRight.y() - offset);
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
  sub = label.mid(k, label.size() - k);
  QPoint translateY{0, (i * dy)};

  QRect subrect{translateY + selection->topLeft,
                translateY + selection->bottomRight};
  p.drawText(subrect, sub, Qt::AlignLeft | Qt::AlignLeft);
  k = ++j;
  i++;

  p.restore();
  p.end();

  auto swap = img->convertToFormat(QImage::Format_RGB888);
  state->matrix = QImageToCvMat(swap);
  selection->isChanged = true;
  selection->showHighlight();
  selection->mat = &state->matrix;
  selection->fontIntensity = colorSelection;
  delete img;

  changeImage();
}

void ImageFrame::connections() {
  connect(this, &ImageFrame::customContextMenuRequested, this,
          [&](const QPoint &pos) {
            QMenu contextMenu{"Context menu", this};
            contextMenu.addAction(ui->actionUndo);
            contextMenu.addSeparator();
            contextMenu.addAction(ui->actionRedo_2);
            contextMenu.addSeparator();
            contextMenu.addAction(ui->actionHide_All);
            contextMenu.addSeparator();

            auto add = std::make_unique<QAction>("Add selection", this);
            QObject::connect(add.get(), &QAction::triggered, this,
                             &ImageFrame::highlightSelection);
            contextMenu.addAction(add.get());
            contextMenu.addSeparator();

            auto remove = std::make_unique<QAction>("Remove selection", this);
            QObject::connect(remove.get(), &QAction::triggered, this,
                             &ImageFrame::removeSelection);
            contextMenu.addAction(remove.get());

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
  for (const auto &obj : state->textObjects) {
    if (obj->isSelected) {
      obj->deselect();
      obj->hide();
      obj->isChanged = false;
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
        obj->setHighlightColor(BLUE_HIGHLIGHT);
        obj->isChanged = false;
      }
    }
    return;
  }

  for (const auto &obj : state->textObjects) {
    if (obj->getText().contains(query, Qt::CaseInsensitive)) {
      obj->setHighlightColor(PURPLE_HIGHLIGHT);
      obj->showHighlight();
      obj->isChanged = true;
    } else {
      obj->setHighlightColor(YELLOW_HIGHLIGHT);
    }
  }
}

void ImageFrame::highlightSelection() {
  if (!this->isEnabled())
    return;

  for (const auto &obj : state->textObjects) {
    if (obj->isSelected) {
      obj->setHighlightColor(YELLOW_HIGHLIGHT);
      obj->isChanged = true;
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
        selection->showHighlight();
        continue;
      }
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
  connect(obj, &ImageTextObject::selection, this, [&](ImageTextObject *curr) {
    selection = qobject_cast<ImageTextObject *>(sender());
    for (const auto &tempObj : state->textObjects) {
      if (tempObj == selection) {
        continue;
      }
      tempObj->setHighlightColor(YELLOW_HIGHLIGHT);
      tempObj->deselect();
    }

    if (!curr->colorSet) {
      selection->fontIntensity = ImageFrame::defaultColor;
    }

    QString style = ImageTextObject::formatStyle(selection->fontIntensity);
    ui->colorSelect->setStyleSheet(style);
  });
}

ImageFrame::State *&ImageFrame::getState() { return state; }

void ImageFrame::populateTextObjects() {
  QVector<ImageTextObject *> tempObjects;

  for (const auto &obj : state->textObjects) {
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
  if (undo.empty() || isProcessing) {
    return;
  }

  if (selection) {
    selection->setHighlightColor(YELLOW_HIGHLIGHT);
    selection->showHighlight();
  }
  for (const auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  State *currState = new State{state->textObjects, cv::Mat{}, selection};

  state->matrix.copyTo(currState->matrix);
  redo.push(currState);
  state = undo.pop();

  for (const auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
    obj->show();
    obj->setDisabled(false);
  }

  if ((selection = state->selection)) {
    selection->setHighlightColor(GREEN_HIGHLIGHT);
    selection->showHighlight();
  }

  changeImage();
}

void ImageFrame::redoAction() {
  if (redo.empty() || isProcessing) {
    return;
  }

  for (const auto &obj : state->textObjects) {
    obj->hide();
    obj->setDisabled(true);
  }

  undo.push(state);
  state = redo.pop();

  for (const auto &obj : state->textObjects) {
    obj->scaleAndPosition(scalar);
    obj->show();
    obj->setDisabled(false);
  }

  if ((selection = state->selection)) {
    selection->setHighlightColor(GREEN_HIGHLIGHT);
    selection->showHighlight();
  }

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
  selection = nullptr;
}

void ImageFrame::keyReleaseEvent(QKeyEvent *event) {
  keysPressed[event->key()] = false;
  if (!keysPressed[Qt::Key_Shift] && stagedState) {
    undo.push(stagedState);
    stagedState = nullptr;
  }
}

void ImageFrame::stageState() {
  undo.push(stagedState);
  stagedState = nullptr;
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

  if (!stagedState) {
    QVector<ImageTextObject *> oldObjs = state->textObjects;
    State *oldState = new State{oldObjs, cv::Mat{}, selection};
    state->matrix.copyTo(oldState->matrix);
    stagedState = oldState;
  }

  // update selection on move
  selection->hide();
  selection->setDisabled(true);
  state->textObjects.remove(state->textObjects.indexOf(selection));

  selection = new ImageTextObject{this, *selection, ui, &state->matrix};
  selection->setHighlightColor(GREEN_HIGHLIGHT);
  selection->isChanged = true;
  selection->showHighlight();
  connectSelection(selection);
  state->textObjects.push_back(selection);

  selection->reposition(shift);
  selection->scaleAndPosition(scalar);
  changeImage();
}

void ImageFrame::hideHighlights() {
  if ((hideAll = !hideAll)) {
    ui->actionHide_All->setText("Show All");
  } else {
    ui->actionHide_All->setText("Hide All");
  }

  for (const auto &obj : state->textObjects) {
    if (hideAll) {
      obj->hide();
      if (!obj->isChanged) {
        obj->deselect();
      }
    } else {
      obj->show();
    }
    obj->setDisabled(hideAll);
  }
}
