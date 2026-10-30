// SPDX-License-Identifier: GPL-3.0-only

#ifndef FRICTION_CORE_PLUGINS_INTERFACE_H
#define FRICTION_CORE_PLUGINS_INTERFACE_H

#include "smartPointers/selfref.h"

#include <QObject>
#include <QList>
#include <QAction>

class Document;
class Canvas;
class BoundingBox;

enum class PreviewState;

class FrictionCorePluginInterface
{
public:
    virtual ~FrictionCorePluginInterface() = default;

    virtual void init() {};

    virtual QList<QAction*> createMenuActions(QObject* parent)
    {
        Q_UNUSED(parent);
        return {};
    }

    virtual QList<QAction*> createToolbarActions(QObject* parent)
    {
        Q_UNUSED(parent);
        return {};
    }

    virtual void triggerAction(Document& doc,
                               Canvas* const scene,
                               const QAction *act)
    {
        Q_UNUSED(doc);
        Q_UNUSED(scene);
        Q_UNUSED(act);
    }

    virtual qsptr<BoundingBox> importFile(Canvas* const scene,
                                          const QString &path)
    {
        Q_UNUSED(scene);
        Q_UNUSED(path);
        return nullptr;
    }

    virtual void renderStateChanged(PreviewState state)
    {
        Q_UNUSED(state);
    }

    virtual void renderProgress(int frame,
                                int total)
    {
        Q_UNUSED(frame);
        Q_UNUSED(total);
    }

    virtual void showNotification(const QString& title,
                                  const QString& message)
    {
        Q_UNUSED(title);
        Q_UNUSED(message);
    }
};

#define FrictionCorePluginInterface_iid "graphics.friction.CorePluginInterface/1.0"
Q_DECLARE_INTERFACE(FrictionCorePluginInterface, FrictionCorePluginInterface_iid)

#endif // FRICTION_CORE_PLUGINS_INTERFACE_H
