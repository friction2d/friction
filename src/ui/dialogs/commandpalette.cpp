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

#include "commandpalette.h"

#include <QVBoxLayout>
#include <QKeyEvent>
#include <QRegularExpression>

#include "Private/esettings.h"
#include "Private/scene.h"

#define ITEM_ACTION_ROLE Qt::UserRole
#define ITEM_CMD_ROLE Qt::UserRole + 1
#define ITEM_TOOLTIP_ROLE Qt::UserRole + 2

CommandPalette::CommandPalette(Document& document,
                               QWidget *parent)
    : QDialog(parent)
    , mDocument(document)
    , mUserInput(nullptr)
    , mLabel(nullptr)
    , mSuggestions(nullptr)
    , mHistoryCounter(-1)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint
#ifdef Q_OS_MAC
                   | Qt::WindowStaysOnTopHint
#endif
                   );
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setWindowTitle(tr("Command Palette"));

    setMinimumWidth(350);

    const auto lay = new QVBoxLayout(this);

    mSuggestions = new QListWidget(this);
    mLabel = new QLabel(this);
    mUserInput = new QLineEdit(this);

    lay->addWidget(mUserInput);
    lay->addWidget(mLabel);
    lay->addWidget(mSuggestions);
    lay->addStretch();

    mSuggestions->setObjectName("CommandPaletteSuggestions");
    mSuggestions->setMinimumHeight(150);
    mSuggestions->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mSuggestions->hide();

    mLabel->setWordWrap(true);
    mLabel->setObjectName("CommandPaletteLabel");
    mLabel->hide();

    mUserInput->setObjectName("CommandPaletteInput");
    mUserInput->setPlaceholderText(tr("Search for action ..."));
    mUserInput->addAction(QIcon::fromTheme("zoom"), QLineEdit::LeadingPosition);
    mUserInput->installEventFilter(this);
    mUserInput->setFocus();

#ifdef Q_OS_MAC
    mSuggestions->installEventFilter(this);
#endif

    connect(mUserInput, &QLineEdit::textChanged, this, [this](const auto &input) {
        mSuggestions->clear();
        mSuggestions->hide();
        adjustSize();

        if (input.isEmpty()) { return; }
        if (isCmd(input)) { return; }

        const auto actions = eSettings::instance().fCommandPalette;
        const auto defIcon = QIcon::fromTheme("drawPathAutoChecked");

        int items = 0;
        for (int i = 0; i < actions.count(); i++) {
            const auto act = actions.at(i);
            if (!act->isEnabled()) { continue; }
            QString alt = act->data().toString();

            // act cmd?
            const bool isCmd = isActionCmd(alt);
            if (isCmd) { alt.clear(); }

            // skip or match
            if (skipItem(alt.isEmpty() ? act->text() : alt, input)) { continue; }

            const auto item = new QListWidgetItem(act->text(), mSuggestions);
            item->setData(ITEM_ACTION_ROLE, i);

            // set alt text as title if any
            if (!alt.isEmpty()) { item->setText(alt); }

            if (isCmd) {
                item->setData(ITEM_CMD_ROLE, act->data().toString().split("cmd:").takeLast().simplified());
                item->setData(ITEM_TOOLTIP_ROLE, act->toolTip());
            }

            if (!act->icon().isNull()) { item->setIcon(act->icon()); }
            else { item->setIcon(defIcon); }
            mSuggestions->addItem(item);
            items++;
        }

        if (mSuggestions->isHidden() && items > 0) {
            mSuggestions->show();
            if (mLabel->isVisible()) { mLabel->hide(); }
            adjustSize();
        }
    });

    connect(mUserInput, &QLineEdit::returnPressed, this, [this]() {
        const auto input = mUserInput->text();
        if (isCmd(input)) {
            parseCmd(input);
            return;
        }
        if (mSuggestions->count() < 1) { return; }
        const auto item = mSuggestions->item(0);
        if (!item) { return; }
        const auto cmd = item->data(ITEM_CMD_ROLE).toString();
        const auto tooltip = item->data(ITEM_TOOLTIP_ROLE).toString();
        const auto act = getAction(item->data(ITEM_ACTION_ROLE).toInt());
        if (!act) { return; }
        if (!cmd.isEmpty()) {
            mUserInput->setText(QString("%1:").arg(cmd));
            mUserInput->setCursorPosition(mUserInput->text().length());
            updateToolTip(tooltip);
            return;
        }
        act->trigger();
        appendHistory(item->text());
        accept();
    });

    connect(mSuggestions, &QListWidget::itemActivated, this, [this](const auto item) {
        if (!item) { return; }
        const auto cmd = item->data(ITEM_CMD_ROLE).toString();
        const auto tooltip = item->data(ITEM_TOOLTIP_ROLE).toString();
        const auto act = getAction(item->data(ITEM_ACTION_ROLE).toInt());
        if (!act) { return; }
        if (!cmd.isEmpty()) {
            mUserInput->setFocus();
            mUserInput->setText(QString("%1:").arg(cmd));
            mUserInput->setCursorPosition(mUserInput->text().length());
            updateToolTip(tooltip);
            return;
        }
        act->trigger();
        appendHistory(item->text());
        accept();
    });
}

