/*
 * This file is part of DownScaler.
 * Copyright (C) 2023 Varvara Petrova
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FileExistsDialog.h"
#include "ui_FileExistsDialog.h"

#include <QFileInfo>
#include <QDateTime>
#include <QPixmap>
#include <QImageReader>
#include <QTransform>

FileExistsDialog::FileExistsDialog(QWidget *parent, QString source, QString destination) :
    QDialog(parent),
    ui(new Ui::FileExistsDialog)
{
    ui->setupUi(this);

    //initialize some labels
    QString fileName = destination.section('/', -1, -1);
    QString sourceFolder = source.section('/', -2, -2);
    QString destinationFolder = destination.section('/', -2, -2);

    ui->fileNameLabel->setText("Name: <b>" + fileName + "</b>");
    ui->sourceLabel->setText("File from <font color='#335eff'>" + sourceFolder + "</font>");
    ui->destinationLabel->setText("File existing in <font color='#335eff'>" + destinationFolder + "</font>");

    QFileInfo fileInfo(destination);
    QDateTime dateTime = fileInfo.lastModified(); //lastModified() or birthTime()
    QString dateStr = dateTime.toString("dd.MM.yyyy");
    QString timeStr = dateTime.toString("hh:mm:ss");
    double size = fileInfo.size()*1./1024;
    QString sizeStr;
    sizeStr.setNum(size, 'g', 4);
    QString info = dateStr + " " + timeStr + "\n" + sizeStr + " KB";
    ui->existingFileInfoLabel->setText(info);

    //inialize viewer-labels
    int len = 80;
    QPixmap leftPix = preparePixmap(source, len);
    QPixmap rightPix = preparePixmap(destination, len);
    ui->sourceImgLabel->setMaximumSize(len, len); //or leftPix.size()
    ui->destinationImgLabel->setMaximumSize(len, len); //or rightPix.size()
    ui->sourceImgLabel->setPixmap(leftPix);
    ui->destinationImgLabel->setPixmap(rightPix);

    connect(ui->overwriteButton, &QAbstractButton::clicked, this, [this] () {done(clickedOverwrite);});
    connect(ui->renameButton, &QAbstractButton::clicked, this, [this] () {done(clickedRename);});
    connect(ui->skipButton, &QAbstractButton::clicked, this, [this] () {done(clickedSkip);});

    setWindowTitle("File exists already");

}

FileExistsDialog::~FileExistsDialog()
{
    delete ui;
}

QPixmap FileExistsDialog::preparePixmap(const QString path, const int length)
{
    QPixmap pix = QPixmap(path);
    if (pix.isNull()) pix = QPixmap(":/img/img/icons8-image-file-80.png");

    QImageReader imReader(path);
    qreal theta = 0;
    switch (imReader.transformation()) {
    case QImageIOHandler::TransformationRotate90:
        theta = 90;
        break;
    case QImageIOHandler::TransformationRotate180:
        theta = 180;
        break;
    case QImageIOHandler::TransformationRotate270:
        theta = 270;
        break;
    }
    QSize pixSize = pix.size();
    qreal scaleFactor = getMin(length*1./pixSize.width(), length*1./pixSize.height());
    QTransform m;
    m.scale(scaleFactor, scaleFactor);
    if (theta) m.rotate(theta);
    pix = pix.transformed(m, Qt::SmoothTransformation);

    return pix;
}
