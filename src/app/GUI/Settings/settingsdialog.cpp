#include "settingsdialog.h"
#include "Private/esettings.h"
#include "hardwareinfo.h"
#include "exceptions.h"
#include "GUI/global.h"

#include "appsupport.h"

#include "generalsettingswidget.h"
#include "widgets/performancesettingswidget.h"
#include "widgets/canvassettingswidget.h"
#include "timelinesettingswidget.h"
#include "pluginssettingswidget.h"
#include "widgets/presetsettingswidget.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QStatusBar>

SettingsDialog::SettingsDialog(QWidget * const parent)
    : QDialog(parent)
{

    setWindowTitle(tr("Preferences"));

    const auto mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    mTabWidget = new QTabWidget(this);

    const auto general = new GeneralSettingsWidget(this);
    addSettingsWidget(general, tr("General"));

    const auto performance = new PerformanceSettingsWidget(this);
    addSettingsWidget(performance,tr("Hardware"));

    const auto canvas = new CanvasSettingsWidget(this);
    addSettingsWidget(canvas, tr("Canvas"));

    const auto timeline = new TimelineSettingsWidget(this);
    addSettingsWidget(timeline, tr("Timeline"));

    const auto plugins = new PluginsSettingsWidget(this);
    addSettingsWidget(plugins, tr("Shaders"));

    const auto presets = new PresetSettingsWidget(this);
    addSettingsWidget(presets, tr("Presets"));

    mainLayout->addWidget(mTabWidget);

    const auto buttonsLayout = new QHBoxLayout;

    const auto restoreButton = new QPushButton(QIcon::fromTheme("loop_back"),
                                               tr("Restore Defaults"),
                                               this);
    restoreButton->setFocusPolicy(Qt::NoFocus);

    const auto cancelButton = new QPushButton(QIcon::fromTheme("dialog-cancel"),
                                              tr("Close"),
                                              this);
    cancelButton->setFocusPolicy(Qt::NoFocus);

    const auto applyButton = new QPushButton(QIcon::fromTheme("disk_drive"),
                                             tr("Save"),
                                             this);
    applyButton->setFocusPolicy(Qt::NoFocus);

    buttonsLayout->addWidget(restoreButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(applyButton);
    buttonsLayout->addWidget(cancelButton);

    eSizesUI::widget.add(restoreButton, [restoreButton,
                                         applyButton,
                                         cancelButton](const int size) {
        Q_UNUSED(size)
        restoreButton->setFixedHeight(eSizesUI::button);
        cancelButton->setFixedHeight(eSizesUI::button);
        applyButton->setFixedHeight(eSizesUI::button);
    });

    mainLayout->addLayout(buttonsLayout);
    const auto statusBar = new QStatusBar(this);
    statusBar->setSizeGripEnabled(false);
    mainLayout->addWidget(statusBar);

    connect(restoreButton, &QPushButton::released, this, [this]() {
        eSettings::sInstance->loadDefaults();
        updateSettings(true /* restore */);
    });

    connect(cancelButton, &QPushButton::released, this, &QDialog::close);

    connect(applyButton, &QPushButton::released,
            this, [this, statusBar]() {
        for (const auto widget : qAsConst(mSettingWidgets)) {
            widget->applySettings();
        }
        emit eSettings::sInstance->settingsChanged();
        try {
            eSettings& sett = *eSettings::sInstance;
            sett.saveToFile();
        } catch(const std::exception& e) {
            gPrintExceptionCritical(e);
        }
        statusBar->showMessage(tr("Settings saved, you might have to restart"), 1500);
    });

    updateSettings();
    restoreGeometry(AppSupport::getSettings("ui",
                                            "SettingsDialogGeometry").toByteArray());
}

SettingsDialog::~SettingsDialog()
{
    AppSupport::setSettings("ui",
                            "SettingsDialogGeometry",
                            saveGeometry());
}

void SettingsDialog::addSettingsWidget(SettingsWidget * const widget,
                                       const QString &name)
{
    mTabWidget->addTab(widget, name);
    mSettingWidgets << widget;
}

void SettingsDialog::updateSettings(bool restore)
{
    for (const auto widget : qAsConst(mSettingWidgets)) {
        widget->updateSettings(restore);
    }
}
