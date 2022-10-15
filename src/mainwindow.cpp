#include "../headers/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_tabscroll.h"

// paste wont work on fresh start
// first tab deletion weird behavior

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent), iFrame{nullptr},
    deleting{false}, ui(new Ui::MainWindow)
  , currTab{nullptr}
{
  loadData();
  initUi();
  connections();

//  on_actionOpen_Image_triggered(true);
}

MainWindow::~MainWindow(){
  delete ui;
  delete undo;
  delete redo;
}

void MainWindow::initUi(){
  ui->setupUi(this);
  options = new Options{this};
  colorMenu = new ColorTray{this};
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

void MainWindow::on_actionOptions_triggered(){
//  options->setView(0);
  options->setModal(true);
  if(options->exec() == QDialog::DialogCode::Rejected)
    return;
}

void MainWindow::connections(){
  paste = new QShortcut{QKeySequence("Ctrl+V"), this};
  open = new QShortcut{QKeySequence("Ctrl+O"), this};
  save = new QShortcut{QKeySequence("Ctrl+S"), this};
  undo = new QShortcut{QKeySequence("Ctrl+Z"), this};
  redo = new QShortcut{QKeySequence("Ctrl+Shift+Z"), this};
  clipboard = QApplication::clipboard();

  connect(ui->fontBox, SIGNAL(activated(int)), this, SLOT(fontSelected()));
  QObject::connect(ui->tab, &QTabWidget::currentChanged, this, [&](int idx){
    if(idx < 1 || !currTab || deleting){
      return;
    }
    currTab->setDisabled(true);
    currTab = qobject_cast<TabScroll*>(ui->tab->currentWidget());
    currTab->setEnabled(true);
    iFrame = currTab->iFrame;
  });
  QObject::connect(ui->fontSizeInput, &QLineEdit::textChanged,
                   this, &MainWindow::fontSizeChanged);

  QObject::connect(ui->color, &QPushButton::clicked, this, &MainWindow::colorTray);
  // PASTE DOESNT ACTIVATE IF NO TAB
  QObject::connect(paste, &QShortcut::activated, this, &MainWindow::pastImage);
  QObject::connect(open, &QShortcut::activated, this, [&](){
    on_actionOpen_Image_triggered();
    if(iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });
  QObject::connect(save, &QShortcut::activated, this, [&](){
    on_actionSave_Image_triggered();
    if(iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });
  QObject::connect(undo, &QShortcut::activated, this, [&](){
    on_actionUndo_triggered();
    if(iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });
  QObject::connect(redo, &QShortcut::activated, this, [&](){
    on_actionRedo_2_triggered();
    if(iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });

  QObject::connect(
  ui->tab->tabBar(), &QTabBar::tabCloseRequested, this, [&](int idx){
    if(!iFrame)
      return;

    deleting = true;
    ui->tab->tabBar()->removeTab(idx);
    if(currTab){
      delete currTab;
      currTab = nullptr;
      deleting = false;
    } else{
      return;
    }

    if(ui->tab->count() > 1){
      currTab = qobject_cast<TabScroll*>(ui->tab->currentWidget());
    } else{
      currTab = nullptr;
    }

  });
}

void MainWindow::colorTray(){
  if(!iFrame)
    return;

  colorMenu->setModal(true);
  if(iFrame->selection){
    colorMenu->setColor(iFrame->selection->fontIntensity);
  }
  if(colorMenu->exec() == QDialog::DialogCode::Rejected)
    return;

  cv::Scalar scalar{
    (double)colorMenu->color.blue(),
    (double)colorMenu->color.green(),
    (double)colorMenu->color.red(),
  };

  iFrame->selection->fontIntensity = scalar;
}

void MainWindow::pastImage(){
  if(!iFrame)
    on_actionOpen_Image_triggered(true);
  const QMimeData *mimeData = clipboard->mimeData();

  if(mimeData->hasImage()){
    QImage img = qvariant_cast<QImage>(mimeData->imageData());
    if(!img.isNull()){
      iFrame->pasteImage(&img);
    }
  }
}

void MainWindow::fontSelected(){
  if(!iFrame)
    return;

  QString text = ui->fontBox->currentText();
  ui->textEdit->setFont(QFont{text, ui->textEdit->font().pointSize()});
}

void MainWindow::fontSizeChanged(){
  if(!iFrame)
    return;

  QFont font = ui->textEdit->font();
  font.setPointSize(ui->fontSizeInput->text().toInt());
  ui->textEdit->setFont(font);
}

void MainWindow::on_actionUndo_triggered(){
  iFrame->undoAction();
}

void MainWindow::on_actionRedo_2_triggered(){
  iFrame->redoAction();
}

void MainWindow::keyPressEvent(QKeyEvent* event){
  if(!iFrame)
    return;

  iFrame->keysPressed[event->key()] = true;
  if(event->key() & Qt::SHIFT){
    this->setCursor(Qt::CursorShape::PointingHandCursor);
  }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event){
  if(!iFrame)
    return;

  iFrame->keysPressed[event->key()] = false;
  if(event->key() & Qt::SHIFT){
    this->setCursor(Qt::CursorShape::ArrowCursor);
  }
}

void MainWindow::on_actionOpen_Image_triggered(bool file){
  QStringList selection, filters{"*.png *.jpeg *.jpg"};
  QString fName = "Untitled";

  if(!file){
    QFileDialog dialog(this);
    dialog.setNameFilters(filters);
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (!dialog.exec()){
        return;
    }
    selection = dialog.selectedFiles();

    fName = selection.first();
    if(fName.contains('/')){
      fName = fName.remove(0, fName.lastIndexOf('/') + 1);
    }

    if(fName.contains('\\')){
      fName = fName.remove(0, fName.lastIndexOf('\\') + 1);
    }
  }

  TabScroll* tabScroll = new TabScroll{ui->tab};
  auto tabUi = tabScroll->getUi();
  ui->tab->addTab(tabScroll, fName);

  iFrame = new ImageFrame(tabUi->scrollAreaWidgetContents, ui, options);
  tabUi->scrollHorizontalLayout->addWidget(iFrame);
  if(!file){
    iFrame->setImage(selection.first());
  } else{
    iFrame->getState() = new ImageFrame::State;
  }
  tabScroll->iFrame = iFrame;
  currTab = tabScroll;
  // emits change
  ui->tab->setCurrentWidget(tabScroll);
}

void MainWindow::on_actionSave_Image_triggered(){
  QString fName = ui->tab->tabText(ui->tab->currentIndex());
  auto saveFile = QFileDialog::getSaveFileName(0,"Save file", fName, ".png");
//                                               ".png;;.jpeg;;.jpg");
  cv::Mat image = iFrame->getImageMatrix();
  cv::imwrite(saveFile.toStdString() + ".png", image);
}
