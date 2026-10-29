import QtQuick 2.0
import Sailfish.Silica 1.0
import "pages"

ApplicationWindow {
    initialPage: Component { MainPage {} }
    cover: Component {
        CoverBackground {
            Column {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.paddingLarge
                spacing: Theme.paddingSmall

                Image {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "/usr/share/icons/hicolor/86x86/apps/saracroche-sailfish.png"
                }
                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    text: saracroche.blockedTotal
                    font.pixelSize: Theme.fontSizeHuge
                    color: Theme.highlightColor
                }
                Label {
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: "appels bloqués"
                    color: Theme.secondaryHighlightColor
                }
            }
        }
    }
    allowedOrientations: defaultAllowedOrientations
}
