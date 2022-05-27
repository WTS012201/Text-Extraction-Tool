#include "../headers/mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::MainWindow)
{
  initUi();
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::initUi(){
  ui->setupUi(this);
  iFrame = new ImageFrame(ui->scrollAreaWidgetContents);
  iFrame->setImage(":/img/galaxy.jpg");
  ui->scrollAreaWidgetContents->layout()->addWidget(iFrame);
  //image details next
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
  iFrame->setImage(selection.first());
}

