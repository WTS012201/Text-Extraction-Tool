﻿#include "../headers/mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_tabscroll.h"

// weird behavior on save, sometimes crashes

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), iFrame{nullptr},
      ui(new Ui::MainWindow), currTab{nullptr}, shift{1} {
  loadData();
  initUi();
  connections();

  ui->zoomFactor->deselect();
  this->setFocusPolicy(Qt::StrongFocus);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initUi() {
  ui->setupUi(this);
  options = new Options{this};
  colorMenu = new ColorTray{this};
}

void MainWindow::loadData() {
  QFileInfo check_file("eng.traineddata");

  if (check_file.exists() && check_file.isFile()) {
    return;
  }

  // write to .config/tfi later
  QFile file{"eng.traineddata"}, qrcFile(":/other/eng.traineddata");
  if (!qrcFile.open(QFile::ReadOnly | QFile::Text)) {
    qDebug() << "failed to open qrc file";
  }
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    qDebug() << "failed to write to file";
  }
  file.write(qrcFile.readAll());
}

void MainWindow::on_actionOptions_triggered() {
  options->setModal(true);
  if (options->exec() == QDialog::DialogCode::Rejected)
    return;

  auto PIL_selection = options->getPILSelection();
  for (auto i = 0; i < ui->tab->count(); i++) {
    auto tab = qobject_cast<TabScroll *>(ui->tab->widget(i));
    tab->iFrame->setMode(PIL_selection);
  }
}

