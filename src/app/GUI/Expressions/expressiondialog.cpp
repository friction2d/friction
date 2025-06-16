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

#include "expressiondialog.h"

#include <QLabel>
#include <QCheckBox>
#include <QScrollBar>
#include <QStatusBar>
#include <QApplication>
#include <QButtonGroup>
#include <QMessageBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QUuid>
#include <QRegularExpression>

#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciapis.h>

#include "Expressions/expression.h"
#include "Boxes/boundingbox.h"
#include "Private/document.h"
#include "expressioneditor.h"
#include "appsupport.h"

using namespace Friction::Core;

class JSLexer : public QsciLexerJavaScript
{
public:
    using QsciLexerJavaScript::QsciLexerJavaScript;
    JSLexer(QsciScintilla* const editor)
        : QsciLexerJavaScript(editor)
    {
        setDefaultPaper(QColor(33, 33, 38)/*"#2E2F30"*/);
        setFont(QApplication::font());
        setColor("#D6CF9A");

        setColor("#666666", Comment);
        setColor("#666666", CommentLine);
        setColor("#666666", CommentDoc);
        setColor("#666666", CommentLineDoc);

        setColor("#bf803C", Number);
        setColor("#D69545", DoubleQuotedString);
        setColor("#D69545", SingleQuotedString);

        setColor("#45C6D6", Keyword); // javascript
        setColor("#88ffbb", KeywordSet2); // definitions
        setColor("#FF8080", GlobalClass); // bindings

        setAutoIndentStyle(QsciScintilla::AiMaintain);
    }

    const char *keywords(int set) const {
        if (set == 1) {
            return sKeywordClass1;
        } else if (set == 2) {
            return mDefKeywordsClass2.data();
        } else if (set == 4) {
            return mBindKeywords5.data();
        }
        return 0;
    }

    QStringList autoCompletionWordSeparators() const
    {
        QStringList wl;

        wl << "::" << "->";

        return wl;
    }

    void addDefinition(const QString& def)
    {
        mDefinitions << def;
    }

    void removeDefinition(const QString& def)
    {
        mDefinitions.removeOne(def);
    }

    void clearDefinitions()
    {
        mDefinitions.clear();
    }

    void prepareDefinitions()
    {
        mDefKeywordsClass2 = mDefinitions.join(' ').toUtf8();
    }

    void addBinding(const QString& bind)
    {
        mBindings << bind;
    }

    void removeBinding(const QString& bind)
    {
        mBindings.removeOne(bind);
    }

    void clearBindings()
    {
        mBindings.clear();
    }

    void prepareBindings()
    {
        mBindKeywords5 = mBindings.join(' ').toUtf8();
    }

private:
    QStringList mDefinitions;
    QStringList mBindings;

    QByteArray mDefKeywordsClass2;
    QByteArray mBindKeywords5;

    static const char *sKeywordClass1;
};

const char *JSLexer::sKeywordClass1 =
            "abstract boolean break byte case catch char class const continue "
            "debugger default delete do double else enum export extends final "
            "finally float for function goto if implements import in instanceof "
            "int interface long native new package private protected public "
            "return short static super switch synchronized this throw throws "
            "transient try typeof var void volatile while with "
            "true false "
            "Math";

#define KEYWORDSET_MAX  8

class JSEditor : public QsciScintilla
{
public:
    JSEditor(const QString& fillerText)
        : mFillerTextV(fillerText)
    {
        setMinimumWidth(20*eSizesUI::widget);

        setFont(QApplication::font());
        setMargins(2);
        setMarginType(0, NumberMargin);
        setMarginWidth(0, "9999");
        setMarginWidth(1, "9");
        setMarginsFont(font());
        setMarginsForegroundColor("#999999");
        setMarginsBackgroundColor(QColor(40, 40, 47)/*"#444444"*/);

        setTabWidth(4);
        setBraceMatching(SloppyBraceMatch);
        setMatchedBraceBackgroundColor(QColor(33, 33, 38)/*"#555555"*/);
        setUnmatchedBraceBackgroundColor(QColor(33, 33, 38)/*"#555555"*/);
        setMatchedBraceForegroundColor("#D6CF9A");
        setUnmatchedBraceForegroundColor(QColor(255, 115, 115));
        setCaretForegroundColor(Qt::white);
        setCaretWidth(2);

        setAutoCompletionThreshold(1);
        setAutoCompletionCaseSensitivity(false);

        setScrollWidth(1);
        setScrollWidthTracking(true);

        connect(this, &JSEditor::SCN_FOCUSIN,
                this, &JSEditor::clearFillerText);
        connect(this, &JSEditor::SCN_FOCUSOUT,
                this, [this]() {
            if (length() == 0) { setFillerText(); }
            else { mFillerText = false; }
        });
    }

    void updateLexer()
    {
        for (int k = 0; k <= KEYWORDSET_MAX; ++k) {
            const char *kw = lexer() -> keywords(k + 1);

            if (!kw) { kw = ""; }

            SendScintilla(SCI_SETKEYWORDS, k, kw);
        }
        recolor();
    }

