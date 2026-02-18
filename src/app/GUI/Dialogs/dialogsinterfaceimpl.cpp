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

#include "dialogsinterfaceimpl.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QStatusBar>

#include "widgets/buttonslist.h"
#include "GUI/global.h"
#include "GUI/mainwindow.h"
#include "GUI/Expressions/expressiondialog.h"
#include "dialogs/durationrectsettingsdialog.h"
//#include "GUI/Dialogs/animationtopaintdialog.h"
#include "dialogs/applyexpressiondialog.h"
#include "dialogs/scenesettingsdialog.h"

DialogsInterfaceImpl DialogsInterfaceImpl::sInstance;

class ShaderChoiceDialog : public QDialog {
public:
    ShaderChoiceDialog(const QString& name, const ShaderOptions& options,
                       QWidget* const parent) : QDialog(parent) {
        const auto layout = new QVBoxLayout(this);

        const QString labelTxt = "Missing Shader Effect '" + name + "'.\n"
                                 "Select compatible replacement.";

        layout->addWidget(new QLabel(labelTxt), 0);

        eSizesUI::widget.addSpacing(layout);

        const auto homePath = QDir::homePath();

        const auto textTriggerGetter = [this, &options, &homePath](const int id) {
            const auto& shader = options.at(id);
            QString ttPath = shader->fGrePath;
            if(ttPath.left(homePath.count()) == homePath) {
                ttPath = "~" + ttPath.mid(homePath.count());
            }
            return ButtonsList::TextTrigger{
                ttPath, [this, ttPath, id, &options]() {
                mSelected = options.at(id);
                accept();
            }};
        };
        const int count = options.count();
        const auto recentWidget = new ButtonsList(textTriggerGetter, count, this);
        layout->addWidget(recentWidget);

        setLayout(layout);
    }

    stdsptr<ShaderEffectCreator> getSelected() const { return mSelected; }
private:
    stdsptr<ShaderEffectCreator> mSelected;
};

stdsptr<ShaderEffectCreator> DialogsInterfaceImpl::execShaderChooser(
        const QString& name, const ShaderOptions& options) const {
    const auto parent = MainWindow::sGetInstance();
    ShaderChoiceDialog dialog(name, options, parent);
    const bool accepted = dialog.exec() == QDialog::Accepted;
    if(accepted) return dialog.getSelected();
    return nullptr;
}

void DialogsInterfaceImpl::showExpressionDialog(QrealAnimator* const target) const {
    const auto parent = MainWindow::sGetInstance();
    const auto dialog = new ExpressionDialog(target, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void DialogsInterfaceImpl::showExpressionDialog(QStringAnimator* const target) const {
    const auto parent = MainWindow::sGetInstance();
    const auto dialog = new ExpressionDialog(target, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void DialogsInterfaceImpl::showApplyExpressionDialog(QrealAnimator* const target) const {
    const auto parent = MainWindow::sGetInstance();
    const auto dialog = new ApplyExpressionDialog(target, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void DialogsInterfaceImpl::showDurationSettingsDialog(
        DurationRectangle* const target) const {
    const auto parent = MainWindow::sGetInstance();
    const auto dialog = new DurationRectSettingsDialog(target, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

/*bool DialogsInterfaceImpl::execAnimationToPaint(
        const AnimationBox* const src,
        int& firstAbsFrame, int& lastAbsFrame,
        int& increment) const {
    const auto parent = MainWindow::sGetInstance();
    return AnimationToPaintDialog::sExec(src, firstAbsFrame, lastAbsFrame,
                                         increment, parent);
}*/

void DialogsInterfaceImpl::showSceneSettingsDialog(Canvas* const scene) const {
    const auto parent = MainWindow::sGetInstance();
    const auto dialog = new SceneSettingsDialog(scene, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(dialog, &QDialog::accepted, scene, [scene, dialog]() {
        dialog->applySettingsToCanvas(scene);
        dialog->close();
    });
    dialog->show();
}

void DialogsInterfaceImpl::displayMessageToUser(
        const QString& message, const int ms) const {
    const auto parent = MainWindow::sGetInstance();
    const auto widget = new QLabel(message, parent, Qt::SplashScreen);
    widget->setObjectName("messageLabel");
    widget->show();
    QTimer::singleShot(ms, widget, &QWidget::deleteLater);
}

void DialogsInterfaceImpl::showStatusMessage(
        const QString& message, const int ms) const {
    const auto win = MainWindow::sGetInstance();
    win->statusBar()->showMessage(message, ms);
}
