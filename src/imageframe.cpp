#include "../headers/imageframe.h"

ImageFrame::ImageFrame(QWidget* parent, Ui::MainWindow* __ui, Options* options):
  scene{new QGraphicsScene(this)}, mode{tesseract::RIL_PARA},
  selection{nullptr}, ui{__ui}, scalar{1.0}, scaleFactor{0.1}
{
  initUi(parent);
  setWidgets();
  connections();
  setOptions(options);
}

ImageFrame::~ImageFrame(){
  delete scene;
  delete matrix;

  for(auto& obj : textObjects){
    delete obj;
  }
  if(ui->content->layout() != nullptr){
    while(ui->content->layout()->count() != 1){
      QLayoutItem* item = ui->content->layout()->takeAt(0);
      delete item->widget();
      delete item;
    }
  }
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

void ImageFrame::changeImage(){
  matrix->copyTo(display);
  cv::resize(
        display, display,
        cv::Size{},
        scalar, scalar,
        cv::INTER_CUBIC
        );
  QImage img{
    (uchar*)display.data,
        display.cols,
        display.rows,
        (int)display.step,
        QImage::Format_BGR888
  };
  auto imagePixmap = QPixmap::fromImage(img);
  // zooming uses image, fix this
  scene->addPixmap(imagePixmap);
  scene->setSceneRect(imagePixmap.rect());
  scene->update();
  this->setScene(scene);

  this->setMinimumSize(imagePixmap.size());
  this->setMaximumSize(imagePixmap.size());
}

void ImageFrame::changeText(){
  if(!selection){
    qDebug() << "No selection";
    return;
  }

  cv::Mat old;
  matrix->copyTo(old);
  undo.push(old);

  selection->fillText();
  matrix->copyTo(display);
  changeImage();
}

void ImageFrame::connections(){
  connect(ui->zoomFactor, &QLineEdit::editingFinished, this, &ImageFrame::changeZoom);
  connect(this, &ImageFrame::rawTextChanged, this, &ImageFrame::setRawText);
  connect(ui->highlightAll, &QPushButton::pressed, this, &ImageFrame::showHighlights);
  connect(ui->changeButton, &QPushButton::pressed, this, &ImageFrame::changeText);
}

void ImageFrame::setRawText(){
  populateTextObjects();
}

void ImageFrame::showHighlights(){
  QString text = ui->highlightAll->text();
  bool on = (text == "Highlight All: On");
  text = on ? "Highlight All: Off" : "Highlight All: On";
  ui->highlightAll->setText(text);

  for(const auto& obj : textObjects){
    on ? obj->hide() : obj->show();
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
  if(!keysPressed[Qt::Key_Control]){
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
  if(!keysPressed[Qt::Key_Control]){
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
  if(event->buttons() & Qt::LeftButton){
    zoomIn();
  }else if(event->buttons() & Qt::RightButton){
    zoomOut();
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
    matrix = new cv::Mat{cv::imread(filepath.toStdString(), cv::IMREAD_COLOR)};
  }catch(...){
    qDebug() << "error reading image";
    return;
  }

  if(matrix->empty()){
    qDebug() << "empty matrix";
    return;
  }  

  // transform matrix for better output here

  QFuture<void> future = QtConcurrent::run(
  [&](cv::Mat matrix) mutable -> void{
      rawText = collect(matrix);
  }, *matrix).then([&](){
    matrix->copyTo(display);
    emit rawTextChanged();
  });
  showAll();
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
  QVector<QPair<QString, Space*>> partials;
  int x1, y1, x2, y2;

  bool a1, a2, a3, a4, a5, a6;
  int a7, a8;
  ri->WordFontAttributes(&a1,&a2,&a3,&a4,&a5,&a6,&a7,&a8);

  QFont font = ui->textEdit->font();
  font.setPointSize(a7);
  ui->textEdit->setFont(font);
  ui->fontSizeInput->setText(QString::number(a7));

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

      partials.push_back(QPair<QString, Space*>{word, new Space{p1, p2}});
    } while (ri->Next(mode));
  }

  for(auto& partial : partials){
    ImageTextObject* textObject = new ImageTextObject{nullptr};
    textObject->setText(partial.first);
    textObject->addLineSpace(partial.second);
    textObjects.push_back(textObject);
  }

  api->End();
  delete api;
  return text;
}

void ImageFrame::populateTextObjects(){
  QVector<ImageTextObject*> tempObjects;
  auto layout = qobject_cast<QVBoxLayout*>(ui->content->layout());

  for(auto& obj : textObjects){
    ImageTextObject* temp = new ImageTextObject{this, *obj, ui, matrix};
    tempObjects.push_back(temp);
    temp->show();

    Content* content = new Content{};
    content->setTextObject(temp);

    auto contentLabel = content->getLabel();
    contentLabel->setText(temp->getText());
    contentLabel->setStyleSheet("border: 1px solid black");
    layout->insertWidget(layout->count() - 1, content);

    connect(temp, &ImageTextObject::selection, this, [&](){
      selection = qobject_cast<ImageTextObject*>(sender());
    });

    delete obj;
  }

  textObjects = tempObjects;
}

void ImageFrame::setMode(tesseract::PageIteratorLevel __mode){
  mode = __mode;
}

void ImageFrame::undoAction(){
  if(undo.empty()){
    return;
  }

  auto old = undo.pop();
  redo.push(*matrix);
  *matrix = old;
  changeImage();
}

void ImageFrame::redoAction(){
  if(redo.empty()){
    return;
  }

  auto old = redo.pop();
  undo.push(*matrix);
  *matrix = old;
  changeImage();
}