    void setText(const QString &text) override
    {
        if (text.isEmpty()) {
            setFillerText();
        } else {
            QsciScintilla::setText(text);
        }
    }

    QString text() const
    {
        if (mFillerText) { return QString(); }
        return QsciScintilla::text();
    }

    void setFillerText()
    {
        if (mFillerText) { return; }
        mFillerText = true;
        setText(mFillerTextV);
    }

    void clearFillerText()
    {
        if (mFillerText) {
            mFillerText = false;
            QsciScintilla::setText("");
        }
    }

private:
    bool mFillerText = false;
    const QString mFillerTextV;
    using QsciScintilla::setText;
    using QsciScintilla::text;
};

void addBasicDefs(QsciAPIs* const target)
{
    target->add("function");
    target->add("var");
    target->add("return");
    target->add("true");
    target->add("false");
    target->add("new");
    target->add("this");
    target->add("delete");
    target->add("const");
    target->add("break");
    target->add("while");
    target->add("for");

    target->add("Math.E");
    target->add("Math.LN2");
    target->add("Math.LN10");
    target->add("Math.LOG2E");
    target->add("Math.LOG10E");
    target->add("Math.PI");
    target->add("Math.SQRT1_2");
    target->add("Math.SQRT2");

    target->add("Math.abs(x)");
    target->add("Math.acos(x)");
    target->add("Math.acosh(x)");
    target->add("Math.asin(x)");
    target->add("Math.asinh(x)");
    target->add("Math.atan(x)");
    target->add("Math.atanh(x)");
    target->add("Math.atan2(y, x)");
    target->add("Math.cbrt(x)");
    target->add("Math.ceil(x)");
    target->add("Math.clz32(x)");
    target->add("Math.cos(x)");
    target->add("Math.cosh(x)");
    target->add("Math.exp(x)");
    target->add("Math.expm1(x)");
    target->add("Math.floor(x)");
    target->add("Math.fround(x)");
    target->add("Math.hypot(x, y, ...)");
    target->add("Math.imul(x, y)");
    target->add("Math.log(x)");
    target->add("Math.log1p(x)");
    target->add("Math.log10(x)");
    target->add("Math.log2(x)");
    target->add("Math.max(x, y, ...)");
    target->add("Math.min(x, y, ...)");
    target->add("Math.pow(x, y)");
    target->add("Math.random()");
    target->add("Math.round(x)");
    target->add("Math.sign(x)");
    target->add("Math.sin(x)");
    target->add("Math.sinh(x)");
    target->add("Math.sqrt(x)");
    target->add("Math.tan(x)");
    target->add("Math.tanh(x)");
    target->add("Math.trunc(x)");

    // add expressions presets
    const auto expressions = eSettings::sInstance->fExpressions.getAll();
    for (const auto &expr : expressions) {
        if (!expr.enabled) { continue; }
        for (const auto &highlight : expr.highlighters) {
            target->add(highlight);
        }
    }
}

