import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        PullDownMenu {
            busy: saracroche.busy

            MenuItem {
                text: "Retirer les numéros Saracroche"
                onClicked: remorse.execute("Retrait des numéros", function() { saracroche.removeAll() })
            }
            MenuItem {
                text: "Mettre à jour"
                enabled: !saracroche.busy
                onClicked: saracroche.sync()
            }
        }

        RemorsePopup { id: remorse }

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: "Saracroche"
                description: "Portage non officiel"
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: saracroche.blockedTotal
                font.pixelSize: Theme.fontSizeHuge
                color: Theme.highlightColor
            }
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "appels bloqués"
                color: Theme.secondaryHighlightColor
            }

            DetailItem {
                label: "Protection"
                value: saracroche.entryCount > 0 ? "Active" : "Désactivée"
            }
            DetailItem {
                label: "Numéros et préfixes bloqués"
                value: saracroche.entryCount
            }
            DetailItem {
                label: "Dernière mise à jour"
                value: saracroche.busy ? "En cours…"
                     : isNaN(saracroche.lastSync) ? "Jamais"
                     : Format.formatDate(saracroche.lastSync, Formatter.DurationElapsed)
            }

            Label {
                visible: saracroche.error !== ""
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.errorColor
                text: "Échec de la mise à jour : " + saracroche.error
            }

            TextSwitch {
                text: "Bloquer les numéros masqués"
                checked: saracroche.blockAnonymous
                automaticCheck: false
                onClicked: saracroche.blockAnonymous = !checked
            }

            SectionHeader { text: "Signaler un numéro" }

            TextField {
                id: number
                width: parent.width
                label: "Numéro de téléphone"
                placeholderText: label
                inputMethodHints: Qt.ImhDialableCharactersOnly
                EnterKey.enabled: false
            }

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.paddingLarge

                Button {
                    text: "Spam"
                    enabled: number.text.trim() !== ""
                    onClicked: saracroche.report(number.text, false)
                }
                Button {
                    text: "Légitime"
                    enabled: number.text.trim() !== ""
                    onClicked: saracroche.report(number.text, true)
                }
            }

            Label {
                id: reportResult
                visible: text !== ""
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }

            SectionHeader { text: "À propos" }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                linkColor: Theme.highlightColor
                textFormat: Text.StyledText
                onLinkActivated: Qt.openUrlExternally(link)
                text: "Avant de désinstaller l'application, choisissez « Retirer les numéros Saracroche » dans le menu, sinon les numéros restent bloqués.<br><br>"
                    + "Portage non officiel pour Sailfish OS de l'application <a href=\"https://saracroche.org\">Saracroche</a>. "
                    + "Listes © Saracroche — CC BY-NC-SA 4.0."
            }
        }

        VerticalScrollDecorator {}
    }

    Connections {
        target: saracroche
        onReportFinished: {
            reportResult.text = message
            if (ok)
                number.text = ""
        }
    }
}
