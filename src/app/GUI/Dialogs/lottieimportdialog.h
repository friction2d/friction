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

#ifndef LOTTIEIMPORTDIALOG_H
#define LOTTIEIMPORTDIALOG_H

#include "lottie/lottieimporter.h"

#include <QDialog>
#include <QSize>

class QComboBox;

class LottieImportDialog : public QDialog {
public:
    struct SceneInfo {
        QSize size;
        int firstFrame;
        int lastFrame;
        qreal fps;
    };

    enum class ScaleMode {
        original,
        scaleScene,
        fitWidth,
        fitHeight,
        stretch
    };

    enum class StructureMode {
        namedGroup,
        directObjects
    };

    using SceneDurationMode = ImportLottie::SceneDurationMode;

    LottieImportDialog(const ImportLottie::Analysis& analysis,
                       const SceneInfo& scene,
                       QWidget* const parent);

    ScaleMode scaleMode() const;
    StructureMode structureMode() const;
    SceneDurationMode sceneDurationMode() const;

    static bool sExec(const QString& path, const SceneInfo& scene,
                      ImportLottie::Analysis& analysis,
                      ScaleMode& scaleMode,
                      StructureMode& structureMode,
                      SceneDurationMode& durationMode,
                      QWidget* const parent);

private:
    QComboBox* mScaleMode;
    QComboBox* mStructureMode;
    QComboBox* mSceneDurationMode;
};

#endif // LOTTIEIMPORTDIALOG_H
