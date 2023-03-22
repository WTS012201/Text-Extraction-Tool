#include "../headers/mainwindow.h"
#include "headers/imagetextobject.h"
#include "qboxlayout.h"
#include "ui_mainwindow.h"
#include "ui_tabscroll.h"
#include <tesseract/publictypes.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), iFrame{nullptr},
      ui(new Ui::MainWindow), currTab{nullptr}, shift{1} {
  initUi();
  scanSettings();
  connections();

  ui->zoomFactor->deselect();
  this->setFocusPolicy(Qt::StrongFocus);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::initUi() {
  ui->setupUi(this);
  ui->dropper->setIcon(QIcon(":/img/dropper.png"));
  ui->hide->setIcon(QIcon(":/img/hide.png"));
  options = new Options{this};
  colorMenu = new ColorTray{this};
}

void MainWindow::scanSettings() {
  QString path = QDir::homePath() + "/.config/tfi/";
  QDir::setCurrent(path);
  settings = new QSettings{"settings.ini", QSettings::IniFormat};

  if (!QFileInfo{"settings.ini"}.exists()) {
    writeSettings(true);
  } else {
    readSettings();
  }

  const auto dataDir = options->getDataDir();
  if (QDir{dataDir}.exists()) {
    QDir::setCurrent(options->getDataDir());
  } else {
    qDebug() << "Data directory " << dataDir
             << " doesn't exists, using default path " << path;
    options->setDataDir(path);
    QDir::setCurrent(path);
  }

  const auto dataFile = options->getDataFile() + ".traineddata";
  if (!QFileInfo{dataFile}.exists()) {
    qDebug() << "Data file " << dataFile
             << " doesn't exists, using default file "
             << "eng.traineddata";
    options->setDataFile("eng");

    QFile file{"eng.traineddata"}, qrcFile{":/other/eng.traineddata"};
    if (!qrcFile.open(QFile::ReadOnly | QFile::Text)) {
      qDebug() << "Failed to open qrc eng.traineddata";
      return;
    }
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
      qDebug() << "Failed to open eng.traineddata";
      return;
    }
    file.write(qrcFile.readAll());
  }
}

void MainWindow::readSettings() {
  auto RIL = settings->value("tesseract/RIL", options->getRIL()).toInt();
  auto OEM = settings->value("tesseract/OEM", options->getOEM()).toInt();
  auto PSM = settings->value("tesseract/PSM", options->getPSM()).toInt();
  auto dataDir =
      settings->value("tesseract/DataDir", options->getDataDir()).toString();
  auto dataFile =
      settings->value("tesseract/DataFile", options->getDataFile()).toString();
  auto fillMethod =
      settings->value("highlight/FillMethod", options->getFillMethod()).toInt();

  options->setRIL(static_cast<tesseract::PageIteratorLevel>(RIL));
  options->setOEM(static_cast<tesseract::OcrEngineMode>(OEM));
  options->setPSM(static_cast<tesseract::PageSegMode>(PSM));
  options->setDataDir(dataDir);
  options->setDataFile(dataFile);
  options->setFillMethod((Options::fillMethod)fillMethod);
}

void MainWindow::writeSettings(bool __default) {
  if (__default) {
    options->setRIL(tesseract::RIL_WORD);
    options->setOEM(tesseract::OEM_DEFAULT);
    options->setPSM(tesseract::PSM_AUTO);
    options->setFillMethod(Options::INPAINT);
    options->setDataDir(QDir::homePath() + "/.config/tfi/");
    options->setDataFile("eng");
  }

  settings->setValue("tesseract/RIL", options->getRIL());
  settings->setValue("tesseract/OEM", options->getOEM());
  settings->setValue("tesseract/PSM", options->getPSM());
  settings->setValue("tesseract/DataDir", options->getDataDir());
  settings->setValue("tesseract/DataFile", options->getDataFile());
  settings->setValue("highlight/FillMethod", options->getFillMethod());
  settings->sync();
}

void MainWindow::on_actionOptions_triggered() {
  options->setModal(true);
  if (options->exec() == QDialog::DialogCode::Rejected)
    return;

  writeSettings(false);
  readSettings();
  scanSettings();
}

void MainWindow::on_hide_clicked() {
  if (iFrame) {
    iFrame->hideHighlights();
  }
}

