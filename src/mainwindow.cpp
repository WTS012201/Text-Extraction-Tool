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

  auto iFrame = new ImageFrame(ui->graphicsView);
  iFrame->setImage("galaxy.jpg");

  //image details next
}
