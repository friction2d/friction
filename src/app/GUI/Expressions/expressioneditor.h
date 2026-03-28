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

#ifndef EXPRESSIONEDITOR_H
#define EXPRESSIONEDITOR_H

#include <QTextEdit>
#include <QCompleter>

#include <QStringListModel>
#include <QAbstractItemView>

#include "GUI/global.h"
#include "Properties/property.h"

class ExpressionHighlighter;

class ExpressionEditor : public QTextEdit
{
    Q_OBJECT

public:
    ExpressionEditor(Property* const target,
                     const QString& text, QWidget* const parent);

    void setCompleterList(const QStringList& values);

    QString text() const;
    void setFillerText();
    void clearFillerText();

signals:
    void focusLost();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;

private:
    using QTextEdit::toHtml;
    using QTextEdit::toPlainText;
    void showCompleter();
    void insertCompletion(const QString &completion);
    QString textUnderCursor() const;
    bool mFillerText = false;
    ExpressionHighlighter* mHighlighter;
    QCompleter* mCompleter;
};

#endif // EXPRESSIONEDITOR_H
