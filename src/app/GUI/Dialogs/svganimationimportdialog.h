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
*/

#ifndef SVGANIMATIONIMPORTDIALOG_H
#define SVGANIMATIONIMPORTDIALOG_H

#include <QDialog>
#include <QSizeF>

#include "svganimationimporter.h"

class QComboBox;

class SVGAnimationImportDialog : public QDialog {
public:
    struct SceneInfo {
        QSize size;
        int firstFrame;
        int lastFrame;
        qreal fps;
    };

    enum class ScaleMode {
        original,
        fitWidth,
        fitHeight,
        stretch
    };

    enum class StructureMode {
        namedGroup,
        directObjects
    };

    using SceneDurationMode = ImportSVGAnimation::SceneDurationMode;

    SVGAnimationImportDialog(const QSizeF& svgSize, const SceneInfo& scene,
                             const ImportSVGAnimation::Analysis& analysis,
                             QWidget* const parent);

    ScaleMode scaleMode() const;
    StructureMode structureMode() const;
    SceneDurationMode sceneDurationMode() const;

    static bool sExec(const QString& path, const SceneInfo& scene,
                      QSizeF& svgSize, ScaleMode& scaleMode,
                      StructureMode& structureMode,
                      SceneDurationMode& durationMode,
                      QWidget* const parent);

private:
    static QSizeF readSVGSize(const QString& path);

    QComboBox* mScaleMode;
    QComboBox* mStructureMode;
    QComboBox* mSceneDurationMode;
};

#endif // SVGANIMATIONIMPORTDIALOG_H
