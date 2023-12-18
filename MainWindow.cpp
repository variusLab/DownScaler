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

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FileExistsDialog.h"

#include <QDebug>
#include <QWidget>
#include <QFileDialog>
#include <QProgressDialog>
#include <QProgressBar>
#include <QImageReader>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QSize>
#include <QMessageBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_settings("VariusLab", "DownScaler"),
      m_selectedFilesList(new QStringList()),
      m_filesCount(0),
      m_destinationFolderPath(QString()),
      m_corruptedFilesList(QStringList()),
      m_customizedName(QString()),
      m_prefix(QString()),
      m_suffix(QString()),
      m_offset(0),
      m_msgBox(new QMessageBox(this))

{
    ui->setupUi(this);    
    /* Read a path from settings */
    m_sourceFolderPath = m_settings.value("/Settings/openDir", "C:/Users").toString();    

    /* Initialize progressDialog */
    m_progressDialog = new QProgressDialog(this);
    m_progressDialog->setWindowModality(Qt::WindowModal);
    m_progressDialog->setWindowTitle("Progress");
    m_progressDialog->setCancelButton(nullptr);
    m_progressDialog->setMinimumWidth(400);
    m_progressDialog->reset();    

    //'FileDialogs' panel: select files and destination folder, overwrite the existing files checkBox
    connect(ui->selectFilesButton, &QPushButton::clicked, this, &MainWindow::updateFilesList);
    connect(ui->selectDestinationButton, &QPushButton::clicked, this, &MainWindow::updateDestinationFolder);
    connect(this, &MainWindow::filesCountChanged, this, &MainWindow::updateAutoIncrementalPart);
    connect(ui->overWriteCheckBox, &QCheckBox::stateChanged, this, &MainWindow::update_fileNamingPanel_destinationFolder);
    connect(this, &MainWindow::sourceFolderPathChanged, this, &MainWindow::updateDestinationFolderIfOverwrite);

    //File naming panel: configure the output file names, file naming strategy
    connect(ui->originalNameRadioBtn, &QAbstractButton::toggled, this, &MainWindow::updateFileNamingPanel);

    //Parameters panel: adjusting downscaling factor and quality
    connect(ui->scaleSlider, &QSlider::valueChanged, ui->scaleSpinBox, &QSpinBox::setValue);
    connect(ui->scaleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateScaleSlider);

    connect(ui->qualitySlider, &QSlider::valueChanged, ui->qualitySpinBox, &QSpinBox::setValue);
    connect(ui->qualitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::updateQualitySlider);

    initializeParameters(); // initialize scale and quality
    connect(ui->defaultSettingsCheckBox, &QCheckBox::stateChanged, this, &MainWindow::updateParametersPanel);

    //Buttons execute and exit
    connect(ui->exitButton, &QPushButton::clicked, qApp, &QApplication::exit);    
    connect(ui->executeButton, &QPushButton::clicked, this, &MainWindow::execute);
    connect(this, &MainWindow::executeCompleted, this, &MainWindow::showMessage);
    connect(m_msgBox, &QDialog::finished, this, &MainWindow::reinitializeWindow);

    setWindowTitle("DownScaler");

    QIcon openFolderIcon(":/img/img/open.png");
    QSize btnSize = ui->selectFilesButton->size();
    ui->selectFilesButton->setIcon(openFolderIcon);
    ui->selectFilesButton->setIconSize(0.57*btnSize);
    ui->selectDestinationButton->setIcon(openFolderIcon);
    ui->selectDestinationButton->setIconSize(0.57*btnSize);

    ui->originalNameRadioBtn->setChecked(true);

}

MainWindow::~MainWindow()
{
    //write path to settings
    m_settings.setValue("/Settings/openDir", m_sourceFolderPath);

    delete ui;
}

