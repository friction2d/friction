/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "expressionpresets.h"
#include "appsupport.h"

#include <QFile>
#include <QSettings>
#include <QDebug>

using namespace Friction::Core;

ExpressionPresets::ExpressionPresets(QObject *parent)
    : QObject{parent}
{
    scanAll();
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getAll()
{
    return mExpr;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getDefinitions()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : getCoreDefinitions()) { list << expr; }
    for (const auto &expr : getUserDefinitions()) { list << expr; }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getCore(const QString &category)
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : mExpr) {
        if (!expr.valid || !expr.core || !expr.enabled) { continue; }
        if (!category.isEmpty()) {
            if (!expr.categories.contains(category)) { continue; }
        }
        list.append(expr);
    }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getCoreDefinitions()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : getCore()) {
        if (expr.bindings.isEmpty() &&
            !expr.definitions.isEmpty() &&
            expr.script.isEmpty()) { list.append(expr); }
    }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getUser(const QString &category,
                                                                const bool &defs)
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : mExpr) {
        if (!expr.valid ||
            expr.core ||
            !expr.enabled ||
            expr.path.startsWith(":")) { continue; }
        if (!defs) {
            if (expr.bindings.isEmpty() &&
                !expr.definitions.isEmpty() &&
                expr.script.isEmpty()) { continue; }
        }
        if (!category.isEmpty()) {
            if (!expr.categories.contains(category)) { continue; }
        }
        list.append(expr);
    }
    return list;
}

const QList<ExpressionPresets::Expr> ExpressionPresets::getUserDefinitions()
{
    QList<ExpressionPresets::Expr> list;
    for (const auto &expr : getUser(QString(), true)) { list.append(expr); }
    return list;
}

const ExpressionPresets::Expr
ExpressionPresets::readExpr(const QString &path)
{
    ExpressionPresets::Expr expr;
    expr.valid = false;

    if (!QFile::exists(path)) { return expr; }

    QSettings fexpr(path, QSettings::IniFormat);

    expr.core = path.startsWith(":");
    expr.valid = true;
    expr.version = fexpr.value("version").toDouble();
    expr.id = fexpr.value("id").toString();
    expr.enabled = !mDisabled.contains(expr.id);
    expr.path = path;
    expr.title = fexpr.value("title").toString();
    expr.author = fexpr.value("author").toString();
    expr.description = fexpr.value("description").toString();
    expr.url = fexpr.value("url").toString();
    expr.license = fexpr.value("license").toString();
    expr.categories = fexpr.value("categories").toStringList();
    expr.highlighters = fexpr.value("highlighters").toStringList();
    expr.definitions = fexpr.value("definitions").toString();
    expr.bindings = fexpr.value("bindings").toString();
    expr.script = fexpr.value("script").toString();

    return expr;
}

bool ExpressionPresets::editExpr(const QString &id,
                                 const QString &title,
                                 const QString &definitions,
                                 const QString &bindings,
                                 const QString &script)
{
    if (!hasExpr(id)) { return false; }

    const auto expr = getExpr(id);
    if (!expr.valid || expr.core) { return false; }

    const int index = getExprIndex(id);
    if (index < 0) { return false; }

    bool modified = false;

    if (!title.trimmed().isEmpty() &&
        title != expr.title) {
        mExpr[index].title = title;
        modified = true;
    }
    if (!definitions.trimmed().isEmpty() &&
        definitions != expr.definitions) {
        mExpr[index].definitions = definitions;
        modified = true;
    }
    if (!bindings.trimmed().isEmpty() &&
        bindings != expr.bindings) {
        mExpr[index].bindings = bindings;
        modified = true;
    }
    if (!script.trimmed().isEmpty() &&
        script != expr.script) {
        mExpr[index].script = script;
        modified = true;
    }

    if (!modified) { return true; }
    return saveExpr(index, expr.path);
}

void ExpressionPresets::loadExpr(const QString &path)
{
    if (!QFile::exists(path)) { return; }
    qDebug() << "Load expression" << path;
    if (!isValidExprFile(path)) {
        qDebug() << "Bad expression" << path;
        return;
    }

    const auto expr = readExpr(path);

    if (!hasExpr(expr.id)) {
        qDebug() << "Added expression" << expr.title << expr.id;
        mExpr << expr;
    }
}

void ExpressionPresets::loadExpr(const QStringList &paths)
{
    for (const auto &path : paths) { loadExpr(path); }
}

