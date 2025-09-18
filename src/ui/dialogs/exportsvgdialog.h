/*
# enve2d - https://github.com/enve2d
#
# Copyright (c) enve2d developers
# Copyright (C) 2016-2020 Maurycy Liebner
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
*/

#ifndef EXPORTSVGDIALOG_H
#define EXPORTSVGDIALOG_H

#include "ui_global.h"

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QTemporaryFile>
#include <QComboBox>

#include "dialog.h"

class SceneChooser;
class ComplexTask;

class UI_EXPORT ExportSvgDialog : public Friction::Ui::Dialog
{
public:
    ExportSvgDialog(QWidget* const parent = nullptr,
                    const QString &warnings = QString());
    void showPreview(const bool &closeWhenDone = false);

private:
    ComplexTask* exportTo(const QString& file,
                          bool preview = false);

    void finishedDialog(const QString &fileName);

    QSharedPointer<QTemporaryFile> mPreviewFile;
    QPushButton *mPreviewButton;

    SceneChooser *mScene;

    QSpinBox *mFirstFrame;
    QSpinBox *mLastFrame;

    QCheckBox *mBackground;
    QCheckBox *mFixedSize;
    QCheckBox *mLoop;
    QCheckBox *mOptimize;
    QCheckBox *mNotify;
    QCheckBox *mBlendMix;
    QCheckBox *mColors11;

    QComboBox *mImageFormat;
    QSpinBox *mImageQuality;
};

#endif // EXPORTSVGDIALOG_H
