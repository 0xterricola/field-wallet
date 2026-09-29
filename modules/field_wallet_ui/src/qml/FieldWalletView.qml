import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    focus: true
    implicitWidth: 900
    implicitHeight: 620
    color: "#0b0d10"


    readonly property var backend: logos ? logos.module("field_wallet_ui") : null
    property bool advancedMode: false
    property bool walletChooserVisible: false
    readonly property string currentWalletLabel:
        activeWalletName()
    property bool approvalVisible: false
    property bool showOpenFields: false
    property bool showCreateAnother: false
    property bool showKeycardConnect: false

    // "", "choose", "seed", "files"
    property string importMode: ""
    property string walletFolderUrl: ""

    function copyToClipboard(value) {
        clipboardProxy.text = value || ""
        clipboardProxy.selectAll()
        clipboardProxy.copy()
        clipboardProxy.text = ""
    }

    function walletFileUrl(fileName) {
        if (!walletFolderUrl.length)
            return ""

        return walletFolderUrl.endsWith("/")
            ? walletFolderUrl + fileName
            : walletFolderUrl + "/" + fileName
    }

    function walletFolderDisplay() {
        if (!walletFolderUrl.length)
            return ""

        var value = walletFolderUrl

        if (value.startsWith("file://"))
            value = value.substring(7)

        return decodeURIComponent(value)
    }

    property bool operationPending: false
    property string operationLabel: ""

    readonly property string walletState:
        (backend && backend.walletState) ? backend.walletState : "loading"

    readonly property string configPath:
        (backend && backend.configPath) ? backend.configPath : ""

    readonly property string storagePath:
        (backend && backend.storagePath) ? backend.storagePath : ""

    readonly property string recoveryPhrase:
        (backend && backend.recoveryPhrase) ? backend.recoveryPhrase : ""

    readonly property string savedWalletsJson:
        (backend && backend.savedWalletsJson)
            ? backend.savedWalletsJson
            : "[]"

    readonly property var savedWallets:
        parseSavedWallets(savedWalletsJson)

    readonly property bool hasSavedWallets:
        savedWallets.length > 0

    readonly property bool hasAccount:
        liveAccountId.length > 0

    readonly property string liveAccountId:
        (backend && backend.accountId) ? backend.accountId : ""

    readonly property string liveAccountKind:
        (backend && backend.accountKind) ? backend.accountKind : ""

    readonly property string liveBalanceRaw:
        (backend && backend.balanceRaw) ? backend.balanceRaw : ""

    readonly property string liveError:
        (backend && backend.walletError) ? backend.walletError : ""

    function walletMatches(wallet) {
        if (!backend || !wallet)
            return false

        return wallet.configPath === backend.configPath
            && wallet.storagePath === backend.storagePath
    }

    function activeWalletName() {
        for (var i = 0; i < savedWallets.length; ++i) {
            if (walletMatches(savedWallets[i]))
                return savedWallets[i].name || "Default"
        }

        return "Default"
    }

    function parseSavedWallets(raw) {
        try {
            var value = JSON.parse(raw)

            if (Array.isArray(value))
                return value
        } catch (error) {
        }

        return []
    }

    function validWalletName(name) {
        if (!name)
            return false

        if (name.length === 0 ||
            name.length > 48 ||
            name === "default")
            return false

        return /^[A-Za-z0-9_-]+$/.test(name)
    }

    function shortAccount(accountId) {
        if (!accountId ||
            accountId.length === 0)
            return "No account"

        if (accountId.length <= 16)
            return accountId

        return accountId.substring(0, 8)
            + "..."
            + accountId.substring(
                accountId.length - 6)
    }

    function accountKindLabel() {
        if (!liveAccountKind ||
            liveAccountKind.length === 0)
            return "loading account"

        return liveAccountKind
            .charAt(0)
            .toUpperCase()
            + liveAccountKind.substring(1)
            + " account"
    }

    function refreshWallet() {
        if (backend)
            backend.refreshWallet()
    }

    function runOperation(label, action) {
        if (operationPending)
            return

        root.forceActiveFocus()

        operationLabel = label
        operationPending = true
        operationTimer.action = action
        operationTimer.restart()
    }

    Timer {
        id: operationTimer

        interval: 60
        repeat: false
        property var action: null

        onTriggered: {
            var pendingAction = action
            action = null

            if (pendingAction)
                pendingAction()

            root.operationPending = false
            root.operationLabel = ""
        }
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            root.refreshWallet()
        })
    }

    Connections {
        target: logos

        function onViewModuleReadyChanged(
            moduleName,
            isReady) {
            if (moduleName ===
                    "field_wallet_ui" &&
                isReady) {
                Qt.callLater(function() {
                    root.refreshWallet()
                })
            }
        }
    }

    readonly property color panel: "#12161b"
    readonly property color line: "#252c35"
    readonly property color primary: "#f4f7fa"
    readonly property color secondary: "#8c98a7"
    readonly property color cyan: "#45e5d0"
    readonly property color violet: "#9c7cff"

    Image {
        id: openWalletHero
        visible: root.walletState === "open"
        anchors.fill: parent
        source: Qt.resolvedUrl(
            "../assets/branding/field-hero.png")
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        smooth: true
        mipmap: true
        opacity: 0.18
    }

    Rectangle {
        visible: root.walletState === "open"
        anchors.fill: parent
        color: "#9A080B0F"
    }


    // Wallet setup / import layout
    readonly property int onboardingWidth: 540
    readonly property int onboardingPageGap: 16
    readonly property int onboardingSectionGap: 10
    readonly property int onboardingControlHeight: 42
    readonly property int onboardingWalletCardHeight: 76
    readonly property int onboardingChoiceCardHeight: 68
    readonly property int onboardingControlRadius: 12
    readonly property int onboardingCardRadius: 14
    readonly property int onboardingFooterHeight: 34

    TextInput {
        id: clipboardProxy
        x: -10000
        y: -10000
        width: 1
        height: 1
        opacity: 0
        focus: false
    }

    ScrollView {
        id: walletScroll

        visible:
            root.walletState === "open"
            && root.hasAccount
            && root.recoveryPhrase.length === 0

        anchors.fill: parent
        clip: true

        contentWidth: availableWidth
        contentHeight: openWalletContent.implicitHeight + 48

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        Item {
            width: walletScroll.availableWidth
            implicitHeight: openWalletContent.implicitHeight + 48

            ColumnLayout {
                id: openWalletContent

                x: 24
                y: 24
                width: parent.width - 48
                spacing: 16

        RowLayout {
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: 174
                Layout.preferredHeight: 42
                radius: 21
                color: "#BF10151C"

                border.width: 1
                border.color: "#405CE7E7"

                Text {
                    anchors.centerIn: parent
                    text: "Field Wallet"
                    color: root.primary
                    font.pixelSize: 21
                    font.weight: Font.DemiBold
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                id: walletSelector

                Layout.preferredWidth:
                    walletSelectorText.implicitWidth + 34
                Layout.preferredHeight: 38

                radius: 19
                color: walletSelectorMouse.containsMouse
                    ? "#273039"
                    : "#C9171C22"

                border.width: 1
                border.color: walletSelectorMouse.containsMouse
                    ? "#40515F"
                    : root.line

                Behavior on color {
                    ColorAnimation { duration: 100 }
                }

                Text {
                    id: walletSelectorText
                    anchors.centerIn: parent

                    text: root.currentWalletLabel + "  ▾"
                    color: root.primary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                }

                MouseArea {
                    id: walletSelectorMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    onClicked: walletMenu.open()
                }
            }

            Popup {
                id: walletMenu

                parent: Overlay.overlay
                width: 220
                height: walletMenuColumn.implicitHeight + 16
                padding: 8

                x: {
                    var overlay = Overlay.overlay

                    if (!overlay)
                        return 8

                    var point =
                        walletSelector.mapToItem(
                            overlay,
                            walletSelector.width - width,
                            0)

                    return Math.max(
                        8,
                        Math.min(
                            overlay.width - width - 8,
                            point.x))
                }

                y: {
                    var overlay = Overlay.overlay

                    if (!overlay)
                        return 0

                    return walletSelector.mapToItem(
                        overlay,
                        0,
                        walletSelector.height + 8).y
                }

                modal: false
                focus: true

                closePolicy:
                    Popup.CloseOnEscape
                    | Popup.CloseOnPressOutside

                background: Rectangle {
                    radius: 12
                    color: "#F015191E"
                    border.width: 1
                    border.color: "#34404A"
                }

                contentItem: Column {
                    id: walletMenuColumn
                    spacing: 2

                    Repeater {
                        model: root.savedWallets

                        Rectangle {
                            required property var modelData

                            width: walletMenu.width - 16
                            height: 38
                            radius: 8

                            color: walletMenuMouse.containsMouse
                                ? "#273039"
                                : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8

                                Text {
                                    Layout.preferredWidth: 14

                                    text:
                                        root.walletMatches(modelData)
                                            ? "✓"
                                            : ""

                                    color: root.cyan
                                    font.pixelSize: 12
                                    font.weight: Font.DemiBold
                                }

                                Text {
                                    Layout.fillWidth: true

                                    text:
                                        modelData.name === "Default"
                                            ? "Default"
                                            : modelData.name

                                    color: root.primary
                                    font.pixelSize: 13
                                    font.weight:
                                        root.walletMatches(modelData)
                                            ? Font.DemiBold
                                            : Font.Medium

                                    elide: Text.ElideRight
                                }
                            }

                            MouseArea {
                                id: walletMenuMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor

                                onClicked: {
                                    if (!root.backend)
                                        return

                                    walletMenu.close()

                                    if (root.walletMatches(modelData))
                                        return

                                    root.runOperation(
                                        "Switching wallet…",
                                        function() {
                                            root.backend.switchWallet(
                                                modelData.configPath,
                                                modelData.storagePath)
                                        })
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: walletMenu.width - 16
                        height: 1
                        color: root.line
                    }

                    Rectangle {
                        width: walletMenu.width - 16
                        height: 38
                        radius: 8

                        color: allWalletsMouse.containsMouse
                            ? "#273039"
                            : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Text {
                                Layout.preferredWidth: 14
                                text: "⋯"
                                color: root.secondary
                                font.pixelSize: 14
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "All wallets…"
                                color: root.primary
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }
                        }

                        MouseArea {
                            id: allWalletsMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor

                            onClicked: {
                                walletMenu.close()

                                root.showCreateAnother = false
                                root.showKeycardConnect = false
                                root.importMode = ""
                                root.walletChooserVisible = true
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: lezBadge
                Layout.preferredWidth: 72
                Layout.preferredHeight: 34
                radius: 17
                color: "#A812171C"
                border.width: 1
                border.color: root.line

                Row {
                    anchors.centerIn: parent
                    spacing: 7

                    Text {
                        text: "●"
                        color: root.cyan
                        font.pixelSize: 9
                    }

                    Text {
                        text: "LEZ"
                        color: root.primary
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 190
                Layout.preferredHeight: 40
                radius: 20
                color: root.panel
                border.width: 1
                border.color: root.line

                Rectangle {
                    id: modeHighlight
                    x: root.advancedMode
                        ? parent.width / 2
                        : 0
                    width: parent.width / 2
                    height: parent.height
                    radius: 20
                    color: "#202830"
                    border.width: 1
                    border.color: "#34424D"

                    Behavior on x {
                        NumberAnimation {
                            duration: 140
                        }
                    }
                }

                Text {
                    width: parent.width / 2
                    height: parent.height
                    text: "Simple"
                    color: root.advancedMode ? root.secondary : root.primary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }

                Text {
                    x: parent.width / 2
                    width: parent.width / 2
                    height: parent.height
                    text: "Advanced"
                    color: root.advancedMode ? root.primary : root.secondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }

                MouseArea {
                    cursorShape: Qt.PointingHandCursor
                    x: 0
                    width: parent.width / 2
                    height: parent.height
                    onClicked: root.advancedMode = false
                }

                MouseArea {
                    cursorShape: Qt.PointingHandCursor
                    x: parent.width / 2
                    width: parent.width / 2
                    height: parent.height
                    onClicked: root.advancedMode = true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 146
            radius: 22
            color: "#D912161B"
            border.width: 1
            border.color: root.line

            RowLayout {
                anchors.fill: parent
                anchors.margins: 22

                ColumnLayout {
                    Text {
                        text: "Personal"
                        color: root.primary
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: root.shortAccount(root.liveAccountId)
                        color: root.secondary
                        font.family: "Monospace"
                        font.pixelSize: 13
                    }
                    Rectangle {
                        Layout.preferredWidth:
                            accountKindText.implicitWidth + 18
                        Layout.preferredHeight: 25
                        radius: 12
                        color: "#18332F"
                        border.width: 1
                        border.color: "#285C54"

                        Text {
                            id: accountKindText
                            anchors.centerIn: parent
                            text: root.accountKindLabel()
                            color: root.cyan
                            font.pixelSize: 11
                            font.weight: Font.Medium
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                ColumnLayout {
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: root.liveBalanceRaw.length
                            ? root.liveBalanceRaw
                            : "—"
                        color: root.primary
                        font.pixelSize: 36
                        font.weight: Font.DemiBold
                    }
                    Text {
                        Layout.alignment: Qt.AlignRight
                        text: "LEZ balance"
                        color: root.secondary
                        font.pixelSize: 12
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.minimumHeight: 52
            Layout.maximumHeight: 52
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 150
                Layout.fillHeight: true
                radius: 15
                color: "#164039"
                border.width: 1
                border.color: "#329A88"
                Text { anchors.centerIn: parent; text: "↗  Send"; color: root.primary; font.pixelSize: 14; font.weight: Font.DemiBold }
                opacity: 0.55
            }

            Rectangle {
                Layout.preferredWidth: 150
                Layout.fillHeight: true
                radius: 15
                color: "#C9171C22"
                border.width: 1
                border.color: root.line
                Text { anchors.centerIn: parent; text: "↓  Receive"; color: root.primary; font.pixelSize: 14; font.weight: Font.DemiBold }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.liveError.length
                    ? root.liveError
                    : "●  Field core connected"
                color: root.liveError.length
                    ? root.secondary
                    : root.cyan
                opacity: 0.8
                font.pixelSize: 11
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            Layout.minimumHeight: 200
            Layout.maximumHeight: 240
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

                    Text {
                        text: "Assets"
                        color: root.primary
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Rectangle { width: parent.width; height: 1; color: root.line }

                    Repeater {
                        model: [
                            {
                                symbol: "LGO",
                                name: "Logos",
                                amount: root.liveBalanceRaw.length
                                    ? root.liveBalanceRaw
                                    : "—"
                            }
                        ]

                        RowLayout {
                            required property var modelData
                            width: parent.width
                            height: 48

                            Rectangle {
                                Layout.preferredWidth: 34
                                Layout.preferredHeight: 34
                                radius: 17
                                color: "#17322F"
                                border.width: 1
                                border.color: "#2C625A"

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.symbol
                                    color: root.cyan
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                }
                            }
                            Text {
                                text: modelData.name
                                color: root.primary
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: modelData.amount
                                color: root.primary
                                font.pixelSize: 13
                                font.weight: Font.Medium
                            }
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

                    Text {
                        text: "Activity"
                        color: root.primary
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                    Rectangle { width: parent.width; height: 1; color: root.line }

                    Text {
                        text: "No activity data yet."
                        color: root.secondary
                        font.pixelSize: 11
                    }

                    Text {
                        text: "Field will populate this from real wallet transactions."
                        color: "#66717e"
                        font.pixelSize: 9
                    }
                }
            }
        }

        Item {
            id: openContentSpacer
            Layout.fillHeight: true
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
                Text { text: "Advanced"; color: root.violet; font.pixelSize: 12 }
                Text { text: "core: field_wallet"; color: root.primary; font.family: "Monospace"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Text { text: "account: " + root.shortAccount(root.liveAccountId); color: root.secondary; font.family: "Monospace"; font.pixelSize: 11 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.minimumHeight: 36
            Layout.maximumHeight: 36

            Repeater {
                model: ["Home", "Assets", "Activity", "Connections", "Settings"]
                Item {
                    required property int index
                    required property string modelData

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: -2

                        text: modelData
                        color: index === 0
                            ? root.cyan
                            : root.secondary
                        font.pixelSize: 12
                        font.weight: index === 0
                            ? Font.DemiBold
                            : Font.Normal
                    }

                    Rectangle {
                        visible: index === 0
                        width: 30
                        height: 2
                        radius: 1
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        color: root.cyan
                    }
                }
            }
        }
    }
        }
    }

    Rectangle {
        visible:
            root.recoveryPhrase.length === 0
            && (root.walletChooserVisible
                || root.walletState !== "open")

        anchors.fill: parent
        color: root.color
        z: 20

        Image {
            anchors.fill: parent
            source: Qt.resolvedUrl(
                "../assets/branding/field-hero.png")
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            smooth: true
            mipmap: true
        }

        // Keep onboarding controls readable while allowing the
        // Field landscape and ring to remain visible.
        Rectangle {
            anchors.fill: parent
            color: "#a30b0d10"
        }

        Text {
            Layout.fillWidth: true
            Layout.preferredHeight: root.onboardingFooterHeight
            Layout.minimumHeight: root.onboardingFooterHeight
            Layout.topMargin: 6
            verticalAlignment: Text.AlignVCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 24

            text: "LOGOS EXECUTION ZONES"
            color: root.primary
            opacity: 0.42
            font.pixelSize: 10
            font.weight: Font.Medium
            font.letterSpacing: 4.2
            z: 2
        }

        ColumnLayout {
            width: Math.min(root.onboardingWidth, parent.width - 48)
            anchors.top: parent.top
            anchors.topMargin: Math.max(72, parent.height * 0.11)
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: root.onboardingPageGap

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: fieldWalletTitle.implicitWidth + 44
                Layout.preferredHeight: 54

                radius: 27
                color: "#BF10151C"

                border.width: 1
                border.color: "#405ce7e7"

                Text {
                    id: fieldWalletTitle
                    anchors.centerIn: parent

                    text: "Field Wallet"
                    color: root.primary
                    font.pixelSize: 32
                    font.weight: Font.DemiBold
                }
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 3

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "PRIVATE BY DESIGN."
                    color: root.primary
                    opacity: 0.72
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    font.letterSpacing: 3.2
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "BUILT FOR WHAT'S NEXT."
                    color: root.primary
                    opacity: 0.72
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    font.letterSpacing: 3.2
                }
            }

            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text:
                    (root.walletState === "configured"
                     || root.walletChooserVisible)
                        ? (root.showCreateAnother
                            ? "Create a wallet"
                            : root.showKeycardConnect
                                ? "Connect Keycard Card"
                                : root.importMode === "choose"
                                ? "Import wallet"
                                : root.importMode === "seed"
                                    ? "Import with seed phrase"
                                    : root.importMode === "files"
                                        ? "Open wallet files"
                                        : "")
                        : root.walletState === "loading"
                            ? "Connecting to Field"
                            : root.walletState === "error"
                                ? "Field could not start"
                                : "Set up your wallet"

                color: root.primary
                font.pixelSize: 26
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                visible: root.walletState === "uninitialized"
                         || root.walletState === "incomplete"

                text:
                    "Create a wallet backed by the Logos Execution Zone, "
                    + "or open an existing wallet."

                color: root.secondary
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            ColumnLayout {
                visible:
                    (root.walletState === "configured"
                     || root.walletChooserVisible)
                    && root.hasSavedWallets
                    && root.importMode.length === 0
                    && !root.showKeycardConnect

                Layout.fillWidth: true
                spacing: root.onboardingSectionGap

                Text {
                    visible:
                        !root.showCreateAnother
                        && root.importMode.length === 0
                    Layout.fillWidth: true
                    text: "Your wallets"
                    color: root.primary
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                ScrollView {
                    id: walletChooserListScroll

                    Layout.fillWidth: true

                    Layout.preferredHeight: Math.min(
                        walletChooserListColumn.implicitHeight,
                        Math.max(
                            180,
                            root.height
                                - 430
                                - root.onboardingFooterHeight))

                    Layout.maximumHeight:
                        Math.max(
                            180,
                            root.height
                                - 430
                                - root.onboardingFooterHeight)

                    clip: true
                    contentWidth: availableWidth

                    ScrollBar.horizontal.policy:
                        ScrollBar.AlwaysOff

                    ScrollBar.vertical.policy:
                        ScrollBar.AsNeeded

                    ColumnLayout {
                        id: walletChooserListColumn

                        width:
                            walletChooserListScroll.availableWidth

                        spacing:
                            root.onboardingSectionGap

Repeater {
                    model:
                        root.showCreateAnother
                            || root.importMode.length > 0
                            ? []
                            : root.savedWallets

                    Rectangle {
                        id: walletCard
                        required property var modelData
                        property bool addressCopied: false
                        property bool renaming: false

                        Layout.fillWidth: true
                        Layout.preferredHeight: root.onboardingWalletCardHeight

                        radius: 14
                        color: "#252B32"
                        border.width: 1
                        border.color: "#39434D"

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 12

                            Item {
                                id: walletInfoArea

                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                HoverHandler {
                                    id: walletInfoHover
                                }

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 4

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 22

                                        Text {
                                            id: walletNameText

                                            anchors.left: parent.left
                                            anchors.verticalCenter:
                                                parent.verticalCenter

                                            text:
                                                modelData.name === "Default"
                                                    ? "Default"
                                                    : modelData.name

                                            visible: !walletCard.renaming
                                            color: root.primary
                                            font.pixelSize: 15
                                            font.weight: Font.DemiBold
                                        }

                                        TextField {
                                            id: renameField

                                            visible: walletCard.renaming

                                            anchors.left: parent.left
                                            anchors.verticalCenter:
                                                parent.verticalCenter

                                            width: Math.min(
                                                260,
                                                parent.width - 8)

                                            height: 26

                                            color: root.primary
                                            font.pixelSize: 14

                                            leftPadding: 8
                                            rightPadding: 8

                                            placeholderText: "Display name"
                                            placeholderTextColor:
                                                root.secondary

                                            background: Rectangle {
                                                radius: 7
                                                color: "#171C22"
                                                border.width: 1
                                                border.color:
                                                    renameField.activeFocus
                                                        ? root.cyan
                                                        : "#52616D"
                                            }

                                            function commitRename() {
                                                var value =
                                                    text.trim()

                                                if (!value.length
                                                        || !root.backend)
                                                    return

                                                root.backend.renameWallet(
                                                    modelData.storagePath,
                                                    value)

                                                walletCard.renaming = false
                                            }

                                            Keys.onPressed:
                                                function(event) {
                                                    if (event.key
                                                            === Qt.Key_Return
                                                            || event.key
                                                            === Qt.Key_Enter) {
                                                        commitRename()
                                                        event.accepted = true
                                                        return
                                                    }

                                                    if (event.key
                                                            === Qt.Key_Escape) {
                                                        walletCard.renaming =
                                                            false
                                                        event.accepted = true
                                                    }
                                                }
                                        }

                                        Rectangle {
                                            id: renameButton

                                            anchors.left: walletNameText.right
                                            anchors.leftMargin: 8
                                            anchors.verticalCenter:
                                                parent.verticalCenter

                                            width: 80
                                            height: 22
                                            radius: 7

                                            opacity:
                                                walletInfoHover.hovered
                                                && !walletCard.renaming
                                                    ? 1
                                                    : 0

                                            enabled:
                                                walletInfoHover.hovered
                                                && !walletCard.renaming

                                            color: renameMouse.containsMouse
                                                ? "#303B44"
                                                : "#29323A"

                                            border.width: 1
                                            border.color:
                                                renameMouse.containsMouse
                                                    ? "#52616D"
                                                    : "#3C4852"

                                            Behavior on opacity {
                                                NumberAnimation {
                                                    duration: 100
                                                }
                                            }

                                            Text {
                                                anchors.centerIn: parent
                                                text: "Edit name  ✎"
                                                color: root.secondary
                                                font.pixelSize: 11
                                                font.weight: Font.Medium
                                            }

                                            MouseArea {
                                                id: renameMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape:
                                                    Qt.PointingHandCursor

                                                onClicked: {
                                                    renameField.text =
                                                        modelData.name
                                                        || modelData.walletId
                                                        || ""

                                                    walletCard.renaming = true

                                                    Qt.callLater(
                                                        function() {
                                                            renameField
                                                                .forceActiveFocus()
                                                            renameField
                                                                .selectAll()
                                                        })
                                                }
                                            }
                                        }
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 22

                                        Text {
                                            id: walletAddressText

                                            anchors.left: parent.left
                                            anchors.verticalCenter:
                                                parent.verticalCenter

                                            width: Math.min(
                                                implicitWidth,
                                                walletInfoArea.width - 100)

                                            text: {
                                                if (root.advancedMode)
                                                    return modelData.storagePath || ""

                                                if (modelData.accountId) {
                                                    var id =
                                                        modelData.accountId

                                                    var shortId =
                                                        id.length > 16
                                                            ? id.substring(0, 8)
                                                              + "…"
                                                              + id.substring(
                                                                  id.length - 6)
                                                            : id

                                                    var kind =
                                                        modelData.accountKind
                                                            ? modelData.accountKind
                                                                  .charAt(0)
                                                                  .toUpperCase()
                                                              + modelData.accountKind
                                                                  .substring(1)
                                                            : "Account"

                                                    return kind
                                                        + " account · "
                                                        + shortId
                                                }

                                                return "Field-managed wallet"
                                            }

                                            color: root.secondary
                                            font.family: "Monospace"
                                            font.pixelSize: 12
                                            elide: Text.ElideMiddle
                                        }

                                        Rectangle {
                                            id: copyButton

                                            anchors.left:
                                                walletAddressText.right
                                            anchors.leftMargin: 8
                                            anchors.verticalCenter:
                                                parent.verticalCenter

                                            width:
                                                walletCard.addressCopied
                                                    ? 78
                                                    : 56

                                            height: 22
                                            radius: 7

                                            opacity:
                                                walletInfoHover.hovered
                                                && !!modelData.accountId
                                                    ? 1
                                                    : 0

                                            enabled:
                                                walletInfoHover.hovered
                                                && !!modelData.accountId

                                            color: copyMouse.containsMouse
                                                ? "#203A3B"
                                                : "#1C3032"

                                            border.width: 1
                                            border.color:
                                                copyMouse.containsMouse
                                                    ? "#4A8C91"
                                                    : "#34585B"

                                            Behavior on opacity {
                                                NumberAnimation {
                                                    duration: 100
                                                }
                                            }

                                            Text {
                                                anchors.centerIn: parent

                                                text:
                                                    walletCard.addressCopied
                                                        ? "Copied ✓"
                                                        : "Copy"

                                                color:
                                                    walletCard.addressCopied
                                                        ? root.cyan
                                                        : root.secondary

                                                font.pixelSize: 11
                                                font.weight: Font.Medium
                                            }

                                            MouseArea {
                                                id: copyMouse
                                                anchors.fill: parent
                                                hoverEnabled: true
                                                cursorShape:
                                                    Qt.PointingHandCursor

                                                onClicked: {
                                                    if (!modelData.accountId)
                                                        return

                                                    root.copyToClipboard(
                                                        modelData.accountId)

                                                    walletCard.addressCopied = true
                                                    copyFeedbackTimer.restart()
                                                }
                                            }
                                        }
                                    }
                                }

                                Timer {
                                    id: copyFeedbackTimer
                                    interval: 1200
                                    repeat: false

                                    onTriggered:
                                        walletCard.addressCopied = false
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 82
                                Layout.preferredHeight: 34

                                radius: 11
                                color: "#16302b"
                                border.width: 1
                                border.color: "#2d786b"

                                Text {
                                    anchors.centerIn: parent
                                    text: "Open"
                                    color: root.primary
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                }

                                MouseArea {
                                    cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                                    onClicked: {
                                        var wallet =
                                            parent.parent.parent
                                                .modelData

                                        if (!root.backend)
                                            return

                                        if (root.walletState === "open"
                                                && root.walletMatches(wallet)) {
                                            root.walletChooserVisible = false
                                            return
                                        }

                                        root.runOperation(
                                            root.walletState === "open"
                                                ? "Switching wallet…"
                                                : "Opening wallet…",
                                            function() {
                                                if (root.walletState === "open") {
                                                    root.backend.switchWallet(
                                                        wallet.configPath,
                                                        wallet.storagePath)
                                                } else {
                                                    root.backend.openWallet(
                                                        wallet.configPath,
                                                        wallet.storagePath)
                                                }

                                                if (!root.backend.walletError
                                                        || root.backend.walletError.length === 0) {
                                                    root.walletChooserVisible = false
                                                }
                                            })
                                    }
                                }
                            }
                        }
                    }
                }
                    }
                }

                Rectangle {
                    visible:
                        !root.showCreateAnother
                        && root.importMode.length === 0

                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight

                    radius: 12
                    color: "#15191e"
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "+ Create another wallet"

                        color: root.cyan
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                        onClicked:
                            root.showCreateAnother = true
                    }
                }

                ColumnLayout {
                    visible: root.showCreateAnother
                    Layout.fillWidth: true
                    spacing: root.onboardingSectionGap

                    TextField {
                        id: newWalletNameField

                        Layout.fillWidth: true

                        placeholderText:
                            "Wallet name, e.g. testing"

                        color: root.primary
                        placeholderTextColor: root.secondary

                        background: Rectangle {
                            radius: 11
                            color: root.panel
                            border.width: 1

                            border.color:
                                newWalletNameField.text.length === 0
                                    || root.validWalletName(
                                        newWalletNameField.text)
                                    ? root.line
                                    : "#7e4949"
                        }
                    }

                    Text {
                        visible:
                            newWalletNameField.text.length > 0
                            && !root.validWalletName(
                                newWalletNameField.text)

                        text:
                            "Use letters, numbers, - or _ only."

                        color: "#d28a8a"
                        font.pixelSize: 12
                    }

                    TextField {
                        id: newWalletPasswordField

                        Layout.fillWidth: true
                        placeholderText: "Wallet password"
                        echoMode: TextInput.Password

                        color: root.primary
                        placeholderTextColor: root.secondary

                        background: Rectangle {
                            radius: 11
                            color: root.panel
                            border.width: 1
                            border.color: root.line
                        }
                    }

                    TextField {
                        id: newWalletSequencerField

                        Layout.fillWidth: true

                        placeholderText:
                            "Sequencer URL (blank = Logos testnet)"

                        color: root.primary
                        placeholderTextColor: root.secondary

                        background: Rectangle {
                            radius: 11
                            color: root.panel
                            border.width: 1
                            border.color: root.line
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.onboardingControlHeight

                        readonly property bool canCreate:
                            root.validWalletName(
                                newWalletNameField.text)
                            && newWalletPasswordField.text.length > 0

                        radius: 12

                        color:
                            canCreate
                                ? "#16302b"
                                : "#171c22"

                        border.width: 1

                        border.color:
                            canCreate
                                ? "#2d786b"
                                : root.line

                        Text {
                            anchors.centerIn: parent
                            text: "Create wallet"

                            color:
                                parent.canCreate
                                    ? root.primary
                                    : root.secondary

                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        MouseArea {
                            cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                            enabled: parent.canCreate

                            onClicked: {
                                var walletName =
                                    newWalletNameField.text

                                var password =
                                    newWalletPasswordField.text

                                var sequencer =
                                    newWalletSequencerField.text

                                root.runOperation(
                                    "Creating wallet…",
                                    function() {
                                        root.backend
                                            .createNamedWallet(
                                                walletName,
                                                password,
                                                sequencer)
                                    })
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.onboardingControlHeight
                        radius: 12
                        color: "transparent"
                        border.width: 1
                        border.color: root.line

                        Text {
                            anchors.centerIn: parent
                            text: "Cancel"
                            color: root.secondary
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        MouseArea {
                            cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                            onClicked: {
                                newWalletNameField.text = ""
                                newWalletPasswordField.text = ""
                                newWalletSequencerField.text = ""
                                root.showCreateAnother = false
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                visible:
                    root.walletState === "uninitialized"
                    || root.walletState === "incomplete"

                Layout.fillWidth: true
                spacing: root.onboardingSectionGap

                TextField {
                    id: passwordField

                    Layout.fillWidth: true
                    placeholderText: "Wallet password"
                    echoMode: TextInput.Password
                    color: root.primary
                    placeholderTextColor: root.secondary

                    background: Rectangle {
                        radius: 12
                        color: root.panel
                        border.width: 1
                        border.color: root.line
                    }
                }

                TextField {
                    id: sequencerField

                    Layout.fillWidth: true
                    placeholderText:
                        "Sequencer URL (blank = Logos testnet)"
                    color: root.primary
                    placeholderTextColor: root.secondary

                    background: Rectangle {
                        radius: 12
                        color: root.panel
                        border.width: 1
                        border.color: root.line
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight
                    radius: 13

                    color:
                        passwordField.text.length > 0
                            ? "#16302b"
                            : "#171c22"

                    border.width: 1

                    border.color:
                        passwordField.text.length > 0
                            ? "#2d786b"
                            : root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Create wallet"
                        color:
                            passwordField.text.length > 0
                                ? root.primary
                                : root.secondary
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                        enabled: passwordField.text.length > 0

                        onClicked: {
                            if (!root.backend)
                                return

                            root.runOperation(
                                "Creating wallet…",
                                function() {
                                    root.backend.createWallet(
                                        passwordField.text,
                                        sequencerField.text)
                                })
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: root.onboardingControlHeight

                        radius: 12
                        color: "transparent"
                        border.width: 1
                        border.color: root.line

                        Text {
                            anchors.centerIn: parent
                            text: "Cancel"
                            color: root.secondary
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        MouseArea {
                            cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                            onClicked: {
                                newWalletNameField.text = ""
                                newWalletPasswordField.text = ""
                                newWalletSequencerField.text = ""
                                root.showCreateAnother = false
                            }
                        }
                    }
                }
            }

            // ------------------------------------------------------
            // Keycard hardware wallet
            // ------------------------------------------------------

            Rectangle {
                visible:
                    !root.showCreateAnother
                    && !root.showKeycardConnect
                    && root.importMode.length === 0
                    && root.walletState !== "loading"
                    && root.walletState !== "error"

                Layout.fillWidth: true
                Layout.preferredHeight:
                    root.onboardingControlHeight

                radius: root.onboardingControlRadius
                color: "#ED6E2D"
                border.width: 0

                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        text: "Connect Keycard Card"
                        color: "#fff8f4"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Text {
                        text: "USB reader"
                        color: "#fff8f4"
                        opacity: 0.82
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                }

                MouseArea {
                    cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                    onClicked: {
                        root.showKeycardConnect = true
                    }
                }
            }

            ColumnLayout {
                visible: root.showKeycardConnect
                Layout.fillWidth: true
                spacing: root.onboardingSectionGap

                Text {
                    Layout.fillWidth: true

                    text:
                        "Insert your Keycard Card 4.0 into a compatible USB smart-card reader. "
                        + "Keep the card inserted while Field "
                        + "communicates with it."

                    color: root.secondary
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight:
                        root.onboardingChoiceCardHeight

                    radius: root.onboardingCardRadius
                    color: root.panel
                    border.width: 1
                    border.color: root.line

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Text {
                                text: "Keycard Card 4.0"
                                color: root.primary
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }

                            Text {
                                text: "USB smart-card reader"
                                color: root.secondary
                                font.pixelSize: 12
                            }
                        }

                        Text {
                            text: "USB reader"
                            color: root.cyan
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight:
                        root.onboardingControlHeight

                    radius: root.onboardingControlRadius
                    color: "#171c22"
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Connect Keycard Card"
                        color: root.secondary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    // Intentionally disabled until USB / smartcard
                    // transport is wired into Field.
                }

                Text {
                    Layout.fillWidth: true
                    text: "This connects the Keycard Card directly through "
          + "a USB smart-card reader — not the Keycard Shell. "
          + "USB support is not connected to the Field backend yet."
                    color: root.secondary
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight:
                        root.onboardingControlHeight

                    radius: root.onboardingControlRadius
                    color: "transparent"
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: root.secondary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                        onClicked: root.showKeycardConnect = false
                    }
                }
            }

            // ------------------------------------------------------
            // Main "Import wallet" action
            // ------------------------------------------------------

            Rectangle {
                visible:
                    !root.showCreateAnother
                    && !root.showKeycardConnect
                    && root.importMode.length === 0
                    && root.walletState !== "loading"
                    && root.walletState !== "error"

                Layout.fillWidth: true
                Layout.preferredHeight: root.onboardingControlHeight

                radius: 13
                color: root.panel
                border.width: 1
                border.color: root.line

                Text {
                    anchors.centerIn: parent
                    text: "Import wallet"
                    color: root.primary
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }

                MouseArea {
                    cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                    onClicked: root.importMode = "choose"
                }
            }

            // ------------------------------------------------------
            // Import method chooser
            // ------------------------------------------------------

            ColumnLayout {
                visible: root.importMode === "choose"
                Layout.fillWidth: true
                spacing: root.onboardingSectionGap

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingChoiceCardHeight

                    radius: 13
                    color: root.panel
                    border.width: 1
                    border.color: root.line

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: "Import with seed phrase"
                            color: root.primary
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "Restore a Field wallet using its recovery phrase."
                            color: root.secondary
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                        onClicked: root.importMode = "seed"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingChoiceCardHeight

                    radius: 13
                    color: root.panel
                    border.width: 1
                    border.color: root.line

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 4

                        Text {
                            text: "Open wallet files"
                            color: root.primary
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: "Choose the folder containing an existing Field wallet."
                            color: root.secondary
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                        onClicked: root.importMode = "files"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight

                    radius: 12
                    color: "transparent"
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: root.secondary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                        onClicked: root.importMode = ""
                    }
                }
            }

            // ------------------------------------------------------
            // Seed phrase import
            // ------------------------------------------------------

            ColumnLayout {
                visible: root.importMode === "seed"
                Layout.fillWidth: true
                spacing: root.onboardingSectionGap

                TextField {
                    id: seedWalletNameField
                    Layout.fillWidth: true
                    placeholderText: "Wallet name"
                    color: root.primary
                    placeholderTextColor: root.secondary

                    background: Rectangle {
                        radius: 11
                        color: root.panel
                        border.width: 1
                        border.color: root.line
                    }
                }

                TextArea {
                    id: seedPhraseField

                    Layout.fillWidth: true
                    Layout.preferredHeight: 90

                    placeholderText: "Recovery phrase"
                    wrapMode: TextEdit.Wrap
                    color: root.primary
                    placeholderTextColor: root.secondary

                    background: Rectangle {
                        radius: 11
                        color: root.panel
                        border.width: 1
                        border.color: root.line
                    }
                }

                TextField {
                    id: seedPasswordField

                    Layout.fillWidth: true
                    placeholderText: "Wallet password"
                    echoMode: TextInput.Password
                    color: root.primary
                    placeholderTextColor: root.secondary

                    background: Rectangle {
                        radius: 11
                        color: root.panel
                        border.width: 1
                        border.color: root.line
                    }
                }

                TextField {
                    id: seedSequencerField

                    Layout.fillWidth: true
                    placeholderText: "Sequencer URL (blank = Logos testnet)"
                    color: root.primary
                    placeholderTextColor: root.secondary

                    background: Rectangle {
                        radius: 11
                        color: root.panel
                        border.width: 1
                        border.color: root.line
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight

                    radius: 12
                    color: "#171c22"
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Import wallet"
                        color: root.secondary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    // Backend seed restore gets wired next.
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight

                    radius: 12
                    color: "transparent"
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: root.secondary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                        onClicked: {
                            seedWalletNameField.text = ""
                            seedPhraseField.text = ""
                            seedPasswordField.text = ""
                            seedSequencerField.text = ""
                            root.importMode = ""
                        }
                    }
                }
            }

            // ------------------------------------------------------
            // Existing wallet folder
            // ------------------------------------------------------

            ColumnLayout {
                visible: root.importMode === "files"
                Layout.fillWidth: true
                spacing: root.onboardingSectionGap

                Connections {
                    target: root.backend

                    function onWalletFolderSelectionChanged() {
                        if (root.backend
                                && root.backend.walletFolderSelection.length > 0) {
                            root.walletFolderUrl =
                                root.backend.walletFolderSelection
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true

                    text:
                        "Choose the folder containing config.json and "
                        + "storage.json. Field checks only this folder, "
                        + "not subfolders."

                    color: root.secondary
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight

                    radius: 12
                    color: root.panel
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Choose wallet folder…"
                        color: root.cyan
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                        onClicked: {
                            if (root.backend)
                                root.backend.chooseWalletFolder()
                        }
                    }
                }

                Rectangle {
                    visible: root.walletFolderUrl.length > 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: 82

                    radius: 12
                    color: "#0f1317"
                    border.width: 1
                    border.color: root.line

                    Column {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 5

                        Text {
                            width: parent.width
                            text: root.walletFolderDisplay()
                            color: root.secondary
                            font.pixelSize: 12
                            elide: Text.ElideMiddle
                        }

                        Row {
                            spacing: 7

                            Text {
                                text:
                                    root.backend
                                    && root.backend.walletFolderHasConfig
                                        ? "✓"
                                        : "✕"

                                color:
                                    root.backend
                                    && root.backend.walletFolderHasConfig
                                        ? "#45d39a"
                                        : "#ef6b73"

                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }

                            Text {
                                text: "config.json"
                                color: root.primary
                                font.pixelSize: 13
                                font.family: "Monospace"
                            }
                        }

                        Row {
                            spacing: 7

                            Text {
                                text:
                                    root.backend
                                    && root.backend.walletFolderHasStorage
                                        ? "✓"
                                        : "✕"

                                color:
                                    root.backend
                                    && root.backend.walletFolderHasStorage
                                        ? "#45d39a"
                                        : "#ef6b73"

                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }

                            Text {
                                text: "storage.json"
                                color: root.primary
                                font.pixelSize: 13
                                font.family: "Monospace"
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight

                    readonly property bool canOpen:
                        root.walletFolderUrl.length > 0
                        && root.backend
                        && root.backend.walletFolderHasConfig
                        && root.backend.walletFolderHasStorage

                    radius: 12

                    color:
                        canOpen
                            ? "#16302b"
                            : "#171c22"

                    border.width: 1

                    border.color:
                        canOpen
                            ? "#2d786b"
                            : root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Open wallet"

                        color:
                            parent.canOpen
                                ? root.primary
                                : root.secondary

                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                        enabled: parent.canOpen

                        onClicked: {
                            if (!root.backend)
                                return

                            root.runOperation(
                                "Opening wallet…",
                                function() {
                                    root.backend.openWallet(
                                        root.walletFileUrl(
                                            "config.json"),
                                        root.walletFileUrl(
                                            "storage.json"))
                                })
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.onboardingControlHeight

                    radius: 12
                    color: "transparent"
                    border.width: 1
                    border.color: root.line

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        color: root.secondary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                        onClicked: {
                            root.walletFolderUrl = ""
                            root.importMode = ""
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.liveError.length > 0
                text: root.liveError
                color: "#e58b8b"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            Rectangle {
                visible:
                    root.walletState === "error"

                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                radius: 12
                color: root.panel
                border.width: 1
                border.color: root.line

                Text {
                    anchors.centerIn: parent
                    text: "Retry"
                    color: root.primary
                    font.pixelSize: 14
                }

                MouseArea {
                    cursorShape: Qt.PointingHandCursor; anchors.fill: parent
                    onClicked: root.refreshWallet()
                }
            }
        }
    }

    Rectangle {
        visible: root.recoveryPhrase.length > 0

        anchors.fill: parent
        color: root.color
        z: 30

        ColumnLayout {
            width: Math.min(600, parent.width - 48)
            anchors.centerIn: parent
            spacing: 16

            Text {
                Layout.fillWidth: true
                text: "Back up your recovery phrase"
                color: root.primary
                font.pixelSize: 23
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text:
                    "Write these words down and store them somewhere safe. "
                    + "Field will clear them from this screen after you continue."

                color: root.secondary
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            TextArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 150

                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap

                text: root.recoveryPhrase
                color: root.primary
                font.family: "Monospace"
                font.pixelSize: 15

                background: Rectangle {
                    radius: 16
                    color: root.panel
                    border.width: 1
                    border.color: "#31524f"
                }
            }

            Text {
                Layout.fillWidth: true
                text:
                    "Do not share this phrase with anyone. "
                    + "Field support will never ask for it."

                color: root.cyan
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: 13
                color: "#16302b"
                border.width: 1
                border.color: "#2d786b"

                Text {
                    anchors.centerIn: parent
                    text: "I've backed it up"
                    color: root.primary
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                MouseArea {
                    cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                    onClicked: {
                        if (root.backend)
                            root.backend.clearRecoveryPhrase()
                    }
                }
            }
        }
    }

    Rectangle {
        visible:
            root.walletState === "open"
            && !root.hasAccount
            && root.recoveryPhrase.length === 0

        anchors.fill: parent
        color: root.color
        z: 20

        ColumnLayout {
            width: Math.min(520, parent.width - 48)
            anchors.centerIn: parent
            spacing: 16

            Text {
                Layout.fillWidth: true
                text: "Wallet ready"
                color: root.primary
                font.pixelSize: 23
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                text:
                    "Create your first account. "
                    + "Public accounts are transparent; "
                    + "private accounts use LEZ privacy."

                color: root.secondary
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 46
                spacing: root.onboardingSectionGap

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 13
                    color: "#16302b"
                    border.width: 1
                    border.color: "#2d786b"

                    Text {
                        anchors.centerIn: parent
                        text: "Create public account"
                        color: root.primary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                        onClicked: {
                            if (!root.backend)
                                return

                            root.runOperation(
                                "Creating public account…",
                                function() {
                                    root.backend.createPublicAccount()
                                })
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 13
                    color: "#17152a"
                    border.width: 1
                    border.color: "#40365e"

                    Text {
                        anchors.centerIn: parent
                        text: "Create private account"
                        color: root.primary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    MouseArea {
                        cursorShape: Qt.PointingHandCursor; anchors.fill: parent

                        onClicked: {
                            if (!root.backend)
                                return

                            root.runOperation(
                                "Creating private account…",
                                function() {
                                    root.backend.createPrivateAccount()
                                })
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.liveError.length > 0
                text: root.liveError
                color: "#e58b8b"
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Rectangle {
        visible: root.operationPending
            || (root.backend && root.backend.walletBusy)
            || (root.backend && root.backend.walletSwitchBusy)

        anchors.fill: parent
        color: "#cc0b0d10"
        z: 100

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            hoverEnabled: true
            preventStealing: true
        }

        Column {
            anchors.centerIn: parent
            spacing: 18

            Item {
                width: 48
                height: 48
                anchors.horizontalCenter: parent.horizontalCenter

                RotationAnimator on rotation {
                    from: 0
                    to: 360
                    duration: 800
                    loops: Animation.Infinite
                    running: root.operationPending
                        || (root.backend && root.backend.walletBusy)
                        || (root.backend && root.backend.walletSwitchBusy)
                }

                Rectangle {
                    anchors.fill: parent
                    radius: width / 2
                    color: "transparent"
                    border.width: 2
                    border.color: "#34414b"
                }

                Rectangle {
                    width: 9
                    height: 9
                    radius: width / 2

                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter

                    color: root.cyan
                }
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter

                text:
                    root.backend && root.backend.walletSwitchBusy
                        ? "Switching wallet…"
                        : root.backend && root.backend.walletBusy
                            ? "Creating wallet…"
                            : root.operationLabel.length
                                ? root.operationLabel
                                : "Working…"

                color: root.primary
                font.pixelSize: 13
                font.weight: Font.DemiBold
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "This can take a few seconds."
                color: root.secondary
                font.pixelSize: 10
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
                        MouseArea {
                            cursorShape: Qt.PointingHandCursor; anchors.fill: parent; onClicked: root.approvalVisible = false }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 13
                        color: "#16302b"
                        border.width: 1
                        border.color: "#2d786b"
                        Text { anchors.centerIn: parent; text: "Approve"; color: root.primary; font.pixelSize: 11; font.weight: Font.DemiBold }
                        MouseArea {
                            cursorShape: Qt.PointingHandCursor; anchors.fill: parent; onClicked: root.approvalVisible = false }
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
