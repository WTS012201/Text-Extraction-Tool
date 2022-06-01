#include "../headers/imageframe.h"

ImageFrame::ImageFrame(QWidget* parent, Ui::MainWindow* ui):
  scene{new QGraphicsScene(this)}, scalar{1.0}, scaleFactor{0.1}
{
  initUi(parent);
  setWidgets(ui);
  buildConnections();
}

ImageFrame::~ImageFrame(){
  delete scene;
}

void ImageFrame::changeZoom(){
  float val = (zoomEdit -> text()).toFloat();

  scalar = (val < 0.1) ? 0.1 : val;
  scalar = (scalar > 10.0) ? 10.0 : scalar;
  zoomEdit->setText(QString::number(scalar));
  resize(originalSize * scalar);
}

void ImageFrame::buildConnections(){
  connect(zoomEdit, &QLineEdit::editingFinished, this, &ImageFrame::changeZoom);
  connect(this, &ImageFrame::rawTextChanged, this, &ImageFrame::setRawText);
}

void ImageFrame::setRawText(){
  populateTextObjects();
}
void ImageFrame::setWidgets(Ui::MainWindow* ui){
  zoomEdit = ui->zoomFactor;
  progressBar = ui->progressBar;
  zoomLabel = ui->label;

  zoomEdit->hide();
  zoomLabel->hide();
  progressBar->hide();
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
  (scalar + scaleFactor > 10.0) ? scalar = 10.0 : scalar += scaleFactor;
  zoomEdit->setText(QString::number(scalar));
  resize(originalSize * scalar);
}

void ImageFrame::zoomOut(){
  (scalar - scaleFactor < 0.1) ? scalar = 0.1 : scalar -= scaleFactor;
  zoomEdit->setText(QString::number(scalar));
  resize(originalSize * scalar);
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

  QPixmap image{imageName};

  scene->addPixmap(image);
  scene->setSceneRect(image.rect());
  scene->update();
  this->setScene(scene);

  originalSize = image.size();
//  this->parentWidget()->setMaximumSize(image.size());
  this->setMinimumSize(image.size());
  this->setMaximumSize(image.size());
  extract();
//  emit rawTextChanged();
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

    while(i > 0 && line.at(i -= 1) != ' ');
    lastWords.push_back(line.mid(i + 1, line.length()));
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

void ImageFrame::test(){

}
void ImageFrame::extract(){
  cv::Mat matrix;
  try{
    matrix = cv::imread(currImage.toStdString(), cv::IMREAD_COLOR);
  }catch(...){
    return;
  }

  if(matrix.empty()){
    qDebug() << "empty matrix";
    return;
  }  
  // transform matrix for darker images
  // upscale/downscale here maybe
  matrix.convertTo(matrix, -1, 2, 0);
  progressBar->show();

  QFuture<void> future = QtConcurrent::run(
  [&](cv::Mat matrix) mutable -> void{
      rawText = collect(matrix);
  }, matrix).then([&](){emit rawTextChanged();});

  progressBar->hide();
  showAll();
}

QString ImageFrame::collect(cv::Mat& matrix){
  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();

  api->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY);
  api->SetPageSegMode(tesseract::PSM_AUTO);
  api->SetImage(matrix.data, matrix.cols, matrix.rows, 4, matrix.step);
  api->Recognize(0);

  auto text = QString{api->GetUTF8Text()};
  tesseract::ResultIterator* ri = api->GetIterator();
  tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
  QString line{""};
  ImageTextObject* textObject = new ImageTextObject{nullptr};
  QPair<QPoint, QPoint> space;
  QPoint prevPoint;
  bool newLine = true;
  int x1, y1, x2, y2;
  auto newBlock = [&](){
    textObject->setText(line);
    line = "";
    space.second = prevPoint;
    textObject->addLineSpace(space);
    newLine = true;
    textObjects.push_back(textObject);
    textObject = new ImageTextObject{nullptr};
  };

  if (ri != 0) {
      do {
        QString word = ri->GetUTF8Text(level);
//        float conf = ri->Confidence(level);

        ri->BoundingBox(level, &x1, &y1, &x2, &y2);
        qDebug() << word;

        if(newLine){
          space.first = QPoint{x1, y1};
          newLine = false;
          qDebug() << "JOINED";
        }

        // new line in block
        if(word == "+"){
          line += '\n';
          space.second = prevPoint;
          textObject->addLineSpace(space);
          newLine = true;

        } else if(word.contains('\n') || word.contains(' ')){
          newBlock();
          qDebug() << "NEW BLOCK";
        } else{
          prevPoint = QPoint{x2, y2};
          line += word + " ";
        }
      } while (ri->Next(level));
    qDebug() << "NEW BLOCK";
    newBlock();
  }
  api->End();
  delete api;
  qDebug() << text;
  return text;
}

void ImageFrame::populateTextObjects(){
  QVector<ImageTextObject*> tempObjects;

  // since widgets were created in another thread
  // have to rebuild them in main thread
  for(auto& obj : textObjects){
    ImageTextObject* temp = new ImageTextObject{this, *obj};
    tempObjects.push_back(temp);
    temp->show();

    delete obj;
  }

  textObjects = tempObjects;
}