int MainWindow::calculateStrategy() //calculate file naming strategy and initialize concerned attributes
{
    if (ui->overWriteCheckBox->isChecked())
        return strategy::overwrite;

    if (ui->originalNameRadioBtn->isChecked())
    {
        m_prefix = ui->prefixLineEdit->text();
        m_suffix = ui->suffixLineEdit->text();
        return strategy::original;
    }
    else
    {
        m_customizedName = ui->nameLineEdit->text();
        if (m_filesCount == 1) return strategy::customizedUnique;
        if (m_filesCount > 1){
            m_offset = ui->incrOffsetSpinBox->value();
            return strategy::customizedIncremented;
        }
    }

    return strategy::undefined;
}

QString MainWindow::prepareFilePath(int strategy, QString originalName, int index, QString extension)
{
    switch (strategy){
        case strategy::overwrite :
            return QString(m_destinationFolderPath + "/" + originalName + "." + extension);

        case strategy::original:
            return QString(m_destinationFolderPath + "/" +
                           m_prefix +
                           originalName +
                           m_suffix +
                           "." + extension );

        case strategy::customizedUnique:
            return QString(m_destinationFolderPath + "/" + m_customizedName + "." + extension);

        case strategy::customizedIncremented:
            return QString(m_destinationFolderPath + "/" + m_customizedName +
                           QString::number(index + m_offset) + "." + extension);

    }
    return QString();
}

void MainWindow::updateFilesList()
{
    QStringList *selectedFiles = new QStringList(QFileDialog::getOpenFileNames(this, "Select one or more files to open",
                                                        m_sourceFolderPath, "Images (*.jpg *.jpeg *.png *.bmp *.tif *.tiff)"));

    if(!selectedFiles->isEmpty())
    {
        delete m_selectedFilesList;
        m_selectedFilesList = selectedFiles; // avoid copying the table
        m_filesCount = m_selectedFilesList->size();
        emit filesCountChanged();

        QString path = (*m_selectedFilesList)[0];
        m_sourceFolderPath = path.section('/', 0, path.count('/')-1);
        emit sourceFolderPathChanged();
        ui->openDirLineEdit->setText(m_sourceFolderPath+"/...");
        ui->openDirLineEdit->setCursorPosition(0);
    }
    else
    {
        delete selectedFiles;
        selectedFiles = nullptr;
    }
}

void MainWindow::updateDestinationFolder()
{
    QString selectedFolder = QFileDialog::getExistingDirectory(this, "Choose Directory", m_sourceFolderPath,
                                                                QFileDialog::ShowDirsOnly);
    if(!selectedFolder.isEmpty())
    {
        m_destinationFolderPath = selectedFolder;
        ui->saveDirLineEdit->setText(m_destinationFolderPath);
        ui->saveDirLineEdit->setCursorPosition(0);        
    }
}


void MainWindow::initializeParameters()
{
    ui->scaleSlider->setValue(DEFAULT_SCALE);
    ui->qualitySlider->setValue(DEFAULT_QUALITY);
}


void MainWindow::updateScaleSlider(int val)
{
    if (ui->scaleSlider->value() != val)
        ui->scaleSlider->setValue(val);
}

void MainWindow::updateQualitySlider(int val)
{
    if (ui->qualitySlider->value() != val)
        ui->qualitySlider->setValue(val);
}

void MainWindow::updateParametersPanel(int state)
{    
    //state = ui->defaultSettingsCheckBox->checkState();
    if (state)
        initializeParameters();

    constexpr int n = 6;
    QWidget *settingsWidgets[n] = {ui->scaleLabel,
                                   ui->scaleSlider,
                                   ui->scaleSpinBox,
                                   ui->qualityLabel,
                                   ui->qualitySlider,
                                   ui->qualitySpinBox};
    for (QWidget *el: settingsWidgets)
        el->setDisabled(static_cast<bool>(state));
}