ExpressionDialog::ExpressionDialog(QrealAnimator* const target,
                                   QWidget * const parent)
    : Friction::Ui::Dialog(parent)
    , mTarget(target)
    , mTab(nullptr)
    , mTabEditor(0)
    , mPresetsCombo(nullptr)
    , mSettings(eSettings::sInstance)
{
    setWindowTitle(tr("Expression %1").arg(target->prp_getName()));

    const auto windowLayout = new QVBoxLayout(this);
    setLayout(windowLayout);

    mTab = new QTabWidget(this);
    mTab->setTabBarAutoHide(true);
    windowLayout->addWidget(mTab);

    const auto editorWidget = new QWidget(this);
    mTabEditor = mTab->addTab(editorWidget, tr("Editor"));
    const auto mainLayout = new QVBoxLayout(editorWidget);

    mainLayout->addWidget(setupPresetsUi());

    const auto tabLayout = new QHBoxLayout;
    tabLayout->setContentsMargins(0, 0, 0, 0);
    mBindingsButton = new QPushButton(tr("Bindings and Script"), this);
    mBindingsButton->setFocusPolicy(Qt::NoFocus);
    mBindingsButton->setObjectName("leftButton");
    mBindingsButton->setCheckable(true);
    mBindingsButton->setChecked(true);

    mDefinitionsButon = new QPushButton(tr("Definitions"), this);
    mDefinitionsButon->setFocusPolicy(Qt::NoFocus);
    mDefinitionsButon->setObjectName("rightButton");
    mDefinitionsButon->setCheckable(true);

    const auto tabGroup = new QButtonGroup(this);
    tabGroup->addButton(mBindingsButton, 0);
    tabGroup->addButton(mDefinitionsButon, 1);
    tabGroup->setExclusive(true);
    connect(tabGroup, qOverload<int, bool>(&QButtonGroup::idToggled),
            this, [this](const int id, const bool checked) {
        if (checked) { setCurrentTabId(id); }
    });

    tabLayout->addWidget(mBindingsButton);
    tabLayout->addWidget(mDefinitionsButon);
    mainLayout->addLayout(tabLayout);

    mBindings = new ExpressionEditor(target, this);
    connect(mBindings, &ExpressionEditor::textChanged,
            this, [this]() {
        mBindingsChanged = true;
        updateAllScript();
    });

    mBindingsLabel = new QLabel(tr("Bindings:"));
    mainLayout->addWidget(mBindingsLabel);
    mainLayout->addWidget(mBindings, 1);

    mBindingsError = new QLabel(this);
    mBindingsError->setObjectName("errorLabel");
    mainLayout->addWidget(mBindingsError);

    mDefsLabel = new QLabel(tr("Definitions:"));
    mainLayout->addWidget(mDefsLabel);
    mDefinitions = new JSEditor(tr("// Here you can define JavaScript functions,\n"
                                   "// you can later use in the 'Calculate'\n"
                                   "// portion of the script."));
    mDefsLexer = new JSLexer(mDefinitions);

    mDefinitionsApi = new QsciAPIs(mDefsLexer);
    addBasicDefs(mDefinitionsApi);
    mDefinitionsApi->prepare();

    mDefinitions->setLexer(mDefsLexer);
    mDefinitions->setAutoCompletionSource(QsciScintilla::AcsAll);
    mDefinitions->setText(target->getExpressionDefinitionsString());
    connect(mDefinitions, &QsciScintilla::textChanged, this, [this]() {
        mDefinitions->autoCompleteFromAll();
        mDefinitionsChanged = true;
        updateAllScript();
    });

    mainLayout->addWidget(mDefinitions);

    mDefinitionsError = new QLabel(this);
    mDefinitionsError->setObjectName("errorLabel");
    mainLayout->addWidget(mDefinitionsError);

    mScriptLabel = new QLabel(QString("%1 (  ) :").arg(tr("Calculate")));
    mainLayout->addWidget(mScriptLabel);
    mScript = new JSEditor(tr("// Here you can define a JavaScript script,\n"
                              "// that will be evaluated every time any of\n"
                              "// the bound property values changes.\n"
                              "// You should return the resulting value\n"
                              "// at the end of this script."));
    mScriptLexer = new JSLexer(mScript);

    mScriptApi = new QsciAPIs(mScriptLexer);
    addBasicDefs(mScriptApi);
    mScriptApi->prepare();

    mScript->setLexer(mScriptLexer);
    mScript->setAutoCompletionSource(QsciScintilla::AcsAll);
    mScript->setText(target->getExpressionScriptString());
    connect(mScript, &QsciScintilla::textChanged,
            mScript, &QsciScintilla::autoCompleteFromAll);
    mainLayout->addWidget(mScript, 2);

    mScriptError = new QLabel(this);
    mScriptError->setObjectName("errorLabel");
    mainLayout->addWidget(mScriptError);

    const auto buttonsLayout = new QHBoxLayout;
    const auto applyButton = new QPushButton(tr("Apply"), this);
    const auto okButton = new QPushButton(tr("Ok"), this);
    const auto cancelButton = new QPushButton(tr("Cancel"), this);
    const auto checkBox = new QCheckBox(tr("Auto Apply"), this);
    connect(checkBox, &QCheckBox::stateChanged,
            this, [this](const int state) {
        if (state) {
            mAutoApplyConn << connect(mBindings, &ExpressionEditor::textChanged,
                                      this, [this]() { apply(false); });
            mAutoApplyConn << connect(mDefinitions, &QsciScintilla::textChanged,
                                      this, [this]() { apply(false); });
            mAutoApplyConn << connect(mScript, &QsciScintilla::textChanged,
                                      this, [this]() { apply(false); });
        } else { mAutoApplyConn.clear(); }
    });

    buttonsLayout->addWidget(checkBox);
    buttonsLayout->addWidget(applyButton);
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonsLayout);

    connect(applyButton, &QPushButton::released,
            this, [this]() { apply(true); });
    connect(okButton, &QPushButton::released,
            this, [this]() {
        const bool valid = apply(true);
        if(valid) accept();
    });
    connect(cancelButton, &QPushButton::released,
            this, &ExpressionDialog::reject);

    connect(mScript, &QsciScintilla::SCN_FOCUSIN,
            this, [this]() {
        if(mBindingsChanged || mDefinitionsChanged) {
            updateAllScript();
        }
    });
    setCurrentTabId(0);
    updateAllScript();

    const int pixSize = eSizesUI::widget/2;
    eSizesUI::widget.add(mBindingsButton,
                         [this](const int size) {
        Q_UNUSED(size)
        mBindingsButton->setFixedHeight(eSizesUI::button);
        mDefinitionsButon->setFixedHeight(eSizesUI::button);
    });

    QPixmap pix(pixSize, pixSize);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setBrush(Qt::red);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawEllipse(pix.rect().adjusted(1, 1, -1, -1));
    p.end();
    mRedDotIcon = QIcon(pix);

    mBindingsButton->setFocus();
}

