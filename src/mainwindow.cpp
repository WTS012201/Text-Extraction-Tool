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
  ui->scrollAreaWidgetContents->setContentsMargins(0,0,0,0);

  QVBoxLayout* layout = new QVBoxLayout{ui->scrollAreaWidgetContents};

  iFrame = new ImageFrame(ui->scrollAreaWidgetContents);
  layout->addWidget(iFrame);
//  iFrame->setImage(":/img/galaxy.jpg");


//  auto view = new QGraphicsView{ui->scrollAreaWidgetContents};
//  view->setMinimumSize(10000,10000);



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

