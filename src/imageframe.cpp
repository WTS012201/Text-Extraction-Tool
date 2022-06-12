#include "../headers/imageframe.h"

ImageFrame::ImageFrame(QWidget* parent, Ui::MainWindow* __ui, Options* options):
  rubberBand{nullptr}, scene{new QGraphicsScene(this)},
  mode{tesseract::RIL_PARA}, selection{nullptr}, ui{__ui}, state{new State},
  scalar{1.0},
  scaleFactor{0.1}
{
  initUi(parent);
  setWidgets();
  connections();
  setOptions(options);
}

ImageFrame::~ImageFrame(){
  for(auto& obj : textObjects){
    delete obj;
  }
  delete state;
  delete scene;
}

void ImageFrame::setMode(tesseract::PageIteratorLevel __mode){
  mode = __mode;
}

void ImageFrame::setOptions(Options* options){
  setMode(options->getPartialSelection());
}

void ImageFrame::changeZoom(){
  float val = (ui->zoomFactor->text()).toFloat();

  scalar = (val < 0.1) ? 0.1 : val;
  scalar = (scalar > 10.0) ? 10.0 : scalar;
  ui->zoomFactor->setText(QString::number(scalar));
  changeImage();
  for(auto& obj : textObjects){
    obj->scaleAndPosition(scalar);
  }
}