void ExpressionDialog::setCurrentTabId(const int id)
{
    const bool first = id == 0;
    if (!first) { mBindingsButton->setChecked(false); }
    mBindingsLabel->setVisible(first);
    mBindings->setVisible(first);
    mBindingsError->setVisible(first);

    mScriptLabel->setVisible(first);
    mScript->setVisible(first);
    mScriptError->setVisible(first);

    if (first) { mDefinitionsButon->setChecked(false); }
    mDefsLabel->setVisible(!first);
    mDefinitions->setVisible(!first);
    mDefinitionsError->setVisible(!first);
}

void ExpressionDialog::updateAllScript()
{
    mScriptApi->clear();
    addBasicDefs(mScriptApi);

    mScriptLexer->clearDefinitions();
    mScriptLexer->clearBindings();

    mDefsLexer->clearDefinitions();

    updateScriptBindings();
    updateScriptDefinitions();

    mScriptApi->prepare();

    mScriptLexer->prepareDefinitions();
    mScriptLexer->prepareBindings();
    mScript->updateLexer();

    mDefsLexer->prepareDefinitions();
    mDefinitions->updateLexer();

    mDefinitionsChanged = false;
    mBindingsChanged = false;
}

void ExpressionDialog::updateScriptDefinitions()
{
    const QString defsText = mDefinitions->text();
    QString scriptContext;

    int nOpenBrackets = 0;
    for (const auto& c : defsText) {
        if (c == '{') { nOpenBrackets++; }
        else if (c == '}') { nOpenBrackets--; }
        else if (c == QChar::LineFeed) { continue; }
        else if (!nOpenBrackets) { scriptContext.append(c); }
    }

    {
        QRegExp funcDefs("(class|function)\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*(\\([a-zA-Z0-9_, ]*\\))");
        int pos = 0;
        while ((pos = funcDefs.indexIn(scriptContext, pos)) != -1) {
              QStringList funcs = funcDefs.capturedTexts();
              for (int i = 2; i < funcs.count() - 1; i += 3) {
                  const auto& func = funcs.at(i);
                  const auto& funcArgs = funcs.at(i + 1);
                  if (func.isEmpty()) { continue; }
                  mScriptApi->add(func + funcArgs);
                  mScriptLexer->addDefinition(func);
                  mDefsLexer->addDefinition(func);
              }
              pos += funcDefs.matchedLength();
        }
    }

    {
        QRegExp varDefs("([a-zA-Z_][a-zA-Z0-9_]*)\\s*=\\s*(?!=)");
        int pos = 0;
        while ((pos = varDefs.indexIn(scriptContext, pos)) != -1) {
              QStringList vars = varDefs.capturedTexts();
              for (int i = 1; i < vars.count(); i++) {
                  const auto& var = vars.at(i);
                  if (var.isEmpty()) { continue; }
                  mScriptApi->add(var);
                  mScriptLexer->addDefinition(var);
                  mDefsLexer->addDefinition(var);
              }
              pos += varDefs.matchedLength();
        }
    }
}

bool ExpressionDialog::getBindings(PropertyBindingMap& bindings)
{
    mBindingsError->clear();
    const auto bindingsStr = mBindings->text();
    try {
        bindings = PropertyBindingParser::parseBindings(bindingsStr,
                                                        nullptr,
                                                        mTarget);
        mBindingsButton->setIcon(QIcon());
        return true;
    } catch (const std::exception& e) {
        mBindingsButton->setIcon(mRedDotIcon);
        mBindingsError->setText(e.what());
        return false;
    }
}

#define BFC_0 "<font color=\"#FF8080\">"
#define BFC_1 "</font>"

void ExpressionDialog::updateScriptBindings()
{
    QStringList bindingList;
    PropertyBindingMap bindings;
    if (getBindings(bindings)) {
        for (const auto& binding : bindings) {
            bindingList << binding.first;
            mScriptApi->add(binding.first);
            mScriptLexer->addBinding(binding.first);
        }
    }
    mScriptLabel->setText(tr("Calculate") + " ( " BFC_0 +
                            bindingList.join(BFC_1 ", " BFC_0) +
                          BFC_1 " ) :");
}

