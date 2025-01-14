﻿#include "../headers/tabscroll.h"
#include "ui_tabscroll.h"

TabScroll::TabScroll(QWidget *parent, ImageFrame *__iFrame)
    : QWidget(parent), iFrame(__iFrame), ui(new Ui::TabScroll) {
  ui->setupUi(this);
}

TabScroll::~TabScroll() {
  delete ui;
  delete iFrame;
}

QScrollArea *TabScroll::getScrollArea() { return ui->scrollArea; }

Ui::TabScroll *TabScroll::getUi() { return ui; }
