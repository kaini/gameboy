/********************************************************************************
** Form generated from reading UI file 'gameboy_ui.ui'
**
** Created by: Qt User Interface Compiler version 5.4.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GAMEBOY_UI_H
#define UI_GAMEBOY_UI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_gameboy_uiClass
{
public:
    QAction *actionExit;
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *loadRomPushButton;
    QPushButton *playPushButton;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_2;
    QFormLayout *formLayout;
    QLabel *label;
    QLineEdit *titleLineEdit;
    QLabel *label_2;
    QLineEdit *manufacturerLineEdit;
    QLabel *label_3;
    QLineEdit *licenseLineEdit;
    QLabel *label_4;
    QLineEdit *versionLineEdit;
    QLabel *label_5;
    QLineEdit *cartridgeLineEdit;
    QLabel *label_6;
    QLabel *label_7;
    QLineEdit *romLineEdit;
    QLineEdit *ramLineEdit;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *gbcCheckBox;
    QCheckBox *sgbCheckBox;
    QCheckBox *jpCheckBox;
    QCheckBox *logoOkCheckBox;
    QCheckBox *headerOkCheckBox;
    QCheckBox *globalOkCheckBox;
    QSpacerItem *verticalSpacer_2;
    QSpacerItem *verticalSpacer;
    QMenuBar *menuBar;
    QMenu *menuFile;

    void setupUi(QMainWindow *gameboy_uiClass)
    {
        if (gameboy_uiClass->objectName().isEmpty())
            gameboy_uiClass->setObjectName(QStringLiteral("gameboy_uiClass"));
        gameboy_uiClass->resize(324, 306);
        actionExit = new QAction(gameboy_uiClass);
        actionExit->setObjectName(QStringLiteral("actionExit"));
        centralWidget = new QWidget(gameboy_uiClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        loadRomPushButton = new QPushButton(centralWidget);
        loadRomPushButton->setObjectName(QStringLiteral("loadRomPushButton"));

        horizontalLayout->addWidget(loadRomPushButton);

        playPushButton = new QPushButton(centralWidget);
        playPushButton->setObjectName(QStringLiteral("playPushButton"));

        horizontalLayout->addWidget(playPushButton);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        formLayout = new QFormLayout();
        formLayout->setSpacing(6);
        formLayout->setObjectName(QStringLiteral("formLayout"));
        formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));
        label->setEnabled(false);

        formLayout->setWidget(0, QFormLayout::LabelRole, label);

        titleLineEdit = new QLineEdit(centralWidget);
        titleLineEdit->setObjectName(QStringLiteral("titleLineEdit"));
        titleLineEdit->setEnabled(false);
        titleLineEdit->setReadOnly(true);

        formLayout->setWidget(0, QFormLayout::FieldRole, titleLineEdit);

        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setEnabled(false);

        formLayout->setWidget(1, QFormLayout::LabelRole, label_2);

        manufacturerLineEdit = new QLineEdit(centralWidget);
        manufacturerLineEdit->setObjectName(QStringLiteral("manufacturerLineEdit"));
        manufacturerLineEdit->setEnabled(false);
        manufacturerLineEdit->setReadOnly(true);

        formLayout->setWidget(1, QFormLayout::FieldRole, manufacturerLineEdit);

        label_3 = new QLabel(centralWidget);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setEnabled(false);

        formLayout->setWidget(2, QFormLayout::LabelRole, label_3);

        licenseLineEdit = new QLineEdit(centralWidget);
        licenseLineEdit->setObjectName(QStringLiteral("licenseLineEdit"));
        licenseLineEdit->setEnabled(false);
        licenseLineEdit->setReadOnly(true);

        formLayout->setWidget(2, QFormLayout::FieldRole, licenseLineEdit);

        label_4 = new QLabel(centralWidget);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setEnabled(false);

        formLayout->setWidget(3, QFormLayout::LabelRole, label_4);

        versionLineEdit = new QLineEdit(centralWidget);
        versionLineEdit->setObjectName(QStringLiteral("versionLineEdit"));
        versionLineEdit->setEnabled(false);
        versionLineEdit->setReadOnly(true);

        formLayout->setWidget(3, QFormLayout::FieldRole, versionLineEdit);

        label_5 = new QLabel(centralWidget);
        label_5->setObjectName(QStringLiteral("label_5"));
        label_5->setEnabled(false);

        formLayout->setWidget(4, QFormLayout::LabelRole, label_5);

        cartridgeLineEdit = new QLineEdit(centralWidget);
        cartridgeLineEdit->setObjectName(QStringLiteral("cartridgeLineEdit"));
        cartridgeLineEdit->setEnabled(false);
        cartridgeLineEdit->setReadOnly(true);

        formLayout->setWidget(4, QFormLayout::FieldRole, cartridgeLineEdit);

        label_6 = new QLabel(centralWidget);
        label_6->setObjectName(QStringLiteral("label_6"));
        label_6->setEnabled(false);

        formLayout->setWidget(5, QFormLayout::LabelRole, label_6);

        label_7 = new QLabel(centralWidget);
        label_7->setObjectName(QStringLiteral("label_7"));
        label_7->setEnabled(false);

        formLayout->setWidget(6, QFormLayout::LabelRole, label_7);

        romLineEdit = new QLineEdit(centralWidget);
        romLineEdit->setObjectName(QStringLiteral("romLineEdit"));
        romLineEdit->setEnabled(false);
        romLineEdit->setReadOnly(true);

        formLayout->setWidget(5, QFormLayout::FieldRole, romLineEdit);

        ramLineEdit = new QLineEdit(centralWidget);
        ramLineEdit->setObjectName(QStringLiteral("ramLineEdit"));
        ramLineEdit->setEnabled(false);
        ramLineEdit->setReadOnly(true);

        formLayout->setWidget(6, QFormLayout::FieldRole, ramLineEdit);


        horizontalLayout_2->addLayout(formLayout);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(6);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        gbcCheckBox = new QCheckBox(centralWidget);
        gbcCheckBox->setObjectName(QStringLiteral("gbcCheckBox"));
        gbcCheckBox->setEnabled(false);
        gbcCheckBox->setCheckable(true);

        verticalLayout_2->addWidget(gbcCheckBox);

        sgbCheckBox = new QCheckBox(centralWidget);
        sgbCheckBox->setObjectName(QStringLiteral("sgbCheckBox"));
        sgbCheckBox->setEnabled(false);
        sgbCheckBox->setCheckable(true);

        verticalLayout_2->addWidget(sgbCheckBox);

        jpCheckBox = new QCheckBox(centralWidget);
        jpCheckBox->setObjectName(QStringLiteral("jpCheckBox"));
        jpCheckBox->setEnabled(false);
        jpCheckBox->setCheckable(true);

        verticalLayout_2->addWidget(jpCheckBox);

        logoOkCheckBox = new QCheckBox(centralWidget);
        logoOkCheckBox->setObjectName(QStringLiteral("logoOkCheckBox"));
        logoOkCheckBox->setEnabled(false);
        logoOkCheckBox->setCheckable(true);

        verticalLayout_2->addWidget(logoOkCheckBox);

        headerOkCheckBox = new QCheckBox(centralWidget);
        headerOkCheckBox->setObjectName(QStringLiteral("headerOkCheckBox"));
        headerOkCheckBox->setEnabled(false);
        headerOkCheckBox->setCheckable(true);

        verticalLayout_2->addWidget(headerOkCheckBox);

        globalOkCheckBox = new QCheckBox(centralWidget);
        globalOkCheckBox->setObjectName(QStringLiteral("globalOkCheckBox"));
        globalOkCheckBox->setEnabled(false);
        globalOkCheckBox->setCheckable(true);

        verticalLayout_2->addWidget(globalOkCheckBox);

        verticalSpacer_2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);


        horizontalLayout_2->addLayout(verticalLayout_2);


        verticalLayout->addLayout(horizontalLayout_2);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        gameboy_uiClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(gameboy_uiClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 324, 21));
        menuFile = new QMenu(menuBar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        gameboy_uiClass->setMenuBar(menuBar);
        QWidget::setTabOrder(loadRomPushButton, playPushButton);
        QWidget::setTabOrder(playPushButton, titleLineEdit);
        QWidget::setTabOrder(titleLineEdit, manufacturerLineEdit);
        QWidget::setTabOrder(manufacturerLineEdit, licenseLineEdit);
        QWidget::setTabOrder(licenseLineEdit, versionLineEdit);
        QWidget::setTabOrder(versionLineEdit, cartridgeLineEdit);
        QWidget::setTabOrder(cartridgeLineEdit, romLineEdit);
        QWidget::setTabOrder(romLineEdit, ramLineEdit);
        QWidget::setTabOrder(ramLineEdit, gbcCheckBox);
        QWidget::setTabOrder(gbcCheckBox, sgbCheckBox);
        QWidget::setTabOrder(sgbCheckBox, jpCheckBox);
        QWidget::setTabOrder(jpCheckBox, logoOkCheckBox);
        QWidget::setTabOrder(logoOkCheckBox, headerOkCheckBox);
        QWidget::setTabOrder(headerOkCheckBox, globalOkCheckBox);

        menuBar->addAction(menuFile->menuAction());
        menuFile->addAction(actionExit);

        retranslateUi(gameboy_uiClass);

        QMetaObject::connectSlotsByName(gameboy_uiClass);
    } // setupUi

    void retranslateUi(QMainWindow *gameboy_uiClass)
    {
        gameboy_uiClass->setWindowTitle(QApplication::translate("gameboy_uiClass", "gameboy_ui", 0));
        actionExit->setText(QApplication::translate("gameboy_uiClass", "Exit", 0));
        loadRomPushButton->setText(QApplication::translate("gameboy_uiClass", "Load ROM", 0));
        playPushButton->setText(QApplication::translate("gameboy_uiClass", "Play", 0));
        label->setText(QApplication::translate("gameboy_uiClass", "Title:", 0));
        label_2->setText(QApplication::translate("gameboy_uiClass", "Manufacturer:", 0));
        label_3->setText(QApplication::translate("gameboy_uiClass", "License:", 0));
        label_4->setText(QApplication::translate("gameboy_uiClass", "Version:", 0));
        label_5->setText(QApplication::translate("gameboy_uiClass", "Cartridge:", 0));
        label_6->setText(QApplication::translate("gameboy_uiClass", "ROM size:", 0));
        label_7->setText(QApplication::translate("gameboy_uiClass", "RAM size:", 0));
        gbcCheckBox->setText(QApplication::translate("gameboy_uiClass", "GBC", 0));
        sgbCheckBox->setText(QApplication::translate("gameboy_uiClass", "SGB", 0));
        jpCheckBox->setText(QApplication::translate("gameboy_uiClass", "JP", 0));
        logoOkCheckBox->setText(QApplication::translate("gameboy_uiClass", "Logo OK", 0));
        headerOkCheckBox->setText(QApplication::translate("gameboy_uiClass", "Header OK", 0));
        globalOkCheckBox->setText(QApplication::translate("gameboy_uiClass", "Global OK", 0));
        menuFile->setTitle(QApplication::translate("gameboy_uiClass", "&File", 0));
    } // retranslateUi

};

namespace Ui {
    class gameboy_uiClass: public Ui_gameboy_uiClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GAMEBOY_UI_H
