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

#ifndef EXPRESSION_PRESETS_H
#define EXPRESSION_PRESETS_H

#include "core_global.h"

#include <QObject>
#include <QList>
#include <QStringList>

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT ExpressionPresets : public QObject
        {
            Q_OBJECT
        public:
            struct Expr
            {
                bool core = false;
                bool valid = false;
                bool enabled = false;
                double version = 0.0; // preset version
                QString id; // preset unique ID
                QString path; // preset absolute path
                QString title; // preset title
                QString author; // preset author (optional)
                QString description; // preset description (optional)
                QString url; // preset url (optional)
                QString license; // preset license (optional)
                QStringList categories; // preset categories (optional)
                QStringList highlighters; // editor highlighters (optional)
                QString definitions; // editor definitions
                QString bindings; // editor bindings
                QString script; // editor script
            };

            explicit ExpressionPresets(QObject *parent = nullptr);

            const QList<Expr> getAll();
            const QList<Expr> getDefinitions();

            const QList<Expr> getCore(const QString &category = QString());
            const QList<Expr> getCoreDefinitions();

            const QList<Expr> getUser(const QString &category = QString(),
                                      const bool &defs = false);
            const QList<Expr> getUserDefinitions();

            const Expr readExpr(const QString &path);

            bool editExpr(const QString &id,
                          const QString &title = QString(),
                          const QString &definitions = QString(),
                          const QString &bindings = QString(),
                          const QString &script = QString());

            void loadExpr(const QString &path);
            void loadExpr(const QStringList &paths);

            bool saveExpr(const int &index,
                          const QString &path);
            bool saveExpr(const QString &id,
                          const QString &path);
            bool saveExpr(const Expr &expr,
                          const QString &path);

            bool hasExpr(const int &index);
            bool hasExpr(const QString &id);

            const Expr getExpr(const int &index);
            const Expr getExpr(const QString &id);

            int getExprIndex(const QString &id);

            bool addExpr(const Expr &expr);

            bool remExpr(const int &index);
            bool remExpr(const QString &id);

            void setExprEnabled(const int &index,
                                const bool &enabled);
            void setExprEnabled(const QString &id,
                                const bool &enabled);

            bool isValidExprFile(const QString &path);

        private:
            QList<Expr> mExpr;
            QStringList mDisabled;

            void firstRun();
            void scanAll(const bool &clear = false);
        };
    }
}

#endif // EXPRESSION_PRESETS_H