bool ExpressionPresets::saveExpr(const int &index,
                                 const QString &path)
{
    if (!hasExpr(index) || path.isEmpty()) { return false; }
    return saveExpr(mExpr.at(index), path);
}

bool ExpressionPresets::saveExpr(const QString &id,
                                 const QString &path)
{
    return saveExpr(getExpr(id), path);
}

bool ExpressionPresets::saveExpr(const Expr &expr,
                                 const QString &path)
{
    if (!expr.valid || path.isEmpty()) { return false; }

    QSettings fexpr(path, QSettings::IniFormat);
    fexpr.setValue("version", expr.version);
    fexpr.setValue("id", expr.id);
    fexpr.setValue("title", expr.title);
    fexpr.setValue("author", expr.author);
    fexpr.setValue("description", expr.description);
    fexpr.setValue("url", expr.url);
    fexpr.setValue("license", expr.license);
    fexpr.setValue("categories", expr.categories);
    fexpr.setValue("highlighters", expr.highlighters);
    fexpr.setValue("definitions", expr.definitions);
    fexpr.setValue("bindings", expr.bindings);
    fexpr.setValue("script", expr.script);
    fexpr.sync();

    return isValidExprFile(path);
}

bool ExpressionPresets::hasExpr(const int &index)
{
    if (mExpr.count() > 0 &&
        index >= 0 &&
        index < mExpr.count()) {
        return mExpr.at(index).valid;
    }
    return false;
}

bool ExpressionPresets::hasExpr(const QString &id)
{
    if (id.isEmpty()) { return false; }
    for (const auto &expr : mExpr) {
        if (!expr.valid) { continue; }
        if (expr.id == id) { return true; }
    }
    return false;
}

const ExpressionPresets::Expr
ExpressionPresets::getExpr(const int &index)
{
    if (hasExpr(index)) { return mExpr.at(index); }
    Expr expr;
    expr.valid = false;
    return expr;
}

const ExpressionPresets::Expr ExpressionPresets::getExpr(const QString &id)
{
    Expr noExpr;
    noExpr.valid = false;
    if (id.isEmpty()) { return noExpr; }

    for (const auto &expr : mExpr) {
        if (!expr.valid) { continue; }
        if (expr.id == id) { return expr; }
    }

    return noExpr;
}

int ExpressionPresets::getExprIndex(const QString &id)
{
    if (id.isEmpty()) { return -1; }
    for (int i = 0; i < mExpr.count(); i++) {
        if (mExpr.at(i).id == id) { return i; }
    }
    return -1;
}

bool ExpressionPresets::addExpr(const Expr &expr)
{
    if (!expr.valid || hasExpr(expr.id)) { return false; }
    mExpr << expr;
    return true;
}

bool ExpressionPresets::remExpr(const int &index)
{
    if (!hasExpr(index)) { return false; }
    const auto expr = mExpr.at(index);
    if (expr.core) { return false; }

    const auto path = expr.path;
    mExpr.removeAt(index);

    QFile file(path);
    return file.remove();
}

bool ExpressionPresets::remExpr(const QString &id)
{
    const int index = getExprIndex(id);
    if (index < 0) { return false; }
    return remExpr(index);
}

void ExpressionPresets::setExprEnabled(const int &index,
                                       const bool &enabled)
{
    if (!hasExpr(index)) { return; }
    mExpr[index].enabled = enabled;

    const QString id = mExpr.at(index).id;
    const auto disabled = AppSupport::getSettings("settings",
                                                  "ExpressionsDisabled").toStringList();
    if (enabled && disabled.contains(id)) {
        QStringList list = disabled;
        list.removeAll(id);
        AppSupport::setSettings("settings",
                                "ExpressionsDisabled",
                                list);
        mDisabled = list;
    } else if (!enabled && !disabled.contains(id)) {
        AppSupport::setSettings("settings",
                                "ExpressionsDisabled",
                                id,
                                true);
        mDisabled << id;
    }
}

void ExpressionPresets::setExprEnabled(const QString &id,
                                       const bool &enabled)
{
    const int index = getExprIndex(id);
    if (index < 0) { return; }
    setExprEnabled(index, enabled);
}

bool ExpressionPresets::isValidExprFile(const QString &path)
{
    if (!QFile::exists(path)) { return false; }

    QSettings fexpr(path, QSettings::IniFormat);

    const double version = fexpr.value("version").toDouble();
    const bool hasId = !fexpr.value("id").toString().isEmpty();
    const bool hasBindings = !fexpr.value("bindings").toString().isEmpty();
    const bool hasDefinitions = !fexpr.value("definitions").toString().isEmpty();
    const bool hasScript = !fexpr.value("script").toString().isEmpty();

    if (version >= 0.1 &&
        hasId &&
        (hasBindings || hasDefinitions || hasScript)) { return true; }

    return false;
}