bool CommandPalette::skipItem(const QString &title,
                              const QString &input)
{
    if (title.isEmpty() || input.isEmpty()) { return true; }
    if (!title.startsWith(input,
                          Qt::CaseInsensitive) && input != "?") { return true; }
    return false;
}

bool CommandPalette::isCmd(const QString &input)
{
    if (input.contains(":") || /* use contains for now, but probably not the best solution */
        input.startsWith("+") ||
        input.startsWith("-")) { return true; }
    return false;
}

void CommandPalette::parseCmd(const QString &input)
{
    if (input.isEmpty()) { return; }
    qDebug() << "parse cmd" << input;

    // multiple?
    if (input.contains(" && ")) {
        for (const auto &cmd : input.split("&&")) { parseCmd(cmd.simplified()); }
        return;
    }

    static QRegularExpression validFrameCmd("^[:][-]?[0-9]*[sm]?$");
    const bool goToFrame = (validFrameCmd.match(input).hasMatch() && input != ":");
    qDebug() << "go to frame?" << goToFrame;

    static QRegularExpression validFrameSkipNextCmd("^\\+[0-9]*[sm]?$");
    const bool skipToNextFrame = (validFrameSkipNextCmd.match(input).hasMatch() && input != "+");
    qDebug() << "skip to next frame?" << skipToNextFrame;

    static QRegularExpression validFrameSkipPrevCmd("^\\-[0-9]*[sm]?$");
    const bool skipToPrevFrame = (validFrameSkipPrevCmd.match(input).hasMatch() && input != "-");
    qDebug() << "skip to prev frame?" << skipToPrevFrame;

    static QRegularExpression validRotateCmd("^rotate[:][-]?[0-9,.]*$");
    const bool doRotate = (validRotateCmd.match(input).hasMatch() && input != "rotate:");
    qDebug() << "do rotate?" << doRotate;

    static QRegularExpression validScaleCmd("^scale[:][-]?[0-9,.]*$");
    const bool doScale = (validScaleCmd.match(input).hasMatch() && input != "scale:");
    qDebug() << "do scale?" << doScale;

    static QRegularExpression validMoveCmd("^move[:][-]?[0-9,.x]*$");
    const bool doMove = (validMoveCmd.match(input).hasMatch() && input != "move:");
    qDebug() << "do move?" << doMove;

    const bool doMarkers = (input.startsWith("marker:") && input != "marker:");
    qDebug() << "do markers?" << doMarkers;

    static QRegularExpression validFrameInCmd("^in[:][-]?[0-9]*[sm]?$");
    const bool doFrameIn = (validFrameInCmd.match(input).hasMatch() && input != "in:");
    qDebug() << "do frame in?" << doFrameIn;

    static QRegularExpression validFrameOutCmd("^out[:][-]?[0-9]*[sm]?$");
    const bool doFrameOut = (validFrameOutCmd.match(input).hasMatch() && input != "out:");
    qDebug() << "do frame out?" << doFrameOut;

    if (goToFrame || skipToNextFrame || skipToPrevFrame) {
        QString frame = input.simplified();
        const bool hasSec = frame.endsWith("s");
        const bool hasMin = frame.endsWith("m");
        if (hasSec && hasMin) { return; }
        if (!isIntOrDouble(frame.replace(goToFrame ? ":" : (skipToNextFrame ? "+" : "-"), "")
                                .replace("m", "")
                                .replace("s", ""))) { return; }
        int value = frame.toInt();
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        if (hasSec) { value *= scene->fps(); }
        else if (hasMin) { value = (value * 60) * scene->fps(); }

        if (!goToFrame) { value = skipToNextFrame ? scene->getCurrentFrame() + value : scene->getCurrentFrame() - value; }
        qDebug() << "go to or skip to frame" << value;
        scene->anim_setAbsFrame(value);
        mDocument.actionFinished();
        appendHistory(input);
        accept();
        return;
    }

    if (doRotate) {
        const QString arg = input.split(":").takeLast().simplified().replace(",", ".");
        if (!isIntOrDouble(arg)) { return; }
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        qDebug() << "do rotate" << arg;
        scene->rotateSelectedBoxesStartAndFinish(arg.toDouble(), false);
        mDocument.actionFinished();
        appendHistory(input);
        accept();
        return;
    }

    if (doScale) {
        const QString arg = input.split(":").takeLast().simplified().replace(",", ".");
        if (!isIntOrDouble(arg)) { return; }
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        qDebug() << "do scale" << arg;
        scene->scaleSelectedBoxesStartAndFinish(arg.toDouble());
        mDocument.actionFinished();
        appendHistory(input);
        accept();
        return;
    }

    if (doMove) {
        const QStringList args = input.split(":").takeLast().simplified().replace(",", ".").split("x");
        if (args.count() != 2) { return; }
        const QString moveX = args.at(0);
        const QString moveY = args.at(1);
        if (!isIntOrDouble(moveX) || !isIntOrDouble(moveY)) { return; }
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        qDebug() << "do move" << moveX << moveY;
        scene->moveSelectedBoxesStartAndFinish({moveX.toDouble(),
                                                moveY.toDouble()});
        mDocument.actionFinished();
        appendHistory(input);
        accept();
        return;
    }

    if (doMarkers) {
        const QString args = input.split(":").takeLast().simplified();
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        for (const auto &arg : args.split(",")) {
            const auto args = arg.split(" ");
            bool hasTitle = args.count() > 1;
            QString mark = hasTitle ? args.at(0).simplified() : arg.simplified();
            const bool hasSec = mark.endsWith("s");
            const bool hasMin = mark.endsWith("m");
            if (hasSec && hasMin) { continue; }
            if (!isIntOrDouble(mark.replace("m", "").replace("s", ""))) { continue; }
            int value = mark.toInt();
            if (hasSec) { value *= scene->fps(); }
            else if (hasMin) { value = (value * 60) * scene->fps(); }
            qDebug() << "do marker" << value;
            if (hasTitle) { scene->setMarker(args.mid(1).join(" "), value); }
            else { scene->setMarker(value); }
        }
        appendHistory(input);
        accept();
        return;
    }

    if (doFrameIn || doFrameOut) {
        QString frame = input.split(":").takeLast().simplified();
        const bool hasSec = frame.endsWith("s");
        const bool hasMin = frame.endsWith("m");
        if (hasSec && hasMin) { return; }
        if (!isIntOrDouble(frame.replace("m", "")
                                .replace("s", ""))) { return; }
        int value = frame.toInt();
        const auto scene = *mDocument.fActiveScene;
        if (!scene) { return; }
        if (hasSec) { value *= scene->fps(); }
        else if (hasMin) { value = (value * 60) * scene->fps(); }
        qDebug() << "do frame in/out" << value;
        if (doFrameIn) { scene->setFrameIn(true, value); }
        else if (doFrameOut) { scene->setFrameOut(true, value); }
        appendHistory(input);
        accept();
        return;
    }
}