void MainWindow::updateFileNamingPanel()
{
    //conserve or not the original file name
    bool original = ui->originalNameRadioBtn->isChecked();

    ui->nameLineEdit->setDisabled(original);

    ui->prefixLabel->setEnabled(original);
    ui->prefixLineEdit->setEnabled(original);
    ui->suffixLabel->setEnabled(original);
    ui->suffixLineEdit->setEnabled(original);

    //Clear some fields if necessary
    if (original)
    {
        if (!ui->nameLineEdit->text().isEmpty())
            ui->nameLineEdit->clear();
        if (ui->incrOffsetSpinBox->value())
            ui->incrOffsetSpinBox->setValue(0);
    }
    else
    {
        if (!ui->prefixLineEdit->text().isEmpty())
            ui->prefixLineEdit->clear();
        if (!ui->suffixLineEdit->text().isEmpty())
            ui->suffixLineEdit->clear();
    }

    updateAutoIncrementalPart();

}

void MainWindow::updateAutoIncrementalPart()
{
    bool original = ui->originalNameRadioBtn->isChecked(); //conserve or not the original file name
    bool originalOrUnique = (original or m_filesCount==1);

    ui->autoIncrLabel->setDisabled(originalOrUnique);
    ui->autoIncrLbl->setDisabled(originalOrUnique);
    ui->incrOffsetLabel->setDisabled(originalOrUnique);
    ui->incrOffsetSpinBox->setDisabled(originalOrUnique);

    QString depends = m_filesCount ? "Yes" : "-";
    QString result = originalOrUnique ? "No" : depends;
    ui->autoIncrLabel->setText(result);

}

void MainWindow::update_fileNamingPanel_destinationFolder(int state)
{
    //state := ui->overWriteCheckBox->checkState();

    //Partial update of the file naming panel
    if (state)
    {
        if (!ui->originalNameRadioBtn->isChecked())
            ui->originalNameRadioBtn->setChecked(true);
        if (!ui->prefixLineEdit->text().isEmpty())
            ui->prefixLineEdit->clear();
        if (!ui->suffixLineEdit->text().isEmpty())
            ui->suffixLineEdit->clear();
    }

    constexpr int k = 6;
    QWidget *updateWidgets[k]= {   ui->originalNameRadioBtn,
                                    ui->customNameRadioBtn,
                                    ui->prefixLabel,
                                    ui->prefixLineEdit,
                                    ui->suffixLabel,
                                    ui->suffixLineEdit};

    bool doOverwrite = static_cast<bool>(state);
    for (int i=0; i<k; ++i) updateWidgets[i]->setDisabled(doOverwrite);

    //Update of the destination folder widgets
    if (!ui->openDirLineEdit->text().isEmpty())
    {
        m_destinationFolderPath = m_sourceFolderPath;
        ui->saveDirLineEdit->setText(m_destinationFolderPath);
        ui->saveDirLineEdit->setCursorPosition(0);
    }
    ui->saveDirLineEdit->setDisabled(doOverwrite);
    ui->selectDestinationButton->setDisabled(doOverwrite);

}


void MainWindow::updateDestinationFolderIfOverwrite()
{
    bool doOverwrite = static_cast<bool>(ui->overWriteCheckBox->checkState());
    if (doOverwrite)
    {
        m_destinationFolderPath = m_sourceFolderPath;
        ui->saveDirLineEdit->setText(m_destinationFolderPath);
        ui->saveDirLineEdit->setCursorPosition(0);
    }
}