void ImageFrame::changeImage(QImage* img){
  state->matrix.copyTo(display);
  cv::resize(
        display, display,
        cv::Size{},
        scalar, scalar,
        cv::INTER_CUBIC
        );

  if(!img){
    img = new QImage{
        (uchar*)display.data,
        display.cols,
        display.rows,
        (int)display.step,
        QImage::Format_BGR888
    };
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

void ImageFrame::changeText(){
  if(!selection){
    qDebug() << "No selection";
    return;
  }

  State* oldState = new State;
  oldState->changed = new ImageTextObject{this, *selection, ui, &state->matrix};
  oldState->changed->hide();
  state->matrix.copyTo(oldState->matrix);
  undo.push(oldState);
  textObjects.push_back(oldState->changed);

  selection->fillBackground();
  state->matrix.copyTo(display);
  QImage* img = new QImage{
      (uchar*)display.data,
      display.cols,
      display.rows,
      (int)display.step,
      QImage::Format_BGR888
  };
  QPainter p;
  if(!p.begin(img)){
    qDebug() << "error with painter";
    return;
  }
  int fontSize = ui->fontSizeInput->text().toInt();
  QString label = ui->textEdit->toPlainText();
  QFont font{"Times", fontSize};
  p.setFont(font);

  if(label.isEmpty()) return;
  QFontMetrics fm{p.font()};
  float x = fm.horizontalAdvance(label);
  float y = fm.height();

  QPoint wh{selection->topLeft.x() + (int)x, selection->topLeft.y() + (int)y};
  QRect rect{selection->topLeft, wh};
  QRect oldRect{selection->topLeft, selection->bottomRight};

  selection->bottomRight = wh;
  float newWidth = rect.width()*1.0/oldRect.width();
  float newHeight = rect.height()*1.0/oldRect.height();
  selection->scaleAndPosition(newWidth, newHeight);

  p.save();
  p.drawText(rect, label, Qt::AlignTop | Qt::AlignLeft);
  p.restore();
  p.end();

  state->matrix = cv::Mat{
        img->height(), img->width(),
        CV_8UC3, (void*)img->constBits(),
        (size_t)img->bytesPerLine()
  };
  selection->mat = &state->matrix;
  delete img;

  changeImage();
  state->changed = selection;
}

void ImageFrame::connections(){
  connect(ui->zoomFactor, &QLineEdit::editingFinished, this, &ImageFrame::changeZoom);
  connect(this, &ImageFrame::rawTextChanged, this, &ImageFrame::setRawText);
  connect(ui->highlightAll, &QPushButton::pressed, this, &ImageFrame::highlightSelection);
  connect(ui->changeButton, &QPushButton::pressed, this, &ImageFrame::changeText);

  connect(ui->removeSelection, &QPushButton::pressed, this, [=](...){
    for(auto& obj : textObjects){
      if(obj->isSelected){
        obj->deselect();
        obj->hide();
        obj->isChanged = false;
      }
    }
  });
}

void ImageFrame::setRawText(){
  populateTextObjects();
}

void ImageFrame::highlightSelection(){
  for(auto& obj : textObjects){
    if(obj->isSelected){
      obj->isChanged = true;
      obj->deselect();
    }
  }
}


void ImageFrame::setWidgets(){
  ui->zoomFactor->hide();
  ui->zoomLabel->hide();
}

void ImageFrame::initUi(QWidget* parent){
  parent->setContentsMargins(0,0,0,0);

  this->setParent(parent);
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->hide();
}

void ImageFrame::zoomIn(){
  if(!keysPressed[Qt::SHIFT]){
    return;
  }
  (scalar + scaleFactor > 10.0) ? scalar = 10.0 : scalar += scaleFactor;
  ui->zoomFactor->setText(QString::number(scalar));
  changeImage();
  for(auto& obj : textObjects){
    obj->scaleAndPosition(scalar);
  }
}

void ImageFrame::zoomOut(){
  if(!keysPressed[Qt::SHIFT]){
    return;
  }
  (scalar - scaleFactor < 0.1) ? scalar = 0.1 : scalar -= scaleFactor;
  ui->zoomFactor->setText(QString::number(scalar));
  changeImage();
  for(auto& obj : textObjects){
    obj->scaleAndPosition(scalar);
  }
}

void ImageFrame::mousePressEvent(QMouseEvent* event) {
  if(!keysPressed[Qt::Key_Control]){
    for(auto& obj : textObjects){
      obj->deselect();
      if(!obj->isChanged){
        obj->hide();
      }
    }
  }
  origin = event->pos();
  if(!rubberBand){
    rubberBand = new QRubberBand{QRubberBand::Rectangle, this};
  }
  rubberBand->setGeometry(QRect{origin, QSize{}});
  rubberBand->show();

  if(event->buttons() & Qt::LeftButton){
    zoomIn();
  }else if(event->buttons() & Qt::RightButton){
    zoomOut();
  }
}

void ImageFrame::mouseMoveEvent(QMouseEvent *event){
    rubberBand->setGeometry(QRect(origin, event->pos()).normalized());
}

void ImageFrame::mouseReleaseEvent(QMouseEvent *event){
  rubberBand->hide();
  auto dest = event->pos();

  QPoint tl, br;
  if(origin.x() < dest.x()){
    tl.setX(origin.x());
    br.setX(dest.x());
  }else{
    tl.setX(dest.x());
    br.setX(origin.x());
  }

  if(origin.y() < dest.y()){
    tl.setY(origin.y());
    br.setY(dest.y());
  }else{
    tl.setY(dest.y());
    br.setY(origin.y());
  }

  QPair<QPoint, QPoint> box{tl / scalar, br / scalar};
  inSelection(box);
}

void ImageFrame::inSelection(QPair<QPoint, QPoint> boundingBox){
  auto a = boundingBox.first;
  auto b = boundingBox.second;

  for(auto& obj : textObjects){
    if(!obj->isEnabled()){
      continue;
    }
    auto tl = obj->topLeft;
    auto br = obj->bottomRight;

    bool xOverlap = !((br.x() < a.x()) || (tl.x() > b.x()));
    bool yOverlap = !((br.y() < a.y()) || (tl.y() > b.y()));

    if(xOverlap && yOverlap){
      obj->selectHighlight();
    }
  }
}

void ImageFrame::setImage(QString imageName){
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

void ImageFrame::showAll(){
  ui->zoomFactor->show();
  ui->zoomLabel->show();
  this->show();
}

void ImageFrame::extract(){
  try{
    state->matrix = cv::Mat{cv::imread(filepath.toStdString(), cv::IMREAD_COLOR)};
  }catch(...){
    qDebug() << "error reading image";
    return;
  }

  if(state->matrix.empty()){
    qDebug() << "empty matrix";
    return;
  }  

  QFuture<void> future = QtConcurrent::run(
  [&](cv::Mat matrix) mutable -> void{
      rawText = collect(matrix);
  }, state->matrix).then([&](){
    state->matrix.copyTo(display);
    emit rawTextChanged();
  });
  showAll();
}

void ImageFrame::populateTextObjects(){
  QVector<ImageTextObject*> tempObjects;

  for(auto obj : textObjects){
    ImageTextObject* temp = new ImageTextObject{this, *obj, ui, &state->matrix};
    temp->hide();

    tempObjects.push_back(temp);

    connect(temp, &ImageTextObject::selection, this, [&](...){
      selection = qobject_cast<ImageTextObject*>(sender());
      for(auto& tempObj : textObjects){
        if(tempObj == selection){
          continue;
        }
        tempObj->deselect();
      }
    });

    delete obj;
  }

  textObjects = tempObjects;
}

QString ImageFrame::collect(
    cv::Mat& matrix
    ){
  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();

  api->Init(nullptr, "eng", tesseract::OEM_DEFAULT);
  api->SetPageSegMode(tesseract::PSM_AUTO);
  api->SetImage(matrix.data, matrix.cols, matrix.rows, 3, matrix.step);
  api->Recognize(0);

  QString text = QString{api->GetUTF8Text()};
  tesseract::ResultIterator* ri = api->GetIterator();

  typedef QPair<QPoint, QPoint> Space;
  int x1, y1, x2, y2;

  if (ri != 0) {
    do {
      QString word = ri->GetUTF8Text(mode);
      ri->BoundingBox(mode, &x1, &y1, &x2, &y2);
      QPoint p1{x1, y1}, p2{x2, y2};

      while(word.endsWith('\n') || word.endsWith(' ')){
        word = word.remove(word.length() - 1, word.length());
      }
      if(word.isEmpty()){
        continue;
      }

      ImageTextObject* textObject = new ImageTextObject{nullptr};
      textObject->setText(word);
      textObject->addLineSpace(new Space{p1, p2});
      textObjects.push_back(textObject);
    } while (ri->Next(mode));
  }

  api->End();
  delete api;
  return text;
}

void ImageFrame::undoAction(){
  if(undo.empty()){
    return;
  }

  State* currState = new State;
  state->matrix.copyTo(currState->matrix);
  currState->changed = state->changed;
  state->changed->setDisabled(true);
  state->changed->hide();
  redo.push(currState);

  state = undo.pop();
  state->changed->showHighlights();
  state->changed->setDisabled(false);
  state->changed->isChanged = true;
  state->changed->deselect();
  changeImage();
}

void ImageFrame::redoAction(){
  if(redo.empty()){
    return;
  }

  State* currState = new State;
  state->matrix.copyTo(currState->matrix);
  currState->changed = state->changed;
  state->changed->setDisabled(true);
  state->changed->hide();
  undo.push(currState);

  state = redo.pop();
  state->changed->showHighlights();
  state->changed->setDisabled(false);
  state->changed->isChanged = true;
  state->changed->deselect();
  changeImage();
}

