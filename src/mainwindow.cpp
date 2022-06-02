#include "../headers/mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), iFrame{nullptr}
  , ui(new Ui::MainWindow)
{
  loadData();
  initUi();
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::initUi(){
  ui->setupUi(this);
}

void MainWindow::loadData(){
  QFileInfo check_file("eng.traineddata");

  if (check_file.exists() && check_file.isFile()) {
    return;
  }
  QFile file{"eng.traineddata"}, qrcFile(":/other/eng.traineddata");
  if(!qrcFile.open(QFile::ReadOnly | QFile::Text)){
    qDebug() << "failed to open qrc file";
  }
  if(!file.open(QFile::WriteOnly | QFile::Text)){
    qDebug() << "failed to write to file";
  }
  file.write(qrcFile.readAll());
}

void MainWindow::on_actionOpen_Image_triggered()
{
  QFileDialog dialog(this);
  QStringList selection, filters{"*.png *.jpeg *.jpg"};

  dialog.setNameFilters(filters);
  dialog.setFileMode(QFileDialog::ExistingFile);
  if (!dialog.exec()){
      return;
  }
  selection = dialog.selectedFiles();
  if(iFrame){
    delete iFrame;
  }
  iFrame = new ImageFrame(ui->scrollAreaWidgetContents, ui);
  ui->scrollAreaWidgetContents->layout()->addWidget(iFrame);
  iFrame->setImage(selection.first());
}