void MainWindow::connections() {
  const auto paste = new QShortcut{QKeySequence("Ctrl+V"), this};
  const auto open = new QShortcut{QKeySequence("Ctrl+O"), this};
  const auto save = new QShortcut{QKeySequence("Ctrl+S"), this};

  const auto undo = new QShortcut{QKeySequence("Ctrl+Z"), this};
  const auto redo = new QShortcut{QKeySequence("Ctrl+Shift+Z"), this};
  const auto find = new QShortcut{QKeySequence("Ctrl+F"), this};

  const auto add = new QShortcut{QKeySequence("Ctrl+A"), this};
  const auto remove = new QShortcut{QKeySequence("Ctrl+R"), this};
  const auto group = new QShortcut{QKeySequence("Ctrl+G"), this};
  const auto __delete = new QShortcut{QKeySequence("Ctrl+D"), this};

  const auto up = new QShortcut{QKeySequence("Shift+Up"), this};
  const auto down = new QShortcut{QKeySequence("Shift+Down"), this};
  const auto left = new QShortcut{QKeySequence("Shift+Left"), this};
  const auto right = new QShortcut{QKeySequence("Shift+Right"), this};
  const auto toggleHigh = new QShortcut{QKeySequence("Ctrl+T"), this};

  zoomIn = new QShortcut{QKeySequence("Ctrl+="), this};
  zoomOut = new QShortcut{QKeySequence("Ctrl+-"), this};
  clipboard = QApplication::clipboard();

  connect(ui->fontBox, SIGNAL(activated(int)), this, SLOT(fontSelected()));
  QObject::connect(ui->moveFactor, &QLineEdit::editingFinished, this, [&] {
    QString text = ui->moveFactor->text();
    shift = (text.toInt() > 10) ? 10 : text.toInt();
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
          return; // to prevent closing on concurrent tess process
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

  QObject::connect(toggleHigh, &QShortcut::activated, this, [&] {
    if (iFrame) {
      iFrame->hideHighlights();
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

    ui->zoomFactor->setText("");
    ui->zoomFactor->setPlaceholderText(QString::number(100 * iFrame->scalar) +
                                       "%");
    ui->textEdit->setText(iFrame->selection ? iFrame->selection->getText()
                                            : "");

    auto fill = [&](cv::Scalar color) {
      if (iFrame && iFrame->selection) {
        QString style = ImageTextObject::formatStyle(color);
        ImageFrame::defaultColor = color;

        iFrame->selection->colorSet = true;
        iFrame->selection->fontIntensity = color;
        ui->colorSelect->setStyleSheet(style);
      }
    };

    QObject::connect(iFrame, &ImageFrame::colorSelected, this, fill);
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
    auto intensity = iFrame->selection->colorPalette.first();
    colorMenu->setColor(intensity);
  }

  colorMenu->setModal(true);

  QVector<QPushButton *> buttons;
  QVector<QHBoxLayout *> layouts;
  QHBoxLayout *curr;

  for (const auto &color : iFrame->selection->colorPalette) {
    if (layouts.empty() || curr->count() % 5 == 0) {
      layouts.push_back(curr = new QHBoxLayout);
      colorMenu->palette->addLayout(curr);
    }
    curr = layouts.last();

    QPushButton *cb = new QPushButton{};
    QObject::connect(cb, &QPushButton::pressed, colorMenu,
                     [&]() { colorMenu->setColor(color); });

    curr->addWidget(cb);
    QString style = ImageTextObject::formatStyle(color);
    cb->setStyleSheet(style);
    buttons.push_back(cb);
  }

  if (colorMenu->exec() == QDialog::DialogCode::Rejected) {
    for (const auto &cb : buttons) {
      colorMenu->palette->removeWidget(cb);
      delete cb;
    }
    for (const auto &lay : layouts) {
      colorMenu->palette->removeItem(lay);
      delete lay;
    }
    return;
  }

  cv::Scalar color{
      static_cast<double>(colorMenu->color.blue()),
      static_cast<double>(colorMenu->color.green()),
      static_cast<double>(colorMenu->color.red()),
  };

  for (const auto &cb : buttons) {
    colorMenu->palette->removeWidget(cb);
    delete cb;
  }
  for (const auto &lay : layouts) {
    colorMenu->palette->removeItem(lay);
    delete lay;
  }

  emit iFrame->colorSelected(color);
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

void MainWindow::on_actionRemove_Selection_Ctrl_R_triggered() {
  if (iFrame)
    iFrame->removeSelection();
}

void MainWindow::on_actionAdd_Selection_Ctrl_A_triggered() {
  if (iFrame)
    iFrame->highlightSelection();
}

void MainWindow::on_actionGroup_Ctrl_G_triggered() {
  if (iFrame)
    iFrame->groupSelections();
}

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
  if (!iFrame->keysPressed[Qt::Key_Shift] && iFrame->stagedState) {
    iFrame->stageState();
  }
}

void MainWindow::on_actionOpen_Image_triggered(bool paste) {
  QStringList selection, filters{"*.png *.jpeg *.jpg"};

  if (!paste) {
    QDir::setCurrent(QDir::homePath());
    QFileDialog dialog(this);
    dialog.setNameFilters(filters);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    if (!dialog.exec()) {
      return;
    }

    selection = dialog.selectedFiles();

    for (const auto &file : selection) {
      loadImage(file);
    }

    QDir::setCurrent(options->getDataDir());
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
  QDir::setCurrent(QDir::homePath());
  auto saveFile = QFileDialog::getSaveFileName(0, "Save file", fName, ".png");
  //                                               ".png;;.jpeg;;.jpg");
  cv::Mat image = iFrame->getImageMatrix();
  cv::imwrite(saveFile.toStdString() + ".png", image);
  QDir::setCurrent(options->getDataDir());
}

void MainWindow::on_actionHide_All_triggered() { iFrame->hideHighlights(); }
