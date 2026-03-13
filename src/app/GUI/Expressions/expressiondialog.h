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

#ifndef EXPRESSIONDIALOG_H
#define EXPRESSIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <functional>

#include "conncontext.h"
#include "dialogs/dialog.h"
#include "Private/esettings.h"
#include "smartPointers/selfref.h"

class QrealAnimator;
class QStringAnimator;
class ExpressionEditor;
class PropertyBindingBase;
class Property;
class QJSValue;
class Expression;

class JSLexer;
class JSEditor;

class QsciAPIs;

class ExpressionDialog : public Friction::Ui::Dialog
{
public:
    ExpressionDialog(QrealAnimator* const target,
                     QWidget * const parent = nullptr);
    ExpressionDialog(QStringAnimator* const target,
                     QWidget * const parent = nullptr);

private:
    using ResultTester = std::function<void(const QJSValue&)>;
    using ExpressionSetter = std::function<void(const qsptr<Expression>&)>;
    struct TargetOps {
        Property* context = nullptr;
        QString name;
        ResultTester tester;
        std::function<QString()> getBindings;
        std::function<QString()> getDefinitions;
        std::function<QString()> getScript;
        ExpressionSetter setExpression;
        ExpressionSetter setExpressionAction;
    };

    ExpressionDialog(const TargetOps& ops, QWidget* const parent);
    static TargetOps makeOps(QrealAnimator* const target);
    static TargetOps makeOps(QStringAnimator* const target);

    using PropertyBindingMap = std::map<QString, QSharedPointer<PropertyBindingBase>>;
    bool getBindings(PropertyBindingMap& bindings);
    void updateScriptBindings();
    void updateScriptDefinitions();
    void updateAllScript();
    void setCurrentTabId(const int id);
    bool apply(const bool action);

    QWidget* setupPresetsUi();
    void populatePresets(const bool &clear = false);
    void exportPreset();
    void importPreset(const QString &path);
    void savePreset();
    void applyPreset(const QString &id);
    const QString genPresetId(const QString &title);
    const QString filterPresetId(const QString &id);
    void fixLeaveEvent(QWidget *widget);
    bool editDialog(const QString &title,
                    Friction::Core::ExpressionPresets::Expr *expr,
                    const bool &showId = true);

    Property* const mContext;
    const QString mTargetName;
    const ResultTester mResultTester;
    const std::function<QString()> mGetBindings;
    const std::function<QString()> mGetDefinitions;
    const std::function<QString()> mGetScript;
    const ExpressionSetter mSetExpression;
    const ExpressionSetter mSetExpressionAction;

    QTabWidget *mTab;
    int mTabEditor;

    QIcon mRedDotIcon;

    QPushButton* mBindingsButton;
    QPushButton* mDefinitionsButon;

    QLabel* mBindingsLabel;
    ExpressionEditor* mBindings;
    QLabel* mBindingsError;

    QLabel* mDefsLabel;
    JSLexer* mDefsLexer;
    JSEditor* mDefinitions;
    QsciAPIs* mDefinitionsApi;
    QLabel* mDefinitionsError;

    JSLexer* mScriptLexer;
    bool mBindingsChanged = true;
    bool mDefinitionsChanged = true;
    QLabel* mScriptLabel;
    JSEditor* mScript;
    QsciAPIs* mScriptApi;
    QLabel* mScriptError;

    ConnContext mAutoApplyConn;

    QComboBox* mPresetsCombo;

    eSettings* mSettings;
};

#endif // EXPRESSIONDIALOG_H