void MainWindow::execute()
{

    if(!m_selectedFilesList->isEmpty() and !m_destinationFolderPath.isEmpty())
    {
        m_progressDialog->setMaximum(m_filesCount);
        m_progressDialog->show();

        int scale = ui->scaleSlider->value();
        int quality = ui->qualitySlider->value();
        int fileNamingStrategy = calculateStrategy();        

        int validFilesCount = 0;
        int corruptedFilesCount = 0;
        int ignoredCount = 0; // 0 <= ignoredCount <= validFilesCount
        qint64 gain = 0; //weight(old files) - weight(new files) in bytes

        for (int i=0; i<m_filesCount; ++i)
        {
            QString filePath = (*m_selectedFilesList)[i];
            QString fileName = filePath.section('/', -1);
            QString progressMessage = QString("Processing image %1 from %2\n%3").arg(i+1).arg(m_filesCount).arg(fileName);
            m_progressDialog->setLabelText(progressMessage);
            m_progressDialog->setValue(i+1);

            QImageReader imReader(filePath);
            imReader.setAutoTransform(true); //prevent indesirable image rotation, auto-orient the image based on the EXIF data

            qDebug() << "\ni =" << i << " original file path:" << filePath;

            if (imReader.canRead()) //quick test of file format support and image data validity (corrupted files may slip through)
            {
                //prepare the size of the image
                QSize newSize = 0.01*scale*imReader.size();
                imReader.setScaledSize(newSize);

                // Second readability test
                QImage resultingImage = imReader.read();
                if(!resultingImage.isNull())
                {
                    validFilesCount++;                    
                    QFileInfo info(filePath);
                    QString extension =info.suffix();
                    qint64 initialWeight = info.size();
                    QString newFilePath = prepareFilePath(fileNamingStrategy, info.completeBaseName(), validFilesCount, extension);
                    qDebug() << "new file path: " << newFilePath;

                    if ( !QFile(newFilePath).exists() or fileNamingStrategy == strategy::overwrite)
                    {
                        resultingImage.save(newFilePath, extension.toStdString().c_str(), quality); //returns boolean
                        updateGain(newFilePath, initialWeight, gain);
                    }
                    else
                    {
                        FileExistsDialog dialog(this, filePath, newFilePath);
                        dialog.adjustSize();
                        int result = dialog.exec();
                        switch (result) {
                        case FileExistsDialog::clickedOverwrite :
                            resultingImage.save(newFilePath, extension.toStdString().c_str(), quality);
                            updateGain(newFilePath, initialWeight, gain);
                            break;
                        case FileExistsDialog::clickedRename :
                        {                            
                            while(QFile(newFilePath).exists())
                                newFilePath = getModifiedFileName(newFilePath);

                            resultingImage.save(newFilePath, extension.toStdString().c_str(), quality);
                            updateGain(newFilePath, initialWeight, gain);
                            break;
                        }
                        case FileExistsDialog::clickedSkip :
                        case QDialog::Rejected :
                            ignoredCount++;
                            break;

                        }
                    }

                }
                else
                {
                    m_corruptedFilesList << fileName;
                    corruptedFilesCount++;                    
                    qDebug() << "resulting image is Null: " << imReader.errorString();
                }
            }
            else
            {
                m_corruptedFilesList << fileName;
                corruptedFilesCount++;                
                qDebug() << "canRead = false: " << imReader.errorString();
            }

        }
        qDebug() << "executeCompleted \nvalidFilesCount:" << validFilesCount <<
                   ", corruptedFilesCount: " << corruptedFilesCount <<
                    ", ignoredCount: " << ignoredCount <<
                    ", gain: " << gain*1./(1048576) << " MB\n";

        emit executeCompleted(validFilesCount, corruptedFilesCount, ignoredCount, gain);

    }
}

