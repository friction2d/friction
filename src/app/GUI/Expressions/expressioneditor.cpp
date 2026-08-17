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

#include "expressioneditor.h"

#include <QScrollBar>

#include "expressionhighlighter.h"
#include "themesupport.h"

ExpressionEditor::ExpressionEditor(Property * const target,
                                   const QString &text,
                                   QWidget * const parent)
    : QTextEdit(parent)
{
    setCursorWidth(2);
    setFont(parent->font());
    setMinimumWidth(20*eSizesUI::widget);
    setMaximumHeight(5*eSizesUI::widget);
    const auto doc = document();
    setAcceptRichText(false);
    mHighlighter = new ExpressionHighlighter(target, this, doc);
    connect(this, &QTextEdit::cursorPositionChanged,
            this, [this]() {
        const auto cursor = textCursor();
        mHighlighter->setCursor(cursor);
    });
    mCompleter = new QCompleter(this);
    mCompleter->setWidget(this);
    mCompleter->setCompletionMode(QCompleter::PopupCompletion);
    mCompleter->setCaseSensitivity(Qt::CaseSensitive);
    connect(mCompleter,
            qOverload<const QString&>(&QCompleter::activated),
            this, &ExpressionEditor::insertCompletion);
    if(text.isEmpty()) setFillerText();
    else setText(text);
}

void ExpressionEditor::setCompleterList(const QStringList &values) {
    mCompleter->setModel(new QStringListModel(values, mCompleter));
}

QString ExpressionEditor::text() const {
    if(mFillerText) return QString();
    else return toPlainText();
}

void ExpressionEditor::keyPressEvent(QKeyEvent *e) {
    const auto key = e->key();
    const bool wasComplete = mCompleter->popup()->isVisible();
    const auto mods = e->modifiers();
    const bool ctrlPressed = mods.testFlag(Qt::ControlModifier);
    const bool spacePressed = key == Qt::Key_Space;
    const bool isShortcut = ctrlPressed && spacePressed;
    const auto popup = mCompleter->popup();
    const bool popupVisible = popup->isVisible();
    if((key == Qt::Key_Return || key == Qt::Key_Tab) && popupVisible) {
        return QWidget::keyPressEvent(e);
    } else if(isShortcut) {
        showCompleter();
    } else if(key == Qt::Key_Tab) {
        return QWidget::keyPressEvent(e);
    } else QTextEdit::keyPressEvent(e);
    const bool input = e->text().contains(QRegExp("[A-Za-z0-9_ \\.\\$]"));
    const bool deletion = key == Qt::Key_Delete ||
            key == Qt::Key_Backspace;
    const bool arrows = key == Qt::Key_Right ||
            key == Qt::Key_Left;
    const bool complete = input || deletion || (arrows && wasComplete);
    if(complete) {
        QTimer::singleShot(1, this, [this]() { showCompleter(); });
    }
}

void ExpressionEditor::focusInEvent(QFocusEvent* e) {
    clearFillerText();
    QTextEdit::focusInEvent(e);
}

void ExpressionEditor::focusOutEvent(QFocusEvent* e) {
    Q_UNUSED(e)
    if(toPlainText().isEmpty()) setFillerText();
    else mFillerText = false;
    QTextEdit::focusOutEvent(e);
    emit focusLost();
}

void ExpressionEditor::setFillerText() {
    if(mFillerText) return;
    mFillerText = true;
    setText("// Here you can bind property values, e.g.:\n"
            "// varName = transform.rotation\n"
            "// Where 'varName' is the name of the variable you can use\n"
            "// from the 'Calculate' portion of the script,\n"
            "// and 'transform.rotation' is the property\n"
            "// the variable will get its value from.");
}

void ExpressionEditor::clearFillerText() {
    if(mFillerText) {
        mFillerText = false;
        setText("");
    }
}

void ExpressionEditor::showCompleter() {
    const auto popup = mCompleter->popup();
    const auto underCursor = textUnderCursor();
    mCompleter->setCompletionPrefix(underCursor);
    QRect cr = cursorRect();
    const auto scrollBar = popup->verticalScrollBar();
    const int scrollBarWidth = scrollBar->sizeHint().width();
    cr.setWidth(popup->sizeHintForColumn(0) + scrollBarWidth);
    mCompleter->complete(cr);
}

void ExpressionEditor::insertCompletion(const QString &completion) {
    if(completion.isEmpty()) return;
    QTextCursor tc = textCursor();
    const int prefixLen = mCompleter->completionPrefix().length();
    const int extra = completion.length() - prefixLen;
    bool afterDot = false;
    {
        auto checkDot = tc;
        checkDot.movePosition(QTextCursor::Left,
                              QTextCursor::KeepAnchor);
        const auto selected = checkDot.selectedText();
        if(!selected.isEmpty()) {
            const auto prevCharacter = selected.front();
            if(prevCharacter == '.') afterDot = true;
        }
    }

    if(!afterDot) {
        tc.movePosition(QTextCursor::Left);
        tc.movePosition(QTextCursor::EndOfWord);
    }
    tc.insertText(completion.right(extra));

    if(completion.right(1) == ')') tc.movePosition(QTextCursor::Left);
    setTextCursor(tc);
}

QString ExpressionEditor::textUnderCursor() const {
    QTextCursor tc = textCursor();
    QChar prevCharacter;
    {
        auto checkSpace = tc;
        checkSpace.movePosition(QTextCursor::Left,
                                QTextCursor::KeepAnchor);
        const auto selected = checkSpace.selectedText();
        if(selected.isEmpty()) prevCharacter = ' ';
        else prevCharacter = selected.front();
    }
    if(prevCharacter == ' ') return "";
    else if(prevCharacter == '$') return "$";
    else if(!prevCharacter.isLetter() && !prevCharacter.isDigit()) return "";

    tc.movePosition(QTextCursor::Left);
    tc.select(QTextCursor::WordUnderCursor);

    QString result = tc.selectedText();
    {
        auto checkVar = tc;
        checkVar.movePosition(QTextCursor::Left,
                              QTextCursor::MoveAnchor);
        checkVar.movePosition(QTextCursor::Left,
                              QTextCursor::KeepAnchor);
        const auto selected = checkVar.selectedText();
        if(selected == "$") result.prepend('$');
    }
    return result;
}
