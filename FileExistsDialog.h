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

#ifndef FILEEXISTSDIALOG_H
#define FILEEXISTSDIALOG_H

#include <QDialog>

namespace Ui {
class FileExistsDialog;
}

class FileExistsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileExistsDialog(QWidget *parent = nullptr, QString source = "path1/sourceFolder/sourceImg.png",
                              QString destination = "path2/destinationFolder/existingImg.png");
    ~FileExistsDialog();
    enum {clickedRename = 2, clickedOverwrite, clickedSkip}; //QDialog::Rejected = 0    QDialog::Accepted = 1

    QPixmap preparePixmap(const QString path, const int length);
    /* Creates an QPixmap, adjusts its scale so that it
     * fits into a square window of size 'length' and rotates it if necessary */

private:
    Ui::FileExistsDialog *ui;

    qreal getMin(const qreal a, const qreal b)
    {
        qreal result;
        result = a <= b ? a : b;
        return result;
    }
};

#endif // FILEEXISTSDIALOG_H
