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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "core_global.h"

#include <exception>
#include <stdexcept>
#include <csignal>
#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcFrictionCore)

#define ERROUT(msg) qCDebug(lcFrictionCore) << msg << __LINE__
#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#ifdef QT_DEBUG
    #if defined (Q_OS_WIN)
        #define NOMINMAX
        #include <windows.h>
        #undef NOMINMAX
        #define BREAKPOINT DebugBreak();
    #else
        #define BREAKPOINT
    #endif
#else
    #define BREAKPOINT
#endif

#define PrettyRuntimeThrow(msg) \
    std::throw_with_nested( \
        std::runtime_error(std::string("") + msg) \
    )

#define NoBreakRuntimeThrow(msg) \
{ \
    std::throw_with_nested( \
        std::runtime_error( \
            std::to_string(__LINE__) + "  :  " + \
            FILENAME + "  :  " + __func__ + "()\n  " + msg \
        ) \
    ); \
}

#define RuntimeThrow(msg) \
{ \
    BREAKPOINT \
    NoBreakRuntimeThrow(msg) \
}

#define CheckInvalidLocation(vLoc, name) \
    if(vLoc < 0) { \
        checkGlErrors; \
        RuntimeThrow("Invalid " name " location."); \
    }

CORE_EXPORT
std::string operator+(const std::string& c, const QString& k);
CORE_EXPORT
std::string operator<<(const std::string& c, const QString& k);
CORE_EXPORT
std::string operator>>(const QString& k, const std::string& c);

CORE_EXPORT
QDebug operator<<(QDebug out, const std::string& str);

CORE_EXPORT
extern QString gAllTextFromException(const std::exception& e,
                                     QString allText = "",
                                     const uint level = 0);
CORE_EXPORT
extern void gPrintExceptionCritical(const std::exception& e);
CORE_EXPORT
extern void gPrintExceptionFatal(const std::exception& e);
CORE_EXPORT
extern void gPrintException(const bool fatal, const QString& allText);
CORE_EXPORT
extern void gPrintException(const QString& allText);
CORE_EXPORT
extern void gPrintExceptionCritical(const std::exception_ptr& eptr);
CORE_EXPORT
extern void gPrintExceptionFatal(const std::exception_ptr& eptr);

#endif // EXCEPTIONS_H