void MainWindow::connections() {
  auto paste = new QShortcut{QKeySequence("Ctrl+V"), this};
  auto open = new QShortcut{QKeySequence("Ctrl+O"), this};
  auto save = new QShortcut{QKeySequence("Ctrl+S"), this};

  auto undo = new QShortcut{QKeySequence("Ctrl+Z"), this};
  auto redo = new QShortcut{QKeySequence("Ctrl+Shift+Z"), this};
  auto find = new QShortcut{QKeySequence("Ctrl+F"), this};

  auto add = new QShortcut{QKeySequence("Ctrl+A"), this};
  auto remove = new QShortcut{QKeySequence("Ctrl+R"), this};
  auto group = new QShortcut{QKeySequence("Ctrl+G"), this};
  auto __delete = new QShortcut{QKeySequence("Ctrl+D"), this};

  auto up = new QShortcut{QKeySequence("Shift+Up"), this};
  auto down = new QShortcut{QKeySequence("Shift+Down"), this};
  auto left = new QShortcut{QKeySequence("Shift+Left"), this};
  auto right = new QShortcut{QKeySequence("Shift+Right"), this};

  zoomIn = new QShortcut{QKeySequence("Ctrl+="), this};
  zoomOut = new QShortcut{QKeySequence("Ctrl+-"), this};
  clipboard = QApplication::clipboard();

  connect(ui->fontBox, SIGNAL(activated(int)), this, SLOT(fontSelected()));
  QObject::connect(ui->moveFactor, &QLineEdit::editingFinished, this, [&] {
    QString text = ui->moveFactor->text();
    if (quint8 num = text.toInt()) {
      shift = qMax(num, shift);
    }
  });

  QObject::connect(ui->tab, &QTabWidget::currentChanged, this, [&](int idx) {
    if (idx == -1 || !currTab)
      return;
    currTab->setDisabled(true);
    currTab = qobject_cast<TabScroll *>(ui->tab->currentWidget());
    currTab->setEnabled(true);
    iFrame = currTab->iFrame;

    emit switchConnections();
  });

  QObject::connect(
      ui->tab->tabBar(), &QTabBar::tabCloseRequested, this, [&](int idx) {
        if (!iFrame || iFrame->isProcessing)
          return; // to prevent closing on concurrent
        delete ui->tab->widget(idx);

        if (ui->tab->count() > 0) {
          currTab = qobject_cast<TabScroll *>(ui->tab->currentWidget());
          currTab->setEnabled(true);
          iFrame = currTab->iFrame;
          iFrame->setEnabled(true);

          emit switchConnections();
        } else {
          currTab = nullptr;
          iFrame = nullptr;
        }
      });

  QObject::connect(find, &QShortcut::activated, this, [&] {
    ui->find->setFocus();
    if (iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });

  QObject::connect(up, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->move(QPoint{0, -shift});
    }
  });

  QObject::connect(down, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->move(QPoint{0, shift});
    }
  });

  QObject::connect(left, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->move(QPoint{-shift, 0});
    }
  });

  QObject::connect(right, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->move(QPoint{shift, 0});
    }
  });

  QObject::connect(add, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->keysPressed[Qt::Key_Control] = false;
      iFrame->highlightSelection();
    }
  });

  QObject::connect(remove, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->keysPressed[Qt::Key_Control] = false;
      iFrame->removeSelection();
    }
  });

  QObject::connect(group, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->keysPressed[Qt::Key_Control] = false;
      iFrame->groupSelections();
    }
  });

  QObject::connect(__delete, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->keysPressed[Qt::Key_Control] = false;
      iFrame->deleteSelection();
    }
  });

  QObject::connect(open, &QShortcut::activated, this, [&] {
    on_actionOpen_Image_triggered();
    if (iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });

  QObject::connect(save, &QShortcut::activated, this, [&] {
    on_actionSave_Image_triggered();
    if (iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });

  QObject::connect(undo, &QShortcut::activated, this, [&] {
    on_actionUndo_triggered();
    if (iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });

  QObject::connect(redo, &QShortcut::activated, this, [&] {
    on_actionRedo_2_triggered();
    if (iFrame)
      iFrame->keysPressed[Qt::Key_Control] = false;
  });

  QObject::connect(ui->fontSizeInput, &QLineEdit::textChanged, this,
                   &MainWindow::fontSizeChanged);

  QObject::connect(ui->color, &QPushButton::clicked, this,
                   &MainWindow::colorTray);

  QObject::connect(paste, &QShortcut::activated, this, &MainWindow::pastImage);

  QObject::connect(this, &MainWindow::switchConnections, this, [&] {
    QObject::connect(zoomIn, &QShortcut::activated, iFrame,
                     &ImageFrame::zoomIn);
    QObject::connect(zoomOut, &QShortcut::activated, iFrame,
                     &ImageFrame::zoomOut);
    ui->zoomFactor->setText(QString::number(iFrame->scalar));
    ui->textEdit->setText(iFrame->selection ? iFrame->selection->getText()
                                            : "");
  });
}

void MainWindow::pastImage() {
  if (!iFrame)
    on_actionOpen_Image_triggered(true);

  const QMimeData *mimeData = clipboard->mimeData();

  if (mimeData->hasImage()) {
    QImage img = qvariant_cast<QImage>(mimeData->imageData());
    if (!img.isNull()) {
      iFrame->pasteImage(&img);
    }
  }
}

void MainWindow::colorTray() {
  if (!iFrame)
    return;
  if (!iFrame->selection)
    return;

  if (iFrame->selection) {
    colorMenu->setColor(iFrame->selection->fontIntensity);
  }

  colorMenu->setModal(true);

  QVector<QPushButton *> buttons;
  for (const auto &color : iFrame->selection->colorPalette) {
    QPushButton *cb = new QPushButton{};
    QObject::connect(cb, &QPushButton::pressed, colorMenu,
                     [&]() { colorMenu->setColor(color); });

    colorMenu->palette->addWidget(cb);
    QString style = "background-color: rgb(";
    style += QString::number(color[2]) + ',';
    style += QString::number(color[1]) + ',';
    style += QString::number(color[0]) + ')';
    cb->setStyleSheet(style);
    buttons.push_back(cb);
  }

  if (colorMenu->exec() == QDialog::DialogCode::Rejected) {
    for (const auto &cb : buttons) {
      colorMenu->palette->removeWidget(cb);
      delete cb;
    }
    return;
  }

  cv::Scalar scalar{
      static_cast<double>(colorMenu->color.blue()),
      static_cast<double>(colorMenu->color.green()),
      static_cast<double>(colorMenu->color.red()),
  };

  iFrame->selection->fontIntensity = scalar;
  for (const auto &cb : buttons) {
    colorMenu->palette->removeWidget(cb);
    delete cb;
  }
}

void MainWindow::fontSelected() {
  if (!iFrame)
    return;

  QString text = ui->fontBox->currentText();
  ui->textEdit->setFont(QFont{text, ui->textEdit->font().pointSize()});
}

void MainWindow::fontSizeChanged() {
  if (!iFrame)
    return;

  QFont font = ui->textEdit->font();
  font.setPointSize(ui->fontSizeInput->text().toInt());
  ui->textEdit->setFont(font);
}

void MainWindow::on_actionUndo_triggered() { iFrame->undoAction(); }

void MainWindow::on_actionRedo_2_triggered() { iFrame->redoAction(); }

void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (!iFrame)
    return;

  iFrame->keysPressed[event->key()] = true;
  if (event->key() & Qt::SHIFT) {
    this->setCursor(Qt::CursorShape::PointingHandCursor);
  }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
  if (!iFrame)
    return;

  iFrame->keysPressed[event->key()] = false;
  if (event->key() & Qt::SHIFT) {
    this->setCursor(Qt::CursorShape::ArrowCursor);
  }
}

void MainWindow::on_actionOpen_Image_triggered(bool paste) {
  QStringList selection, filters{"*.png *.jpeg *.jpg"};

  if (!paste) {
    QFileDialog dialog(this);
    dialog.setNameFilters(filters);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    if (!dialog.exec())
      return;

    selection = dialog.selectedFiles();

    for (const auto &file : selection) {
      loadImage(file);
    }

    /* const int LOAD_LIMIT = 2; */
    /* QVector<ImageFrame *> load; */
    /* for (const auto &file : selection) { */
    /*   auto loadIt = load.begin(); */
    /*   while (load.size() >= LOAD_LIMIT) { */
    /*  */
    /*     if (!(**loadIt).isProcessing) */
    /*       loadIt = load.erase(loadIt); */
    /*     if (loadIt == load.end()) */
    /*       loadIt = load.begin(); */
    /*  */
    /*     loadIt++; */
    /*   } */
    /*  */
    /*   loadImage(file); */
    /*   load.push_back(iFrame); */
    /* } */

    emit switchConnections();
    return;
  }

  TabScroll *tabScroll = new TabScroll{ui->tab};
  auto tabUi = tabScroll->getUi();
  ui->tab->addTab(tabScroll, "Untitled");

  iFrame =
      new ImageFrame(tabUi->scrollAreaWidgetContents, tabScroll, ui, options);
  tabUi->scrollHorizontalLayout->addWidget(iFrame);
  iFrame->getState() = new ImageFrame::State;
  tabScroll->iFrame = iFrame;
  currTab = tabScroll;
  ui->tab->setCurrentWidget(tabScroll);

  emit switchConnections();
}

void MainWindow::loadImage(QString fileName) {
  QString name = fileName;
  if (name.contains('/')) {
    name = name.remove(0, name.lastIndexOf('/') + 1);
  }

  if (name.contains('\\')) {
    name = name.remove(0, name.lastIndexOf('\\') + 1);
  }

  TabScroll *tabScroll = new TabScroll{ui->tab};
  auto tabUi = tabScroll->getUi();
  ui->tab->addTab(tabScroll, name);

  iFrame =
      new ImageFrame(tabUi->scrollAreaWidgetContents, tabScroll, ui, options);
  tabUi->scrollHorizontalLayout->addWidget(iFrame);
  iFrame->setImage(fileName);
  tabScroll->iFrame = iFrame;
  currTab = tabScroll;
  ui->tab->setCurrentWidget(tabScroll);
}

void MainWindow::on_actionSave_Image_triggered() {
  if (!iFrame || ui->tab->count() == 0)
    return;
  QString fName = ui->tab->tabText(ui->tab->currentIndex());
  auto saveFile = QFileDialog::getSaveFileName(0, "Save file", fName, ".png");
  //                                               ".png;;.jpeg;;.jpg");
  cv::Mat image = iFrame->getImageMatrix();
  cv::imwrite(saveFile.toStdString() + ".png", image);
}
