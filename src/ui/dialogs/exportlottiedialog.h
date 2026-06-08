/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
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

#ifndef EXPORTLOTTIEDIALOG_H
#define EXPORTLOTTIEDIALOG_H

#include "dialogs/dialog.h"

class Canvas;
class QCheckBox;
class QComboBox;
class QPushButton;
class QSpinBox;
class QTemporaryFile;
class SceneChooser;

class UI_EXPORT ExportLottieDialog : public Friction::Ui::Dialog
{
public:
    ExportLottieDialog(QWidget* const parent = nullptr,
                       const QString& warnings = QString());
    void showPreview(const bool& closeWhenDone = false);

private:
    bool exportTo(const QString& file);
    bool writePreviewHtml(const QString& jsonFile,
                          const QString& htmlFile);
    void finishedDialog(const QString& fileName);

    QSharedPointer<QTemporaryFile> mPreviewJsonFile;
    QSharedPointer<QTemporaryFile> mPreviewHtmlFile;
    QPushButton* mPreviewButton;

    SceneChooser* mScene;
    QSpinBox* mFirstFrame;
    QSpinBox* mLastFrame;
    QComboBox* mFormat;
    QCheckBox* mBackground;
    QCheckBox* mEmbedImages;
    QComboBox* mPreviewBackground;
    QCheckBox* mNativeText;
    QCheckBox* mNotify;
};

#endif // EXPORTLOTTIEDIALOG_H
