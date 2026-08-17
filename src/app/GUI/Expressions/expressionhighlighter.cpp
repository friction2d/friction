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

#include "expressionhighlighter.h"

#include "Animators/complexanimator.h"
#include "Animators/qpointfanimator.h"
#include "Animators/coloranimator.h"

#include "expressioneditor.h"

ExpressionHighlighter::ExpressionHighlighter(
        Property * const target,
        ExpressionEditor * const editor,
        QTextDocument *parent) :
    QSyntaxHighlighter(parent),
    mTarget(target),
    mSearchCtxt(target->getParent()),
    mEditor(editor) {    
    HighlightingRule rule;

    mErrorFormat.setForeground(Qt::red);
    mAssignFormat.setForeground(QColor(255, 128, 128));

    const QString propPath = "([a-zA-Z0-9_\\.]+[a-zA-Z0-9_ \\.]*[a-zA-Z0-9_\\.]+)";
    mPropSetRegex = QRegularExpression("^\\s*"
                                            "([A-Za-z_][A-Za-z0-9_]*)"
                                       "\\s*=\\s*" + propPath);
    mFrameValueSetRegex = QRegularExpression("^\\s*"
                                                 "([A-Za-z_][A-Za-z0-9_]*)"
                                             "\\s*=\\s*(\\$frame|\\$scene.fps|\\$scene.width|\\$scene.height|\\$scene.rangeMin|\\$scene.rangeMax|\\$value)");
//    const auto propPathRegex = QRegularExpression(propPath);

    mPropPathFormat.setFontWeight(QFont::Bold);
    mPropPathFormat.setForeground(Qt::white);
//    rule.pattern = propPathRegex;
//    rule.format = mPropPathFormat;
//    mHighlightingRules.append(rule);

    const QStringList specs = {
        QStringLiteral("$value"),
        QStringLiteral("$frame"),
        QStringLiteral("$scene.fps"),
        QStringLiteral("$scene.width"),
        QStringLiteral("$scene.height"),
        QStringLiteral("$scene.rangeMin"),
        QStringLiteral("$scene.rangeMax")
    };

    QTextCharFormat specialFormat;
    specialFormat.setForeground(QColor(185, 255, 155));
    rule.format = specialFormat;
    for(const QString &spec : specs) {
        rule.pattern = QRegularExpression(QString("\\%1\\b").arg(spec));
        mHighlightingRules.append(rule);
        mBaseComplete << spec;
    }

    QTextCharFormat commentFormat;
    commentFormat.setForeground(QColor("#666666"));
    rule.pattern = QRegularExpression("\\/\\/.*");;
    rule.format = commentFormat;
    mHighlightingRules.append(rule);

    mBaseVarsComplete = mBaseComplete;
}

void ExpressionHighlighter::setCursor(const QTextCursor& cursor) {
    mCursorPos = cursor.positionInBlock();
    const auto oldBlock = mOldCursor.block();
    if(oldBlock.isValid()) rehighlightBlock(oldBlock);
    rehighlightBlock(cursor.block());
    mOldCursor = cursor;
}

void ExpressionHighlighter::addVariable(const QString &name) {
    mVariables << name;
}

void ExpressionHighlighter::removeVariable(const QString &name){
    mVariables.removeAll(name);
}

void ExpressionHighlighter::highlightBlock(const QString &text) {
    for(const auto &rule : qAsConst(mHighlightingRules)) {
        auto matchIterator = rule.pattern.globalMatch(text);
        while(matchIterator.hasNext()) {
            const auto match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    auto matchIterator = mFrameValueSetRegex.globalMatch(text);
    while(matchIterator.hasNext()) {
        const auto match = matchIterator.next();
        const auto captured = match.capturedTexts();
        if(captured.count() < 3) continue;
        const int min = match.capturedStart(1);
        const int max = match.capturedEnd(1);
        setFormat(min, max - min, mAssignFormat);
    }

    bool objCompletSetup = false;
    bool addFuncsComplete = true;
    QStringList completions;
    matchIterator = mPropSetRegex.globalMatch(text);
    while(matchIterator.hasNext()) {
        const auto match = matchIterator.next();
        const auto captured = match.capturedTexts();
        if(captured.count() < 3) continue;
        {
            const int min = match.capturedStart(1);
            const int max = match.capturedEnd(1);
            setFormat(min, max - min, mAssignFormat);
        }
        const auto path = captured[2];
        const auto objs = path.split('.');
        for(int i = objs.count() - 1; i >= 0; i--) {
            const auto subPath = objs.mid(0, objs.count() - i);
            int min = match.capturedStart(2);
            for(int j = 0; j < subPath.count() - 1; j++) {
                const auto& jObj = subPath.at(j);
                min += jObj.count() + 1;
            }
            const auto& lastObjPath = subPath.last();
            const int max = min + lastObjPath.count();
            const bool autocomplete = mCursorPos >= min && mCursorPos <= max;
            const auto obj = mSearchCtxt->ca_findPropertyWithPathRec(
                        0, subPath, autocomplete ? &completions : nullptr);
            if(autocomplete) {
                if(objs.count() > 1) addFuncsComplete = false;
                completions.removeDuplicates();
                objCompletSetup = true;
            }

            bool error = false;
            const auto qra = enve_cast<QrealAnimator*>(obj);
            const auto ca = enve_cast<ComplexAnimator*>(obj);
            const auto pa = enve_cast<QPointFAnimator*>(obj);
            const auto co = enve_cast<ColorAnimator*>(obj);

            const bool validFinal = qra || pa || co;

            if(obj == mTarget) error = true;
            else if(!ca && !validFinal) error = true;
            else if(i == 0) error = !validFinal || obj->prp_dependsOn(mTarget);
            if(error) {
                const int len = match.capturedEnd() - min;
                setFormat(min, len, mErrorFormat);
                break;
            } else {
                setFormat(min, max - min, mPropPathFormat);
            }
        }
    }
    if(!objCompletSetup) {
        mSearchCtxt->ca_findPropertyWithPathRec(
                    0, QStringList() << "", &completions);
    }
    if(addFuncsComplete) {
        QStringList tmp = mBaseVarsComplete;
        tmp.append(completions);
        completions = tmp;
    }
    mEditor->setCompleterList(completions);
}