void MainWindow::showMessage(int valid, int corrupted, int ignored, qint64 gain)
{
    if (ignored == m_filesCount)
        reinitializeWindow();
    else
    {
        double gainInMiB = gain*1./1048576; //weight(old files) - weight(new files) in mebibytes
        QString gainInMiB_str = QString::number(gainInMiB, 'g', 3);

        QString msgText;
        if (!corrupted)
        {
            msgText = QString("Rescaling completed!\n\n"
                      "(The files size has been reduced by %1 MB).").arg(gainInMiB_str);
            QPixmap msgIcon(":/img/img/checked.jpg");
            msgIcon = msgIcon.scaledToWidth(60, Qt::SmoothTransformation);
            m_msgBox->setIconPixmap(msgIcon);
            m_msgBox->setWindowTitle("Success");
        }
        else
        {
            if (valid){
                msgText = QString("%1 image(s) from %2 have been successfully rescaled and saved.\n"
                                  "(The files size has been reduced by %3 MB).\n\n"
                                  "Failed to read %4 file(s):\n\n")
                                            .arg(valid - ignored) //valid-ignored, 0 <= ignored <= valid
                                            .arg(m_filesCount - ignored)
                                            .arg(gainInMiB_str)
                                            .arg(corrupted)
                            + m_corruptedFilesList.join("\n");

                m_msgBox->setIcon(QMessageBox::Information);
                m_msgBox->setWindowTitle("Partial success");
            }
            else {
                msgText = "Failed to read the file(s): invalid image data.";
                m_msgBox->setIcon(QMessageBox::Warning);
                m_msgBox->setWindowTitle("Invalid files");
            }
        }

        m_msgBox->setText(msgText);
        m_msgBox->exec();
    }
    /* Closing the m_msgBox triggers the reinitialization of
       some attributes and widgets of the MainWindow*/

}

void MainWindow::reinitializeWindow()
{
    *m_selectedFilesList = QStringList();
    m_corruptedFilesList = QStringList();
    QString *strAttributes[4] = {  &m_destinationFolderPath,
                                   &m_customizedName,
                                   &m_prefix,
                                   &m_suffix };
    for (QString *el : strAttributes) *el = QString();
    m_filesCount = 0;
    m_offset = 0;

    QLineEdit* lineEdits [5]= { ui->openDirLineEdit,
                                ui->saveDirLineEdit,
                                ui->nameLineEdit,
                                ui->prefixLineEdit,
                                ui->suffixLineEdit };
    for (QLineEdit * el : lineEdits) el->clear();

    ui->incrOffsetSpinBox->setValue(0);

    if(ui->overWriteCheckBox->checkState())
        ui->overWriteCheckBox->setChecked(false);

}

QString MainWindow::getSuffixedFileName(const QString & name, const QString & suffix)
{
    //path = a/b.c

    QFileInfo fileInfo(name);
    QString a = name.section('/', 0, name.count('/')-1);
    QString b = fileInfo.completeBaseName();
    QString c = fileInfo.suffix();

    b.append(suffix);
    QString result = a + '/' + b + '.' + c;    
    return result;
}

void MainWindow::updateGain(const QString & destinationPath, const qint64 & initialWeight, qint64 & out_gain)
{
    QFileInfo info(destinationPath);
    out_gain += initialWeight - info.size();
}

QString MainWindow::getModifiedFileName(QString path)
{
    /* path = a/b.c

    idea: transform b into b' = b + " (i)" or into b' = b + "-bis"
    ex: b = xyz -> b' = xyz (2)
    b = xyz (2) or b = xyz(2) -> b' = xyz (3) and so on
    b = xyz(uv -> b' = xyz(uv-bis
    */

    QFileInfo fileInfo(path);
    QString a = path.section('/', 0, path.count('/')-1);
    QString b = fileInfo.completeBaseName();
    QString c = fileInfo.suffix();

    //Prepare suffix
    QString s;
    int cutCount=0;

    QString currentIndexStr = b.contains('(')? b.section('(', -1, -1) : QString(); // '(' is excluded
    currentIndexStr.chop(1); // ')' is сut off

    if (!currentIndexStr.isEmpty())
    {
        bool ok;
        int currentIndex = currentIndexStr.toInt(&ok);
        if (ok)
        {
            s = QString(" (%1)").arg(currentIndex+1);
            cutCount = currentIndexStr.size()+2; // size of '(' + currentIndexStr + ')'
            if( b.at(b.size() - cutCount -1) == ' ' ) ++cutCount; // add 1 if there exists space before '(' e.g.: b = xxyyzz (4)
        }
        else
            s = "-bis";
    }
    else
        s = " (2)";

    b.chop(cutCount);
    b.append(s);
    QString result = a + '/' + b + '.' + c;

    return result;

}
