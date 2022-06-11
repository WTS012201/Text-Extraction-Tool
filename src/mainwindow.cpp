#include "../headers/mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), iFrame{nullptr}
  , ui(new Ui::MainWindow)
{
  loadData();
  initUi();
  connections();

  iFrame = new ImageFrame(ui->scrollAreaWidgetContents, ui, options);
  ui->scrollHorizontalLayout->addWidget(iFrame);
  iFrame->setImage("/home/will/screenshots/use_this.png");
}

MainWindow::~MainWindow()
{
  delete ui;
  delete undo;
  delete redo;
}

void MainWindow::initUi(){
  ui->setupUi(this);
  options = new Options{this};
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
  iFrame = new ImageFrame(ui->scrollAreaWidgetContents, ui, options);
  ui->scrollHorizontalLayout->addWidget(iFrame);
  iFrame->setImage(selection.first());
}


void MainWindow::on_actionOptions_triggered()
{
//  options->setView(0);
  options->setModal(true);
  if(options->exec() == QDialog::DialogCode::Rejected)
    return;
}

void MainWindow::connections(){
  undo = new QShortcut{QKeySequence("Ctrl+Z"), this};
  redo = new QShortcut{QKeySequence("Ctrl+Shift+Z"), this};

  connect(ui->fontBox, SIGNAL(activated(int)), this, SLOT(fontSelected()));
  QObject::connect(
        ui->fontSizeInput,
        &QLineEdit::returnPressed,
        this,
        &MainWindow::fontSizeChanged
        );

  QObject::connect(undo, &QShortcut::activated, this, [&](){
    on_actionUndo_triggered();
  });
  QObject::connect(redo, &QShortcut::activated, this, [&](){
    on_actionRedo_2_triggered();
  });
}

void MainWindow::fontSelected(){
  QString text = ui->fontBox->currentText();
  ui->textEdit->setFont(QFont{text, ui->textEdit->font().pointSize()});
}

void MainWindow::fontSizeChanged(){
  QFont font = ui->textEdit->font();
  font.setPointSize(ui->fontSizeInput->text().toInt());
  ui->textEdit->setFont(font);
}

void MainWindow::on_actionUndo_triggered(){
  if(!iFrame){
    return;
  }
  iFrame->undoAction();
}

void MainWindow::on_actionRedo_2_triggered(){
  if(!iFrame){
    return;
  }
  iFrame->redoAction();
}

void MainWindow::keyPressEvent(QKeyEvent* event){
    iFrame->keysPressed[event->key()] = true;
    if(event->key() & Qt::SHIFT){
      this->setCursor(Qt::CursorShape::PointingHandCursor);
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event){
    iFrame->keysPressed[event->key()] = false;
    if(event->key() & Qt::SHIFT){
      this->setCursor(Qt::CursorShape::ArrowCursor);
    }
}
