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

#include "lottieimportdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QString sizeText(const QSize& size)
{
    if (!size.isValid() || size.isEmpty()) {
        return LottieImportDialog::tr("Unknown");
    }
    return LottieImportDialog::tr("%1 x %2")
            .arg(size.width())
            .arg(size.height());
}

QString durationText(const qreal seconds)
{
    return LottieImportDialog::tr("%1 seconds").arg(seconds, 0, 'g', 8);
}

QTableWidgetItem* tableItem(const QString& text)
{
    return new QTableWidgetItem(text);
}

}

LottieImportDialog::LottieImportDialog(
        const ImportLottie::Analysis& analysis,
        const SceneInfo& scene,
        QWidget* const parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Import Lottie Animation"));

    const auto mainLayout = new QVBoxLayout(this);
    const int sceneFrameCount = scene.lastFrame - scene.firstFrame + 1;
    const qreal sceneDuration = scene.fps > 0 ?
                (scene.lastFrame - scene.firstFrame) / scene.fps : 0;
    const int lottieFrames = analysis.lastFrame - analysis.firstFrame + 1;

    const auto comparison = new QTableWidget(4, 3, this);
    comparison->setHorizontalHeaderLabels(
                {tr("Property"), tr("Lottie"), tr("Active scene")});
    comparison->verticalHeader()->hide();
    comparison->horizontalHeader()->setSectionResizeMode(
                0, QHeaderView::ResizeToContents);
    comparison->horizontalHeader()->setSectionResizeMode(
                1, QHeaderView::Stretch);
    comparison->horizontalHeader()->setSectionResizeMode(
                2, QHeaderView::Stretch);
    comparison->setEditTriggers(QAbstractItemView::NoEditTriggers);
    comparison->setSelectionMode(QAbstractItemView::NoSelection);
    comparison->setFocusPolicy(Qt::NoFocus);
    comparison->setShowGrid(false);

    comparison->setItem(0, 0, tableItem(tr("Dimensions")));
    comparison->setItem(0, 1, tableItem(sizeText(analysis.size)));
    comparison->setItem(0, 2, tableItem(sizeText(scene.size)));
    comparison->setItem(1, 0, tableItem(tr("Duration")));
    comparison->setItem(1, 1, tableItem(durationText(analysis.duration)));
    comparison->setItem(1, 2, tableItem(durationText(sceneDuration)));
    comparison->setItem(2, 0, tableItem(tr("Frames")));
    comparison->setItem(2, 1, tableItem(tr("%1 to %2 (%3 total)")
                                        .arg(analysis.firstFrame)
                                        .arg(analysis.lastFrame)
                                        .arg(lottieFrames)));
    comparison->setItem(2, 2, tableItem(tr("%1 to %2 (%3 total)")
                                        .arg(scene.firstFrame)
                                        .arg(scene.lastFrame)
                                        .arg(sceneFrameCount)));
    comparison->setItem(3, 0, tableItem(tr("Layers")));
    comparison->setItem(3, 1, tableItem(tr("%1 of %2 supported")
                                        .arg(analysis.supportedLayers)
                                        .arg(analysis.totalLayers)));
    comparison->setItem(3, 2, tableItem(tr("%1 FPS").arg(scene.fps, 0, 'g', 8)));
    const int compactRowHeight = comparison->fontMetrics().height() + 4;
    comparison->verticalHeader()->setMinimumSectionSize(compactRowHeight);
    comparison->verticalHeader()->setDefaultSectionSize(compactRowHeight);
    for (int row = 0; row < comparison->rowCount(); ++row) {
        comparison->setRowHeight(row, compactRowHeight);
    }
    comparison->setMinimumWidth(460);
    comparison->setFixedHeight(comparison->horizontalHeader()->height() +
                               comparison->verticalHeader()->length() + 2);
    mainLayout->addWidget(comparison);

    if (!analysis.unsupported.isEmpty()) {
        const auto warning = new QLabel(
                    tr("Unsupported content will be skipped: %1")
                    .arg(analysis.unsupported.join(", ")), this);
        warning->setWordWrap(true);
        mainLayout->addWidget(warning);
    }

    const auto optionsLayout = new QFormLayout();
    mScaleMode = new QComboBox(this);
    mScaleMode->addItem(tr("Don't scale"));
    mScaleMode->addItem(tr("Scale proportionally (width fit)"));
    mScaleMode->addItem(tr("Scale proportionally (height fit)"));
    mScaleMode->addItem(tr("Deform (fit to scene)"));
    mScaleMode->addItem(tr("Scale Friction active scene"));
    const bool validSize = analysis.size.width() > 0 &&
            analysis.size.height() > 0 &&
            scene.size.width() > 0 &&
            scene.size.height() > 0;
    if (!validSize) {
        const auto model = qobject_cast<QStandardItemModel*>(mScaleMode->model());
        for (int i = 1; model && i < mScaleMode->count(); ++i) {
            model->item(i)->setEnabled(false);
        }
    }
    optionsLayout->addRow(tr("Scale mode"), mScaleMode);

    mStructureMode = new QComboBox(this);
    mStructureMode->addItem(tr("Grouped"));
    mStructureMode->addItem(tr("Ungrouped"));
    mStructureMode->setCurrentIndex(static_cast<int>(StructureMode::directObjects));
    optionsLayout->addRow(tr("Import structure"), mStructureMode);

    mSceneDurationMode = new QComboBox(this);
    mSceneDurationMode->addItem(tr("Don't modify"));
    mSceneDurationMode->addItem(tr("Extend if needed"));
    mSceneDurationMode->addItem(tr("Fit to imported Lottie"));
    mSceneDurationMode->setCurrentIndex(
                analysis.duration > 0 ?
                    static_cast<int>(SceneDurationMode::fitImportedLottie) :
                    static_cast<int>(SceneDurationMode::dontModify));
    if (analysis.duration <= 0) {
        const auto model = qobject_cast<QStandardItemModel*>(
                    mSceneDurationMode->model());
        for (int i = 1; model && i < mSceneDurationMode->count(); ++i) {
            model->item(i)->setEnabled(false);
        }
    }
    optionsLayout->addRow(tr("Scene duration"), mSceneDurationMode);
    mainLayout->addLayout(optionsLayout);

    const auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                              QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

LottieImportDialog::ScaleMode LottieImportDialog::scaleMode() const
{
    return static_cast<ScaleMode>(mScaleMode->currentIndex());
}

LottieImportDialog::StructureMode LottieImportDialog::structureMode() const
{
    return static_cast<StructureMode>(mStructureMode->currentIndex());
}

LottieImportDialog::SceneDurationMode LottieImportDialog::sceneDurationMode() const
{
    return static_cast<SceneDurationMode>(mSceneDurationMode->currentIndex());
}

bool LottieImportDialog::sExec(
        const QString& path,
        const SceneInfo& scene,
        ImportLottie::Analysis& analysis,
        ScaleMode& scaleMode,
        StructureMode& structureMode,
        SceneDurationMode& durationMode,
        QWidget* const parent)
{
    analysis = ImportLottie::analyzeFile(path);
    LottieImportDialog dialog(analysis, scene, parent);
    if (dialog.exec() != QDialog::Accepted) { return false; }
    scaleMode = dialog.scaleMode();
    structureMode = dialog.structureMode();
    durationMode = dialog.sceneDurationMode();
    return true;
}