bool CommandPalette::showToolTip(const QString &input)
{
    return input.startsWith("cmd:");
}

void CommandPalette::updateToolTip(const QString &input)
{
    if (input.isEmpty()) { return; }
    mLabel->setText(input);
    if (mLabel->isHidden()) {
        mLabel->show();
        adjustSize();
    }
}

void CommandPalette::getHistory(const bool append)
{
    if (append && !isCmd(mUserInput->text())) { return; }
    const auto history = eSettings::instance().fCommandHistory;
    if (history.count() < 1) { return; }
    if (mHistoryCounter < 0 ||
        mHistoryCounter >= history.count()) { mHistoryCounter = history.count() - 1; }
    const QString cmd = history.at(mHistoryCounter);
    if (append && !isCmd(cmd)) { return; }
    mUserInput->setText(append ? QString("%1 && %2").arg(mUserInput->text(), cmd) : cmd);
    mUserInput->setCursorPosition(mUserInput->text().length());
    mHistoryCounter--;
}

void CommandPalette::appendHistory(const QString &input)
{
    if (input.isEmpty()) { return; }
    eSettings::sInstance->fCommandHistory.append(input);
}

QAction *CommandPalette::getAction(const int index)
{
    const auto actions = eSettings::instance().fCommandPalette;
    if (actions.count() < 1 ||
        index < 0 ||
        index >= actions.count()) { return nullptr; }
    return actions.at(index);
}

bool CommandPalette::isIntOrDouble(const QString &input)
{
    if (input.isEmpty()) { return false; }

    static QRegularExpression isInt("^[-]?[0-9]*$");
    static QRegularExpression isDouble("^[-]?[0-9]*[.][0-9]*?$");

    return (isInt.match(input).hasMatch() || isDouble.match(input).hasMatch());
}

bool CommandPalette::isActionCmd(const QString &input)
{
    return (input.startsWith("cmd:") && input != "cmd:");
}

bool CommandPalette::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == mUserInput) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if (keyEvent->key() == Qt::Key_Up) {
                getHistory(keyEvent->modifiers() & Qt::ShiftModifier);
                return true;
            } else if (keyEvent->key() == Qt::Key_Down) {
                if (mSuggestions->isVisible()) { mSuggestions->setFocus(); }
                return true;
            }
        }
        return false;
    }
#ifdef Q_OS_MAC
    else if (obj == mSuggestions) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
            if (keyEvent->key() == Qt::Key_Return) {
                if (mSuggestions->currentItem()) {
                    emit mSuggestions->itemActivated(mSuggestions->currentItem());
                    return true;
                }
            }
        }
    }
#endif
    return QDialog::eventFilter(obj, e);
}