bool ExpressionDialog::apply(const bool action)
{
    mBindingsButton->setIcon(QIcon());
    mDefinitionsButon->setIcon(QIcon());
    mDefinitionsError->clear();
    mScriptError->clear();

    const auto definitionsStr = mDefinitions->text();
    const auto scriptStr = mScript->text();

    PropertyBindingMap bindings;
    if (!getBindings(bindings)) { return false; }

    auto engine = std::make_unique<QJSEngine>();
    try {
        Expression::sAddDefinitionsTo(definitionsStr, *engine);
    } catch (const std::exception& e) {
        mDefinitionsError->setText(e.what());
        mDefinitionsButon->setIcon(mRedDotIcon);
        return false;
    }

    QJSValue eEvaluate;
    try {
        Expression::sAddScriptTo(scriptStr, bindings, *engine, eEvaluate,
                                 Expression::sQrealAnimatorTester);
    } catch (const std::exception& e) {
        mScriptError->setText(e.what());
        mBindingsButton->setIcon(mRedDotIcon);
        return false;
    }

    try {
        auto expr = Expression::sCreate(definitionsStr,
                                        scriptStr, std::move(bindings),
                                        std::move(engine),
                                        std::move(eEvaluate));
        if (expr && !expr->isValid()) { expr = nullptr; }
        if (action) {
            mTarget->setExpressionAction(expr);
        } else {
            mTarget->setExpression(expr);
        }
    } catch (const std::exception& e) {
        return false;
    }

    Document::sInstance->actionFinished();
    return true;
}

QWidget *ExpressionDialog::setupPresetsUi()
{
    const auto presetWidget = new QWidget(this);
    presetWidget->setContentsMargins(0, 0, 0, 0);

    const auto presetLayout = new QHBoxLayout(presetWidget);
    presetLayout->setContentsMargins(0, 0, 0, 0);

    mPresetsCombo = new QComboBox(this);
    mPresetsCombo->setFocusPolicy(Qt::ClickFocus);
    mPresetsCombo->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Preferred);
    mPresetsCombo->setToolTip(tr("Select a Preset from the list to fill Bindings, Definitions\n"
                                 "and Calculate fields. In case there is no Preset available,\n"
                                 "you can create a new one by clicking on the '+' button."));

    const auto presetLabel = new QLabel(tr("Preset"), this);

    const auto addPresetBtn = new QPushButton(QIcon::fromTheme("plus"),
                                              QString(), this);
    addPresetBtn->setToolTip(tr("Save as New Preset"));
    addPresetBtn->setFocusPolicy(Qt::NoFocus);

    const auto removePresetBtn = new QPushButton(QIcon::fromTheme("minus"),
                                                 QString(), this);
    removePresetBtn->setToolTip(tr("Remove Active Preset"));
    removePresetBtn->setFocusPolicy(Qt::NoFocus);

    const auto editPresetBtn = new QPushButton(QIcon::fromTheme("disk_drive"),
                                               QString(), this);
    editPresetBtn->setToolTip(tr("Save Active Preset"));
    editPresetBtn->setFocusPolicy(Qt::NoFocus);

    const auto importPresetBtn = new QPushButton(QIcon::fromTheme("file-import"),
                                                 QString(), this);
    importPresetBtn->setToolTip(tr("Import Preset from file"));
    importPresetBtn->setFocusPolicy(Qt::NoFocus);

    const auto exportPresetBtn = new QPushButton(QIcon::fromTheme("file-export"),
                                                 QString(), this);
    exportPresetBtn->setToolTip(tr("Export Active Preset to file"));
    exportPresetBtn->setFocusPolicy(Qt::NoFocus);

    presetLayout->addWidget(presetLabel);
    presetLayout->addWidget(mPresetsCombo);
    presetLayout->addWidget(addPresetBtn);
    presetLayout->addWidget(removePresetBtn);
    presetLayout->addWidget(editPresetBtn);
    presetLayout->addWidget(importPresetBtn);
    presetLayout->addWidget(exportPresetBtn);

    populatePresets(true);

    mPresetsCombo->setEditable(true);
    mPresetsCombo->setInsertPolicy(QComboBox::NoInsert);
    connect(mPresetsCombo->lineEdit(),
            &QLineEdit::editingFinished,
            this, [this]() {
        const QString id = mPresetsCombo->currentData().toString();
        const QString text = mPresetsCombo->currentText();
        const int index = mPresetsCombo->currentIndex();
        if (id.isEmpty() || text.trimmed().isEmpty() || index < 1) {
            mPresetsCombo->setCurrentText(mPresetsCombo->itemText(index));
            mScript->setFocus();
            fixLeaveEvent(mPresetsCombo);
            return;
        }
        if (mPresetsCombo->itemText(index) == text) {
            mScript->setFocus();
            fixLeaveEvent(mPresetsCombo);
            return;
        }
        if (mSettings->fExpressions.editExpr(id, text)) {
            populatePresets(true);
            const int newIndex = mPresetsCombo->findData(id);
            if (newIndex >= 0) {
                mPresetsCombo->setCurrentIndex(newIndex);
            }
        } else {
            mPresetsCombo->setCurrentText(mPresetsCombo->itemText(index));
        }
        mScript->setFocus();
        fixLeaveEvent(mPresetsCombo);
    });

    connect(mPresetsCombo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        const QString id = mPresetsCombo->itemData(index).toString();
        if (!id.isEmpty()) { applyPreset(id); }
    });

    connect(editPresetBtn,
            &QPushButton::released,
            this, [this]() {
        const QString id = mPresetsCombo->currentData().toString();
        if (id.isEmpty()) {
            QMessageBox::warning(this,
                                 tr("No Preset"),
                                 tr("No preset selected."));
            return;
        }
        if (!mSettings->fExpressions.editExpr(id,
                                              QString(),
                                              mDefinitions->text(),
                                              mBindings->text(),
                                              mScript->text())) {
            QMessageBox::warning(this,
                                 tr("Failed to change preset"),
                                 tr("Unable to edit preset, "
                                    "check file permissions."));
        }
    });

    connect(addPresetBtn,
            &QPushButton::released,
            this, &ExpressionDialog::savePreset);
    connect(exportPresetBtn,
            &QPushButton::released,
            this, &ExpressionDialog::exportPreset);

    connect(importPresetBtn,
            &QPushButton::released,
            this, [this]() {
        const QString preset = QFileDialog::getOpenFileName(this,
                                                            tr("Import Preset"),
                                                            AppSupport::getSettings("files",
                                                                                    "lastExprImportDir",
                                                                                    QDir::homePath()).toString(),
                                                            "Expressions (*.fexpr)");
        if (mSettings->fExpressions.isValidExprFile(preset)) {
            importPreset(preset);
        } else {
            QMessageBox::warning(this,
                                 tr("Failed to read preset"),
                                 tr("This file is not a valid expression preset."));
        }
    });

    connect(removePresetBtn,
            &QPushButton::released,
            this, [this]() {
                const int index = mPresetsCombo->currentIndex();
                const QString text = mPresetsCombo->currentText();
                const QString id = mPresetsCombo->currentData().toString();
                if (index < 1 || id.isEmpty()) { return; }
                const int ask = QMessageBox::question(this,
                                                      tr("Delete Preset?"),
                                                      tr("Are you sure you want "
                                                         "to remove '%1' preset?").arg(text));
                if (ask == QMessageBox::Yes) {
                    if (mSettings->fExpressions.remExpr(id)) {
                        populatePresets(true);
                    } else {
                        QMessageBox::warning(this,
                                             tr("Failed to remove"),
                                             tr("Failed to remove preset, check file permissions."));
                    }
                }
    });

    return presetWidget;
}

