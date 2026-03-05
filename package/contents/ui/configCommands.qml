import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.jarvis 1.0

Item {
    id: commandsRootItem
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    ScrollView {
        id: commandsRoot
        anchors.fill: parent
        contentWidth: availableWidth

    ColumnLayout {
        width: commandsRoot.availableWidth
        spacing: Kirigami.Units.largeSpacing

        // ── HEADER ──
        Kirigami.Heading {
            text: i18n("Voice Command Mappings")
            level: 2
            Layout.fillWidth: true
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: true
            type: Kirigami.MessageType.Information
            text: i18n("When you speak a trigger phrase, J.A.R.V.I.S. will execute the associated action instead of sending it to the AI.<br><br>" +
                       "<b>App</b> — launches an application. Use <code>||</code> to list fallbacks (e.g. <code>konsole||gnome-terminal||xterm</code>).<br>" +
                       "<b>Command</b> — runs a shell command directly.")
        }

        // ── COMMAND LIST ──
        Repeater {
            model: JarvisBackend.commandMappings
            delegate: Kirigami.AbstractCard {
                Layout.fillWidth: true
                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    Kirigami.Icon {
                        source: modelData.type === "app" ? "application-x-executable" : "utilities-terminal"
                        implicitWidth: Kirigami.Units.iconSizes.medium
                        implicitHeight: Kirigami.Units.iconSizes.medium
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        RowLayout {
                            spacing: Kirigami.Units.smallSpacing
                            Label {
                                text: "\"" + modelData.phrase + "\""
                                font.bold: true
                            }
                            Rectangle {
                                width: typeLabel.implicitWidth + 12
                                height: typeLabel.implicitHeight + 4
                                radius: 3
                                color: modelData.type === "app" ? Kirigami.Theme.positiveBackgroundColor : Kirigami.Theme.neutralBackgroundColor
                                Label {
                                    id: typeLabel
                                    anchors.centerIn: parent
                                    text: modelData.type
                                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                                    font.bold: true
                                    color: modelData.type === "app" ? Kirigami.Theme.positiveTextColor : Kirigami.Theme.neutralTextColor
                                }
                            }
                        }

                        Label {
                            text: modelData.action
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            font.family: "monospace"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        Label {
                            visible: modelData.desc && modelData.desc.length > 0
                            text: modelData.desc
                            color: Kirigami.Theme.disabledTextColor
                            font.pointSize: Kirigami.Theme.smallFont.pointSize
                            font.italic: true
                        }
                    }

                    Button {
                        icon.name: "edit-entry"
                        flat: true
                        ToolTip.text: i18n("Edit command")
                        ToolTip.visible: hovered
                        onClicked: {
                            editPhraseField.text = modelData.phrase
                            editActionField.text = modelData.action
                            editTypeCombo.currentIndex = modelData.type === "app" ? 0 : 1
                            commandsRootItem.editIndex = index
                            editDialog.open()
                        }
                    }

                    Button {
                        icon.name: "edit-delete"
                        flat: true
                        ToolTip.text: i18n("Remove command")
                        ToolTip.visible: hovered
                        onClicked: JarvisBackend.removeCommand(index)
                    }
                }
            }
        }

        // ── ADD NEW COMMAND ──
        Kirigami.Heading {
            text: i18n("Add New Command")
            level: 3
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
        }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Label {
                        text: i18n("When I say:")
                        font.bold: true
                        Layout.preferredWidth: 100
                    }
                    TextField {
                        id: newPhraseField
                        placeholderText: i18n("e.g. open music player")
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Label {
                        text: i18n("Then run:")
                        font.bold: true
                        Layout.preferredWidth: 100
                    }
                    TextField {
                        id: newActionField
                        placeholderText: i18n("e.g. audacious||rhythmbox||vlc")
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing
                    Label {
                        text: i18n("Type:")
                        font.bold: true
                        Layout.preferredWidth: 100
                    }
                    ComboBox {
                        id: newTypeCombo
                        model: [
                            i18n("app — Launch application"),
                            i18n("command — Run shell command")
                        ]
                        Layout.fillWidth: true
                        property string selectedType: currentIndex === 0 ? "app" : "command"
                    }
                }

                Button {
                    text: i18n("Add Command")
                    icon.name: "list-add"
                    enabled: newPhraseField.text.length > 0 && newActionField.text.length > 0
                    Layout.alignment: Qt.AlignRight
                    onClicked: {
                        JarvisBackend.addCommand(newPhraseField.text, newActionField.text, newTypeCombo.selectedType)
                        newPhraseField.text = ""
                        newActionField.text = ""
                    }
                }
            }
        }

        // ── RESET ──
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            Item { Layout.fillWidth: true }
            Button {
                text: i18n("Reset All to Defaults")
                icon.name: "edit-undo"
                onClicked: JarvisBackend.resetCommandsToDefaults()
            }
        }
    }

    }

    // ── EDIT DIALOG ──
    property int editIndex: -1

    Dialog {
        id: editDialog
        title: i18n("Edit Voice Command")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(450, commandsRoot.width * 0.9)

        ColumnLayout {
            spacing: Kirigami.Units.mediumSpacing
            width: parent.width

            Label { text: i18n("Trigger phrase:"); font.bold: true }
            TextField {
                id: editPhraseField
                Layout.fillWidth: true
                placeholderText: i18n("What you say...")
            }

            Label { text: i18n("Action:"); font.bold: true }
            TextField {
                id: editActionField
                Layout.fillWidth: true
                placeholderText: i18n("What happens...")
            }

            Label { text: i18n("Type:"); font.bold: true }
            ComboBox {
                id: editTypeCombo
                model: ["app", "command"]
                Layout.fillWidth: true
            }
        }

        onAccepted: {
            if (commandsRootItem.editIndex >= 0) {
                JarvisBackend.updateCommand(commandsRootItem.editIndex, editPhraseField.text, editActionField.text, editTypeCombo.currentText)
            }
        }
    }
}
