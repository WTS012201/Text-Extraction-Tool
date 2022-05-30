#include "../headers/imageframe.h"

ImageFrame::ImageFrame(QWidget* parent):
  scene{new QGraphicsScene(this)}, scalar{1.0}, scaleFactor{0.1}
{
  initUi(parent);
}

ImageFrame::~ImageFrame(){
  delete scene;
}

void ImageFrame::changeZoom(){
  scalar = (zoomEdit -> text()).toFloat();
  resize(originalSize * scalar);
}

void ImageFrame::setZoomFactor(QLineEdit* __zoomEdit){
  zoomEdit = __zoomEdit;
  connect(zoomEdit, &QLineEdit::editingFinished, this, &ImageFrame::changeZoom);
}

void ImageFrame::initUi(QWidget* parent){
  parent->setContentsMargins(0,0,0,0);
  this->setParent(parent);
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void ImageFrame::resize(QSize newSize){
  QPixmap image = QPixmap{currImage}.scaled(newSize);

  scene->clear();
  scene->addPixmap(image);
  scene->setSceneRect(image.rect());

  this->setScene(scene);
  this->setMinimumSize(image.size());
}

void ImageFrame::zoomIn(){
  scalar += scaleFactor;
  zoomEdit->setText(QString::number(scalar));
  resize(originalSize * scalar);
}

void ImageFrame::zoomOut(){
  scalar -= scaleFactor;
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

cv::Mat ImageFrame::QImageToMat(QImage image){
    cv::Mat mat;

    switch(image.format()){
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            mat = cv::Mat(
                image.height(), image.width(),
                CV_8UC4, (void*)image.constBits(),
                image.bytesPerLine()
            );
            break;
        case QImage::Format_RGB888:
            mat = cv::Mat(
                image.height(), image.width(),
                CV_8UC3, (void*)image.constBits(),
                image.bytesPerLine()
            );
            cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
            break;
        case QImage::Format_Grayscale8:
            mat = cv::Mat(
                image.height(), image.width(),
                CV_8UC1, (void*)image.constBits(),
                image.bytesPerLine()
            );
            break;
        default:
            break;
    }
    return mat;
}

void ImageFrame::setImage(QString imageName){
  currImage = imageName;
  scalar = 1.0;

  QPixmap image{imageName};

  scene->addPixmap(image);
  scene->setSceneRect(image.rect());
  originalSize = image.size();

  this->setScene(scene);
  this->setMinimumSize(image.size());
  extract();
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

  tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
  api->Init(nullptr, "eng", tesseract::OEM_LSTM_ONLY);
  api->SetPageSegMode(tesseract::PSM_AUTO);
  api->SetImage(matrix.data, matrix.cols, matrix.rows, 4, matrix.step);

  auto text = QString{api->GetUTF8Text()};
  qDebug() << text;

  api->End();
  delete api;
}
