import QtQuick 2.15
import org.kde.plasma.configuration 2.0

ConfigModel {
    ConfigCategory {
        name: i18n("General")
        icon: "configure"
        source: "configGeneral.qml"
    }
    ConfigCategory {
        name: i18n("Voice Commands")
        icon: "dialog-scripts"
        source: "configCommands.qml"
    }
    ConfigCategory {
        name: i18n("J.A.R.V.I.S.")
        icon: "preferences-desktop-text-to-speech"
        source: "configAbout.qml"
    }
}