void ExpressionDialog::populatePresets(const bool &clear)
{
    if (clear) {
        mPresetsCombo->clear();
        mPresetsCombo->addItem(tr("Select ..."));
    }
    auto expressions = mSettings->fExpressions.getUser();
    std::sort(expressions.begin(),
              expressions.end(), [](const auto &a,
                                    const auto &b) {
        return a.title.toLower() < b.title.toLower();
    });
    for (const auto &expr : expressions) {
        mPresetsCombo->addItem(expr.title, expr.id);
    }
}

void ExpressionDialog::exportPreset()
{
    const QString bindings = mBindings->text();
    const QString definitions = mDefinitions->text();
    const QString script = mScript->text();

    if (bindings.trimmed().isEmpty() &&
        definitions.trimmed().isEmpty() &&
        script.trimmed().isEmpty()) { return; }

    ExpressionPresets::Expr expr;
    const int index = mPresetsCombo->currentIndex();
    if (index >= 1) {
        const QString currentId = mPresetsCombo->currentData().toString();
        const auto currentExpr = mSettings->fExpressions.getExpr(currentId);
        if (currentExpr.valid) { expr = currentExpr; }
    }

    expr.valid = true;
    expr.enabled = true;

    if (expr.version < 0.1) {
        expr.version = 1.0;
    }
    if (expr.title.trimmed().isEmpty()) {
        expr.title = tr("New Preset");
    }
    if (expr.id.trimmed().isEmpty()) {
        expr.id = genPresetId(QString());
    }

    expr.bindings = bindings;
    expr.definitions = definitions;
    expr.script = script;

    if (!editDialog(tr("Export Preset"), &expr)) { return; }

    QString path = QFileDialog::getSaveFileName(this,
                                                tr("Export Preset"),
                                                AppSupport::getSettings("files",
                                                                        "lastExprExportDir",
                                                                        QDir::homePath()).toString(),
                                                "Expressions (*.fexpr)");
    if (path.trimmed().isEmpty()) { return; }
    if (QFileInfo(path).suffix() != "fexpr") { path.append(".fexpr"); }

    if (mSettings->fExpressions.saveExpr(expr, path)) {
        QMessageBox::information(this,
                                 tr("Export Success"),
                                 tr("Your new preset has been exported to %1.").arg(path));
        AppSupport::setSettings("files",
                                "lastExprExportDir",
                                QFileInfo(path).absoluteDir().absolutePath());
    } else {
        QMessageBox::warning(this,
                             tr("Export Failed"),
                             tr("Unable to export preset to %1.").arg(path));
    }
}

