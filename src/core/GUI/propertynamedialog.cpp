#include "propertynamedialog.h"

#include <QVBoxLayout>
#include <QPushButton>

#include "Properties/property.h"
#include "Boxes/boundingbox.h"
#include "Private/scene.h"

PropertyNameDialog::PropertyNameDialog(
        const QString &initialValue,
        QWidget* const parent) : QDialog(parent) {
    setWindowTitle("Rename " + initialValue);

    const auto mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    mName = new QLineEdit(initialValue, this);
    mainLayout->addWidget(mName);
    connect(mName, &QLineEdit::textChanged,
            this, &PropertyNameDialog::validate);

    mErrorLabel = new QLabel(this);
    mErrorLabel->setObjectName("errorLabel");
    mainLayout->addWidget(mErrorLabel);

    const auto buttonsLayout = new QHBoxLayout;
    mOkButton = new QPushButton("Ok", this);
    const auto cancelButton = new QPushButton("Cancel", this);

    buttonsLayout->addWidget(mOkButton);
    buttonsLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonsLayout);

    connect(mOkButton, &QPushButton::released,
            this, &PropertyNameDialog::accept);
    connect(cancelButton, &QPushButton::released,
            this, &PropertyNameDialog::reject);

    validate();
    mName->setSelection(0, mName->text().length());
}

bool PropertyNameDialog::sGetPropertyName(QString& name,
                                          QWidget* const parent) {
    const auto dialog = new PropertyNameDialog(name, parent);
    const auto result = dialog->exec() == QDialog::Accepted;
    if(result) name = dialog->name();
    delete dialog;
    return result;
}

void PropertyNameDialog::sRenameBox(BoundingBox * const box,
                                    QWidget * const parent) {
    const auto dialog = new PropertyNameDialog(box->prp_getName(),
                                               parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    connect(dialog, &QDialog::accepted, box, [box, dialog]() {
        box->rename(dialog->name());
    });
}

void PropertyNameDialog::sRenameProperty(Property * const prop,
                                         QWidget * const parent) {
    const auto dialog = new PropertyNameDialog(prop->prp_getName(),
                                               parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    connect(dialog, &QDialog::accepted, prop, [prop, dialog]() {
        prop->prp_setNameAction(dialog->name());
    });
}

bool PropertyNameDialog::validate() {
    QString error;
    const bool valid = Property::prp_sValidateName(mName->text(), &error);
    mErrorLabel->setText(error);
    mOkButton->setEnabled(valid);
    return valid;
}
