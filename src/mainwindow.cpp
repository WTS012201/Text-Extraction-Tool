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

//  auto edit = new QTextEdit{};
  auto pFrame = new ImageFrame(ui->scrollArea->widget());
  pFrame->setImage("galaxy.jpg");

//  auto splitter = new QSplitter{Qt::Vertical};
//  splitter->addWidget(pFrame);

//  splitter->setSizes(QList<int>{1000,800});

}