void ExpressionDialog::importPreset(const QString& path)
{
    if (!QFile::exists(path)) { return; }
    if (!mSettings->fExpressions.isValidExprFile(path)) {
        return;
    }

    auto expr = mSettings->fExpressions.readExpr(path);
    if (mSettings->fExpressions.hasExpr(expr.id)) {
        QMessageBox::warning(this,
                             tr("Expression exists"),
                             tr("An expression with id %1 already exists.").arg(expr.id));
        return;
    }

    const QString newPath = QString("%1/%2.fexpr").arg(AppSupport::getAppUserExPresetsPath(),
                                                       filterPresetId(expr.id));
    if (!mSettings->fExpressions.saveExpr(expr, newPath)) {
        QMessageBox::warning(this,
                             tr("Save Failed"),
                             tr("Unable to save preset %1.").arg(newPath));
    } else {
        expr.path = newPath;
        mSettings->fExpressions.addExpr(expr);
        if (!expr.highlighters.isEmpty()) {
            for (const auto &highlight : expr.highlighters) {
                mScriptApi->add(highlight);
            }
        }
        populatePresets(true);
        const int index = mPresetsCombo->findData(expr.id);
        if (index >= 1) {
            mPresetsCombo->setCurrentIndex(index);
        }
        AppSupport::setSettings("files",
                                "lastExprImportDir",
                                QFileInfo(path).absoluteDir().absolutePath());
    }
}

void ExpressionDialog::savePreset()
{
    const QString bindings = mBindings->text();
    const QString definitions = mDefinitions->text();
    const QString script = mScript->text();

    const bool hasDef = !definitions.trimmed().isEmpty();
    const bool hasBind = !bindings.isEmpty();
    const bool hasScript = !script.trimmed().isEmpty();
    const bool onlyDef = hasDef && !hasBind && !hasScript;

    if (!hasDef && !hasBind && !hasScript) { return; }

    ExpressionPresets::Expr expr;
    expr.valid = true;
    expr.enabled = true;
    expr.version = 1.0;
    expr.title = tr("New Preset");
    expr.id = genPresetId(QString());
    expr.bindings = bindings;
    expr.definitions = definitions;
    expr.script = script;
    expr.path = QString("%1/%2.fexpr").arg(AppSupport::getAppUserExPresetsPath(),
                                           expr.id);

    if (!editDialog(tr("Save Preset"), &expr, false)) { return; }

    if (onlyDef) {
        const auto lines = definitions.split("\n");
        for (const auto &line : lines) {
            if (line.startsWith("function")) {
                QStringList split = line.split("function");
                if (split.count() >= 2) { expr.highlighters << split.at(1)
                                                                    .trimmed()
                                                                    .split("{")
                                                                    .takeFirst()
                                                                    .trimmed(); }
            }
        }
    }

    if (!mSettings->fExpressions.saveExpr(expr, expr.path)) {
        QMessageBox::warning(this,
                             tr("Save Failed"),
                             tr("Unable to save preset %1.").arg(expr.path));
    } else {
        mSettings->fExpressions.addExpr(expr);
        if (!expr.highlighters.isEmpty()) {
            for (const auto &highlight : expr.highlighters) {
                mScriptApi->add(highlight);
            }
        }
        populatePresets(true);
        const int index = mPresetsCombo->findData(expr.id);
        if (index >= 1) {
            mPresetsCombo->blockSignals(true);
            mPresetsCombo->setCurrentIndex(index);
            mPresetsCombo->blockSignals(false);
        }
    }
}

void ExpressionDialog::applyPreset(const QString &id)
{
    if (!mSettings->fExpressions.hasExpr(id)) { return; }

    const auto expr = mSettings->fExpressions.getExpr(id);

    const bool hasBind = !expr.bindings.isEmpty();
    const bool hasScript = !expr.script.isEmpty();

    if (!hasBind && !hasScript) { return; }

    mDefinitions->clearFillerText();
    mDefinitions->setText(expr.definitions);

    mBindings->clearFillerText();
    mBindings->setText(expr.bindings);

    mScript->clearFillerText();
    mScript->setText(expr.script);

    updateAllScript();

    mScript->setFocus();
    fixLeaveEvent(mPresetsCombo);

    if (hasScript) { apply(true); }
}

const QString ExpressionDialog::genPresetId(const QString &title)
{
    QString uid = QUuid::createUuid().toString();
    if (!title.isEmpty()) {
        uid.append(QString("_%1").arg(title));
    }
    return filterPresetId(uid);
}

const QString ExpressionDialog::filterPresetId(const QString &id)
{
    static QRegularExpression rx("[^a-zA-Z0-9-_]");
    QString result(id);
    return result.replace(rx, "");
}

void ExpressionDialog::fixLeaveEvent(QWidget *widget)
{
    if (!widget) { return; }
    QEvent event(QEvent::Leave);
    QApplication::sendEvent(widget, &event);
}

