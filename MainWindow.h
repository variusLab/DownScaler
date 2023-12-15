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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QSettings>

class ProgressDialog;
class QProgressBar;
class QProgressDialog;
class QMessageBox;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

constexpr int DEFAULT_SCALE = 50;
constexpr int DEFAULT_QUALITY = 75;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    enum strategy {undefined, overwrite, original, customizedUnique, customizedIncremented}; //file naming strategies

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    int calculateStrategy(); //calculate file naming strategy and initialize corresponding attributes
    QString prepareFilePath(int strategy, QString name, int index, QString extension);    
    QString getSuffixedFileName(const QString & name, const QString & suffix);
    QString getModifiedFileName(QString path);
    void updateGain(const QString & destinationPath, const qint64 & initialWeight, qint64 & out_gain);

public slots:
    void updateFilesList(); //update list of selected files
    void updateDestinationFolder();    
    void updateScaleSlider(int val);
    void updateQualitySlider(int val);
    void initializeParameters(); //initialize quality and scale (elements of parameters panel)
    void updateParametersPanel(int state); // set default quality and scale if state not null
    void updateFileNamingPanel();
    void updateAutoIncrementalPart();
    void update_fileNamingPanel_destinationFolder(int state); //called when 'Overwrite existing files' checkBox is toggled
    void updateDestinationFolderIfOverwrite();
    void execute(); //rescale and save
    void showMessage(int valid, int corrupted, int ignored, qint64 gain); //message at the end of the operation 'execute()'
    void reinitializeWindow(); //reinitialize some fields and attributes

signals:
    void filesCountChanged();
    void sourceFolderPathChanged();
    void executeCompleted(int valid, int corrupted, int ignored, qint64 bytesGained);
    void stepChanged(int newStep, QString newMessage); //progress step changed


private:
    Ui::MainWindow *ui;

    QSettings m_settings;
    QString m_sourceFolderPath;

    QStringList* m_selectedFilesList;
    int m_filesCount; //total number of files selected
    QString m_destinationFolderPath;
    QStringList m_corruptedFilesList;

    QString m_customizedName;
    QString m_prefix;
    QString m_suffix;
    int m_offset;

    QProgressDialog *m_progressDialog;
    QMessageBox *m_msgBox;

};
#endif // MAINWINDOW_H
