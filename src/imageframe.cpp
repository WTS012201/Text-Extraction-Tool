#include "../headers/imageframe.h"

ImageFrame::ImageFrame(QWidget* parent, Ui::MainWindow* ui, Options* options):
  scene{new QGraphicsScene(this)}, scalar{1.0}, scaleFactor{0.1}
{

  mode = tesseract::RIL_PARA;
  initUi(parent);
  setWidgets(ui);
  buildConnections();
  setOptions(options);
}

void ImageFrame::keyPressEvent(QKeyEvent* event){
    keysPressed[event->key()] = true;
    if(event->key() & Qt::Key_Control){
      this->setCursor(Qt::CursorShape::PointingHandCursor);
    }
}

void ImageFrame::keyReleaseEvent(QKeyEvent* event){
    keysPressed[event->key()] = false;
    if(event->key() & Qt::Key_Control){
      this->setCursor(Qt::CursorShape::ArrowCursor);
    }
}


ImageFrame::~ImageFrame(){
  delete scene;
  delete image;

  for(auto& obj : textObjects){
    delete obj;
  }
  if(contentLayout != nullptr){
    while(contentLayout->count() != 1){
      QLayoutItem* item = contentLayout->takeAt(0);
      delete item->widget();
      delete item;
    }
  }
}

void ImageFrame::setOptions(Options* options){
  setMode(options->getPartialSelection());
}

void ImageFrame::changeZoom(){
  float val = (zoomEdit -> text()).toFloat();

  scalar = (val < 0.1) ? 0.1 : val;
  scalar = (scalar > 10.0) ? 10.0 : scalar;
  zoomEdit->setText(QString::number(scalar));
  resize(originalSize * scalar);
  for(auto& obj : textObjects){
    obj->scaleAndPosition(scalar);
  }
}

void ImageFrame::buildConnections(){
  connect(zoomEdit, &QLineEdit::editingFinished, this, &ImageFrame::changeZoom);
  connect(this, &ImageFrame::rawTextChanged, this, &ImageFrame::setRawText);
  connect(highlightAll, &QPushButton::pressed, this, &ImageFrame::showHighlights);
}

void ImageFrame::setRawText(){
  populateTextObjects();
}

void ImageFrame::showHighlights(){
  QString text = highlightAll->text();
  text = (text == "Highlight All: On") ? "Highlight All: Off" : "Highlight All: On";
  highlightAll->setText(text);

  for(const auto& obj : textObjects){
    obj->isHidden() ? obj->show() : obj->hide();
  }
}
void ImageFrame::setWidgets(Ui::MainWindow* ui){
  zoomEdit = ui->zoomFactor;
  zoomLabel = ui->label;
  contentLayout = qobject_cast<QVBoxLayout*>(ui->contentScrollLayout->layout());
  textEdit = ui->textEdit;
  fontSizeEdit = ui->fontSizeInput;
  highlightAll = ui->highlightAll;

  zoomEdit->hide();
  zoomLabel->hide();
}

void ImageFrame::initUi(QWidget* parent){
  parent->setContentsMargins(0,0,0,0);

  this->setParent(parent);
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->hide();
}

void ImageFrame::resize(QSize newSize){
  QPixmap image = QPixmap{currImage}.scaled(newSize);

  scene->clear();
  scene->addPixmap(image);
  scene->setSceneRect(image.rect());

  this->setScene(scene);
  this->setMinimumSize(image.size());
  this->setMaximumSize(image.size());
}

void ImageFrame::zoomIn(){
  if(!keysPressed[Qt::Key_Control]){
    return;
  }
  (scalar + scaleFactor > 10.0) ? scalar = 10.0 : scalar += scaleFactor;
  zoomEdit->setText(QString::number(scalar));
  resize(originalSize * scalar);
  for(auto& obj : textObjects){
    obj->scaleAndPosition(scalar);
  }
}

void ImageFrame::zoomOut(){
  if(!keysPressed[Qt::Key_Control]){
    return;
  }
  (scalar - scaleFactor < 0.1) ? scalar = 0.1 : scalar -= scaleFactor;
  zoomEdit->setText(QString::number(scalar));
  resize(originalSize * scalar);
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
  currImage = imageName;
  scalar = 1.0;

  image = new QImage{imageName};
  QPixmap imagePixmap{imageName};

  scene->addPixmap(imagePixmap);
  scene->setSceneRect(imagePixmap.rect());
  scene->update();
  this->setScene(scene);
  originalSize = imagePixmap.size();

  this->setMinimumSize(imagePixmap.size());
  this->setMaximumSize(imagePixmap.size());
  extract();
  populateTextObjects();
}

void ImageFrame::showAll(){
  zoomEdit->show();
  zoomLabel->show();
  this->show();
}


QVector<QString> ImageFrame::getLastWords(QVector<QString> lines){
  QVector<QString> lastWords;

  for(const auto& line : lines){
    int i = line.length();

    while(i > 0 && line.at(--i) != ' ');
    lastWords.push_back(line.mid(i ? i + 1 : 0, line.length()));
  }

  return lastWords;
}

QVector<QString> ImageFrame::getLines(QString text){
  QVector<QString> lines;

  while(!text.isEmpty()){
    auto pos = text.indexOf("\n");
    lines.push_back(text.mid(0, pos));
    text = text.mid(pos + 1, text.length());
  }

  return lines;
}

void ImageFrame::extract(){
  cv::Mat matrix;
  try{
    matrix = cv::imread(currImage.toStdString(), cv::IMREAD_COLOR);
  }catch(...){
    qDebug() << "error reading image";
    return;
  }

  if(matrix.empty()){
    qDebug() << "empty matrix";
    return;
  }  
  // transform matrix for better output here
  matrix.convertTo(matrix, -1, 2, 0);

  QFuture<void> future = QtConcurrent::run(
  [&](cv::Mat matrix) mutable -> void{
      rawText = collect(matrix);
  }, matrix).then([&](){emit rawTextChanged();});
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

  QFont font = textEdit->font();
  font.setPointSize(a7);
  textEdit->setFont(font);
  fontSizeEdit->setText(QString::number(a7));

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

  matrix = buildImageMatrix();
  for(auto& obj : textObjects){
    ImageTextObject* temp = new ImageTextObject{this, *obj, textEdit};
    temp->setImage(matrix);
    tempObjects.push_back(temp);
    temp->show();

    QLabel* contentLabel = new QLabel{};

    contentLabel->setText(temp->getText());
    contentLabel->setStyleSheet("border: 1px solid black");
    contentLayout->insertWidget(contentLayout->count() - 1, contentLabel);

    delete obj;
  }

  textObjects = tempObjects;
}

void ImageFrame::setMode(tesseract::PageIteratorLevel __mode){
  mode = __mode;
}

cv::Mat* ImageFrame::buildImageMatrix(){
    cv::Mat* mat = nullptr;

    switch(image->format()){
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            mat = new cv::Mat(
                image->height(), image->width(),
                CV_8UC4, (void*)image->constBits(),
                image->bytesPerLine()
            );
            break;
        case QImage::Format_RGB888:
            mat = new cv::Mat(
                image->height(), image->width(),
                CV_8UC3, (void*)image->constBits(),
                image->bytesPerLine()
            );
            cv::cvtColor(*mat, *mat, cv::COLOR_BGR2RGB);
            break;
        case QImage::Format_Grayscale8:
            mat = new cv::Mat(
                image->height(), image->width(),
                CV_8UC1, (void*)image->constBits(),
                image->bytesPerLine()
            );
            break;
        default:
            break;
    }
    return mat;
}