void ExpressionPresets::firstRun()
{
    const QString path = AppSupport::getAppUserExPresetsPath();
    const bool firstrun = AppSupport::getSettings("settings",
                                                  "firstRunExprPresets",
                                                  true).toBool();
    if (!firstrun || path.isEmpty()) { return; }

    QStringList presets;
    presets << "copyX.fexpr";
    presets << "copyY.fexpr";
    presets << "frameRemapLoop.fexpr";
    presets << "frameRemapLoopBounce.fexpr";
    presets << "noise.fexpr";
    presets << "orbitX.fexpr";
    presets << "orbitY.fexpr";
    presets << "oscillation.fexpr";
    presets << "rotation.fexpr";
    presets << "time.fexpr";
    presets << "trackObject.fexpr";
    presets << "wave.fexpr";
    presets << "wiggle.fexpr";

    for (const auto &preset : presets) {
        const auto expr = readExpr(QString(":/expressions/%1").arg(preset));
        if (!expr.valid) { continue; }
        QFile file(QString("%1/%2.fexpr").arg(path, expr.id));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QFile res(expr.path);
            if (res.open(QIODevice::ReadOnly | QIODevice::Text)) {
                file.write(res.readAll());
                res.close();
            } else {
                qWarning() << "Failed to find expression preset" << res.fileName();
            }
            file.close();
        } else {
            qWarning() << "Failed to install expression preset" << file.fileName();
        }
    }

    AppSupport::setSettings("settings",
                            "firstRunExprPresets",
                            false);
}

void ExpressionPresets::scanAll(const bool &clear)
{
    if (clear) { mExpr.clear(); }

    firstRun();

    mDisabled = AppSupport::getSettings("settings",
                                        "ExpressionsDisabled").toStringList();

    QStringList expressions;
    expressions << ":/expressions/clamp.fexpr";
    expressions << ":/expressions/lerp.fexpr";
    expressions << ":/expressions/easeInBack.fexpr";
    expressions << ":/expressions/easeInBounce.fexpr";
    expressions << ":/expressions/easeInCirc.fexpr";
    expressions << ":/expressions/easeInCubic.fexpr";
    expressions << ":/expressions/easeInElastic.fexpr";
    expressions << ":/expressions/easeInExpo.fexpr";
    expressions << ":/expressions/easeInOutBack.fexpr";
    expressions << ":/expressions/easeInOutBounce.fexpr";
    expressions << ":/expressions/easeInOutCirc.fexpr";
    expressions << ":/expressions/easeInOutCubic.fexpr";
    expressions << ":/expressions/easeInOutElastic.fexpr";
    expressions << ":/expressions/easeInOutExpo.fexpr";
    expressions << ":/expressions/easeInOutQuad.fexpr";
    expressions << ":/expressions/easeInOutQuart.fexpr";
    expressions << ":/expressions/easeInOutQuint.fexpr";
    expressions << ":/expressions/easeInOutSine.fexpr";
    expressions << ":/expressions/easeInQuad.fexpr";
    expressions << ":/expressions/easeInQuart.fexpr";
    expressions << ":/expressions/easeInQuint.fexpr";
    expressions << ":/expressions/easeInSine.fexpr";
    expressions << ":/expressions/easeOutBack.fexpr";
    expressions << ":/expressions/easeOutBounce.fexpr";
    expressions << ":/expressions/easeOutCirc.fexpr";
    expressions << ":/expressions/easeOutCubic.fexpr";
    expressions << ":/expressions/easeOutElastic.fexpr";
    expressions << ":/expressions/easeOutExpo.fexpr";
    expressions << ":/expressions/easeOutQuad.fexpr";
    expressions << ":/expressions/easeOutQuart.fexpr";
    expressions << ":/expressions/easeOutQuint.fexpr";
    expressions << ":/expressions/easeOutSine.fexpr";

    for (const auto &file : AppSupport::getFilesFromPath(AppSupport::getAppUserExPresetsPath(),
                                                         QStringList() << "*.fexpr")) {
        qDebug() << "Checking user expression" << file;
        if (isValidExprFile(file)) { expressions << file; }
    }

    loadExpr(expressions);
}
