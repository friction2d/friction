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

#include "svganimationimportdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFile>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

qreal readDimension(const QString& value) {
    static const QRegularExpression number(
                QStringLiteral("^\\s*([+-]?(?:\\d*\\.)?\\d+(?:[eE][+-]?\\d+)?)"));
    const auto match = number.match(value);
    if (!match.hasMatch() || value.trimmed().endsWith('%')) { return 0; }
    bool ok = false;
    const qreal result = match.captured(1).toDouble(&ok);
    return ok && result > 0 ? result : 0;
}

QString sizeText(const QSizeF& size) {
    if (size.width() <= 0 || size.height() <= 0) {
        return SVGAnimationImportDialog::tr("Unknown");
    }
    return SVGAnimationImportDialog::tr("%1 x %2")
            .arg(size.width(), 0, 'g', 8)
            .arg(size.height(), 0, 'g', 8);
}

QString durationText(const qreal seconds) {
    return SVGAnimationImportDialog::tr("%1 seconds").arg(seconds, 0, 'g', 8);
}

QTableWidgetItem* tableItem(const QString& text) {
    return new QTableWidgetItem(text);
}

}

SVGAnimationImportDialog::SVGAnimationImportDialog(
        const QSizeF& svgSize, const SceneInfo& scene,
        const ImportSVGAnimation::Analysis& analysis, QWidget* const parent) :
    QDialog(parent) {
    setWindowTitle(tr("Import SVG Animation"));

    const auto mainLayout = new QVBoxLayout(this);
    const int sceneFrameCount = scene.lastFrame - scene.firstFrame + 1;
    const qreal sceneDuration = scene.fps > 0 ?
                (scene.lastFrame - scene.firstFrame) / scene.fps : 0;
    const int svgFrames = qRound(analysis.duration * scene.fps);

    const auto comparison = new QTableWidget(3, 3, this);
    comparison->setHorizontalHeaderLabels(
                {tr("Property"), tr("SVG"), tr("Active scene")});
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
    comparison->setItem(0, 1, tableItem(sizeText(svgSize)));
    comparison->setItem(0, 2, tableItem(sizeText(scene.size)));
    comparison->setItem(1, 0, tableItem(tr("Duration")));
    comparison->setItem(1, 1, tableItem(durationText(analysis.duration)));
    comparison->setItem(1, 2, tableItem(durationText(sceneDuration)));
    comparison->setItem(2, 0, tableItem(tr("Frames")));
    comparison->setItem(2, 1, tableItem(tr("%1 at scene FPS").arg(svgFrames)));
    comparison->setItem(2, 2, tableItem(tr("%1 to %2 (%3 total)")
                                        .arg(scene.firstFrame)
                                        .arg(scene.lastFrame)
                                        .arg(sceneFrameCount)));
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
                    tr("Unsupported animations will be skipped: %1")
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

    const bool validSize = svgSize.width() > 0 && svgSize.height() > 0 &&
            scene.size.width() > 0 && scene.size.height() > 0;
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
    mStructureMode->setCurrentIndex(
                static_cast<int>(StructureMode::directObjects));
    optionsLayout->addRow(tr("Import structure"), mStructureMode);

    mSceneDurationMode = new QComboBox(this);
    mSceneDurationMode->addItem(tr("Don't modify"));
    mSceneDurationMode->addItem(tr("Extend if needed"));
    mSceneDurationMode->addItem(tr("Fit to imported SVG"));
    mSceneDurationMode->setCurrentIndex(
                analysis.duration > sceneDuration ?
                    static_cast<int>(SceneDurationMode::extendIfNeeded) :
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

SVGAnimationImportDialog::ScaleMode SVGAnimationImportDialog::scaleMode() const {
    return static_cast<ScaleMode>(mScaleMode->currentIndex());
}

SVGAnimationImportDialog::StructureMode
SVGAnimationImportDialog::structureMode() const {
    return static_cast<StructureMode>(mStructureMode->currentIndex());
}

SVGAnimationImportDialog::SceneDurationMode
SVGAnimationImportDialog::sceneDurationMode() const {
    return static_cast<SceneDurationMode>(mSceneDurationMode->currentIndex());
}

bool SVGAnimationImportDialog::sExec(
        const QString& path, const SceneInfo& scene, QSizeF& svgSize,
        ScaleMode& scaleMode, StructureMode& structureMode,
        SceneDurationMode& durationMode, QWidget* const parent) {
    svgSize = readSVGSize(path);
    const auto analysis = ImportSVGAnimation::analyzeSVGFile(path);
    SVGAnimationImportDialog dialog(svgSize, scene, analysis, parent);
    if (dialog.exec() != QDialog::Accepted) { return false; }
    scaleMode = dialog.scaleMode();
    structureMode = dialog.structureMode();
    durationMode = dialog.sceneDurationMode();
    return true;
}

QSizeF SVGAnimationImportDialog::readSVGSize(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { return {}; }

    QDomDocument document;
    if (!document.setContent(&file)) { return {}; }
    const QDomElement svg = document.documentElement();
    if (svg.tagName().compare(QStringLiteral("svg"),
                              Qt::CaseInsensitive) != 0) {
        return {};
    }

    const QStringList viewBox = svg.attribute(QStringLiteral("viewBox"))
            .split(QRegularExpression(QStringLiteral("[,\\s]+")),
                   Qt::SkipEmptyParts);
    if (viewBox.size() == 4) {
        bool widthOk = false;
        bool heightOk = false;
        const qreal width = viewBox.at(2).toDouble(&widthOk);
        const qreal height = viewBox.at(3).toDouble(&heightOk);
        if (widthOk && heightOk && width > 0 && height > 0) {
            return QSizeF(width, height);
        }
    }

    return QSizeF(readDimension(svg.attribute(QStringLiteral("width"))),
                  readDimension(svg.attribute(QStringLiteral("height"))));
}
