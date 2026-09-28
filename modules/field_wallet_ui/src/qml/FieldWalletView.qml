import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    implicitWidth: 900
    implicitHeight: 620
    color: "#0b0d10"

    readonly property var backend: logos.module("field_wallet_ui")
    property bool advancedMode: false
    property bool approvalVisible: false

    readonly property color panel: "#12161b"
    readonly property color line: "#252c35"
    readonly property color primary: "#f4f7fa"
    readonly property color secondary: "#8c98a7"
    readonly property color cyan: "#45e5d0"
    readonly property color violet: "#9c7cff"

    Rectangle {
        width: 260; height: 260; radius: 130
        x: root.width - 140; y: -155
        color: "transparent"; border.width: 1
        border.color: "#28524f"; opacity: 0.38
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.preferredWidth: 30
                Layout.preferredHeight: 30
                Rectangle {
                    anchors.centerIn: parent
                    width: 27; height: 27; radius: 14
                    color: "transparent"; border.width: 2
                    border.color: root.cyan
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: 15; height: 15; radius: 8
                    color: "transparent"; border.width: 2
                    border.color: root.violet
                }
            }

            Text {
                text: "field"
                color: root.primary
                font.pixelSize: 25
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                Layout.preferredWidth: 170
                Layout.preferredHeight: 34
                radius: 17
                color: root.panel
                border.width: 1
                border.color: root.line

                Text {
                    width: parent.width / 2
                    height: parent.height
                    text: "Simple"
                    color: root.advancedMode ? root.secondary : root.primary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 11
                }

                Text {
                    x: parent.width / 2
                    width: parent.width / 2
                    height: parent.height
                    text: "Advanced"
                    color: root.advancedMode ? root.primary : root.secondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 11
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.advancedMode = !root.advancedMode
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 120
            radius: 18
            color: root.panel
            border.width: 1
            border.color: root.line

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18

                ColumnLayout {
                    Text { text: "Personal"; color: root.primary; font.pixelSize: 15; font.weight: Font.DemiBold }
                    Text { text: "8K4f...d921"; color: root.secondary; font.family: "Monospace"; font.pixelSize: 11 }
                    Text { text: "Public account"; color: root.cyan; font.pixelSize: 10 }
                }

                Item { Layout.fillWidth: true }

                ColumnLayout {
                    Text { Layout.alignment: Qt.AlignRight; text: "1,284.72 LGO"; color: root.primary; font.pixelSize: 28; font.weight: Font.DemiBold }
                    Text { Layout.alignment: Qt.AlignRight; text: "available balance"; color: root.secondary; font.pixelSize: 10 }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 126
                Layout.fillHeight: true
                radius: 13
                color: "#16302b"
                border.width: 1
                border.color: "#2d786b"
                Text { anchors.centerIn: parent; text: "Send"; color: root.primary; font.pixelSize: 12; font.weight: Font.DemiBold }
                MouseArea { anchors.fill: parent; onClicked: root.approvalVisible = true }
            }

            Rectangle {
                Layout.preferredWidth: 126
                Layout.fillHeight: true
                radius: 13
                color: "#171c22"
                border.width: 1
                border.color: root.line
                Text { anchors.centerIn: parent; text: "Receive"; color: root.primary; font.pixelSize: 12; font.weight: Font.DemiBold }
            }

            Item { Layout.fillWidth: true }

            Text { text: "mock wallet data"; color: root.secondary; font.pixelSize: 10 }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: root.panel
                border.width: 1
                border.color: root.line

                Column {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text { text: "Assets"; color: root.primary; font.pixelSize: 14; font.weight: Font.DemiBold }
                    Rectangle { width: parent.width; height: 1; color: root.line }

                    Repeater {
                        model: [
                            { symbol: "LGO", name: "Logos", amount: "1,284.72" },
                            { symbol: "TOK", name: "Token Alpha", amount: "420" },
                            { symbol: "NFT", name: "Field Pass", amount: "1" }
                        ]

                        RowLayout {
                            required property var modelData
                            width: parent.width
                            height: 48

                            Text { text: modelData.symbol; color: root.cyan; font.pixelSize: 10; font.weight: Font.Bold }
                            Text { text: modelData.name; color: root.primary; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                            Text { text: modelData.amount; color: root.primary; font.pixelSize: 11 }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 18
                color: root.panel
                border.width: 1
                border.color: root.line

                Column {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text { text: "Activity"; color: root.primary; font.pixelSize: 14; font.weight: Font.DemiBold }
                    Rectangle { width: parent.width; height: 1; color: root.line }

                    Text { text: "Received LGO                         +40.00"; color: root.primary; font.pixelSize: 11 }
                    Text { text: "Sent Token Alpha                     -12 TOK"; color: root.primary; font.pixelSize: 11 }
                    Text { text: "Connected testimonial          2 capabilities"; color: root.secondary; font.pixelSize: 10 }
                    Text { text: "Connections                                    3 apps"; color: root.violet; font.pixelSize: 10 }
                }
            }
        }

        Rectangle {
            visible: root.advancedMode
            Layout.fillWidth: true
            Layout.preferredHeight: 62
            radius: 14
            color: "#11131a"
            border.width: 1
            border.color: "#40365e"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                Text { text: "Advanced"; color: root.violet; font.pixelSize: 11 }
                Text { text: "core: field_wallet"; color: root.primary; font.family: "Monospace"; font.pixelSize: 10 }
                Item { Layout.fillWidth: true }
                Text { text: "account: 8K4f...d921"; color: root.secondary; font.family: "Monospace"; font.pixelSize: 10 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Repeater {
                model: ["Home", "Assets", "Activity", "Connections", "Settings"]
                Text {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    text: modelData
                    color: index === 0 ? root.cyan : root.secondary
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 10
                }
            }
        }
    }

    Rectangle {
        visible: root.approvalVisible
        anchors.fill: parent
        color: "#b0000000"
        z: 50

        Rectangle {
            width: 410
            height: 350
            anchors.centerIn: parent
            radius: 22
            color: "#11161b"
            border.width: 1
            border.color: "#304039"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 22
                spacing: 12

                Text { text: "Transaction request"; color: root.primary; font.pixelSize: 19; font.weight: Font.DemiBold }
                Text { text: "testimonial  /  verified effect"; color: root.cyan; font.pixelSize: 10 }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 90
                    radius: 15
                    color: "#171c22"
                    border.width: 1
                    border.color: root.line

                    Column {
                        anchors.centerIn: parent
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Send"; color: root.secondary; font.pixelSize: 10 }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "12 TOK"; color: root.primary; font.pixelSize: 28; font.weight: Font.DemiBold }
                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Token Alpha"; color: root.secondary; font.pixelSize: 10 }
                    }
                }

                Text { text: "From     Personal  8K4f...d921"; color: root.primary; font.family: "Monospace"; font.pixelSize: 10 }
                Text { text: "To       3Ac1...91bf"; color: root.primary; font.family: "Monospace"; font.pixelSize: 10 }
                Text { text: "Program  Token / source verified"; color: root.cyan; font.pixelSize: 10 }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    spacing: 9

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 13
                        color: "#171c22"
                        border.width: 1
                        border.color: root.line
                        Text { anchors.centerIn: parent; text: "Reject"; color: root.primary; font.pixelSize: 11 }
                        MouseArea { anchors.fill: parent; onClicked: root.approvalVisible = false }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 13
                        color: "#16302b"
                        border.width: 1
                        border.color: "#2d786b"
                        Text { anchors.centerIn: parent; text: "Approve"; color: root.primary; font.pixelSize: 11; font.weight: Font.DemiBold }
                        MouseArea { anchors.fill: parent; onClicked: root.approvalVisible = false }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "Mock UI only - nothing is submitted yet."
                    color: "#6f7b88"
                    font.pixelSize: 8
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