bool ExpressionDialog::editDialog(const QString &title,
                                  ExpressionPresets::Expr *expr,
                                  const bool &showId)
{
    if (!expr) { return false; }

    Friction::Ui::Dialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.setMinimumWidth(400);

    QVBoxLayout layout(&dialog);

    QHBoxLayout layoutVer;
    QLabel labelVer(tr("Version"), &dialog);
    QLineEdit editVer(&dialog);

    layoutVer.addWidget(&labelVer);
    layoutVer.addWidget(&editVer);

    QHBoxLayout layoutId;
    QLabel labelId(tr("ID"), &dialog);
    QLineEdit editId(&dialog);

    if (showId) {
        layoutId.addWidget(&labelId);
        layoutId.addWidget(&editId);
    } else {
        labelId.setVisible(false);
        editId.setVisible(false);
    }

    QHBoxLayout layoutTitle;
    QLabel labelTitle(tr("Title"), &dialog);
    QLineEdit editTitle(&dialog);

    layoutTitle.addWidget(&labelTitle);
    layoutTitle.addWidget(&editTitle);

    QHBoxLayout layoutAuthor;
    QLabel labelAuthor(tr("Author"), &dialog);
    QLineEdit editAuthor(&dialog);

    layoutAuthor.addWidget(&labelAuthor);
    layoutAuthor.addWidget(&editAuthor);

    QHBoxLayout layoutUrl;
    QLabel labelUrl(tr("Url"), &dialog);
    QLineEdit editUrl(&dialog);

    layoutUrl.addWidget(&labelUrl);
    layoutUrl.addWidget(&editUrl);

    QHBoxLayout layoutDesc;
    QLabel labelDesc(tr("Description"), &dialog);
    QTextEdit editDesc(&dialog);
    editDesc.setObjectName("TextEdit");

    layoutDesc.addWidget(&labelDesc);
    layoutDesc.addWidget(&editDesc);

    QHBoxLayout layoutLic;
    QLabel labelLic(tr("License"), &dialog);
    QLineEdit editLic(&dialog);

    layoutLic.addWidget(&labelLic);
    layoutLic.addWidget(&editLic);

    layout.addLayout(&layoutVer);
    if (showId) { layout.addLayout(&layoutId); }
    layout.addLayout(&layoutTitle);
    layout.addLayout(&layoutAuthor);
    layout.addLayout(&layoutUrl);
    layout.addLayout(&layoutLic);
    layout.addLayout(&layoutDesc);

    QSizePolicy labelSizePolicy(QSizePolicy::Expanding,
                                QSizePolicy::Preferred);
    const int labelSize = labelDesc.width();

    labelVer.setSizePolicy(labelSizePolicy);
    labelVer.setMinimumWidth(labelSize);
    labelVer.setMaximumWidth(labelSize);

    if (showId) {
        labelId.setSizePolicy(labelSizePolicy);
        labelId.setMinimumWidth(labelSize);
        labelId.setMaximumWidth(labelSize);
    }

    labelTitle.setSizePolicy(labelSizePolicy);
    labelTitle.setMinimumWidth(labelSize);
    labelTitle.setMaximumWidth(labelSize);

    labelAuthor.setSizePolicy(labelSizePolicy);
    labelAuthor.setMinimumWidth(labelSize);
    labelAuthor.setMaximumWidth(labelSize);

    labelUrl.setSizePolicy(labelSizePolicy);
    labelUrl.setMinimumWidth(labelSize);
    labelUrl.setMaximumWidth(labelSize);

    labelDesc.setSizePolicy(labelSizePolicy);
    labelDesc.setMinimumWidth(labelSize);
    labelDesc.setMaximumWidth(labelSize);

    labelLic.setSizePolicy(labelSizePolicy);
    labelLic.setMinimumWidth(labelSize);
    labelLic.setMaximumWidth(labelSize);

    editVer.setText(QString::number(expr->version));
    if (showId) { editId.setText(expr->id); }
    editTitle.setText(expr->title);
    editAuthor.setText(expr->author);
    editUrl.setText(expr->url);
    editDesc.setText(expr->description);
    editLic.setText(expr->license);

    editTitle.setFocus();

    QHBoxLayout buttonLayout;
    QPushButton noButton(tr("Cancel"), &dialog);
    QPushButton yesButton(tr("Save"), &dialog);

    buttonLayout.addWidget(&yesButton);
    buttonLayout.addWidget(&noButton);
    layout.addLayout(&buttonLayout);

    yesButton.setDefault(true);

    connect(&noButton, &QPushButton::clicked,
            &dialog, &QDialog::reject);
    connect(&yesButton, &QPushButton::clicked,
            &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    expr->version = editVer.text().trimmed().replace(",", ".").toDouble();
    expr->title = editTitle.text().trimmed();
    expr->author = editAuthor.text().trimmed();
    expr->url = editUrl.text().trimmed();
    expr->description = editDesc.toPlainText().trimmed();
    expr->license = editLic.text().trimmed();

    if (showId) {
        QString exprId = editId.text().trimmed();
        if (exprId.isEmpty()) { exprId = genPresetId(expr->title); }
        expr->id = exprId;
    }

    return true;
}
