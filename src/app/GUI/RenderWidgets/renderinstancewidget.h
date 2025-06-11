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
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#ifndef RENDERINSTANCEWIDGET_H
#define RENDERINSTANCEWIDGET_H

#include "closablecontainer.h"
#include "renderinstancesettings.h"
#include "GUI/BoxesList/boxsinglewidget.h"
#include <QPushButton>
#include <QMouseEvent>
#include <QAction>
#include <QMenu>
#include <QLineEdit>

class OutputSettingsDisplayWidget;
class RenderSettingsDisplayWidget;

class RenderInstanceWidget;
class OutputProfilesListButton : public QPushButton {
    Q_OBJECT
public:
    OutputProfilesListButton(RenderInstanceWidget *parent);
signals:
    void profileSelected(OutputSettingsProfile *);
protected:
    void mousePressEvent(QMouseEvent *e);
    RenderInstanceWidget *mParentWidget;
};

class RenderInstanceWidget : public ClosableContainer {
    Q_OBJECT
public:
    RenderInstanceWidget(Scene *scene, QWidget *parent = nullptr);
    RenderInstanceWidget(const RenderInstanceSettings& scene,
                         QWidget *parent);
    RenderInstanceSettings &getSettings();

    void write(eWriteStream &dst) const;
    void read(eReadStream &src);
    void updateRenderSettings();

protected:
    void mousePressEvent(QMouseEvent* e);
private:
    void iniGUI();

    ClosableContainer *mOutputSettings;
    RenderSettingsDisplayWidget *mRenderSettingsDisplayWidget;
    OutputSettingsDisplayWidget *mOutputSettingsDisplayWidget;

    QPushButton *mOutputDestinationButton;
    QLineEdit *mOutputDestinationLineEdit;

    QPushButton* mRenderSettingsButton;

    OutputProfilesListButton *mOutputSettingsProfilesButton;
    QPushButton *mOutputSettingsButton;
    QLineEdit *mNameLabel;
    QVBoxLayout *mContentLayout = new QVBoxLayout();
    RenderInstanceSettings mSettings;
    QPushButton *mPlayButton;

signals:
    void duplicate(RenderInstanceSettings&);
protected:
    void updateOutputDestinationFromCurrentFormat();
private:
    void updateFromSettings();

    void outputSettingsProfileSelected(OutputSettingsProfile *profile);

    void openOutputSettingsDialog();
    void openOutputDestinationDialog();
    void openRenderSettingsDialog();

    const QPair<QString,QStringList> getExportImageExtensions(const OutputSettings &settings);
};

#endif // RENDERINSTANCEWIDGET_H
