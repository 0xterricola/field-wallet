import QtQuick
import QtTest
import "../../src/qml" as Wallet

Item {
    id: fixture
    width: 1000
    height: 720
    property QtObject logos: QtObject {
        function module(name) { return fakeBackend }
        signal viewModuleReadyChanged(string moduleName, bool isReady)
    }
    QtObject {
        id: fakeBackend
        property string walletState: "open"
        property string configPath: "/test/config.json"
        property string storagePath: "/test/storage.json"
        property string recoveryPhrase: ""
        property string savedWalletsJson: "[]"
        property string accountsJson: '[{"accountId":"aaaaaaaaaaaaaaaa","accountKind":"public"}]'
        property bool accountsLoaded: true
        property bool accountBusy: false
        property bool walletBusy: false
        property bool walletSwitchBusy: false
        property string accountId: "aaaaaaaaaaaaaaaa"
        property string accountKind: "public"
        property string balanceRaw: "0"
        property string walletError: ""
        property string walletFolderSelection: ""
        property bool walletFolderHasConfig: false
        property bool walletFolderHasStorage: false
        property int privateCreates: 0
        property int publicCreates: 0
        property string selected: ""
        function refreshWallet() {}
        function createPrivateAccount() { privateCreates++ }
        function createPublicAccount() { publicCreates++ }
        function renameAccount(id, name) {}
        function selectAccount(id) { selected = id; accountId = id }
    }
    Wallet.FieldWalletView { id: view; anchors.fill: parent }
    TestCase {
        name: "AccountControls"
        when: windowShown
        function init() {
            view.privacyMode = false
            fakeBackend.accountBusy = false
            fakeBackend.accountsLoaded = true
            fakeBackend.accountsJson = '[{"accountId":"aaaaaaaaaaaaaaaa","accountKind":"public"}]'
            fakeBackend.accountId = "aaaaaaaaaaaaaaaa"
            fakeBackend.privateCreates = 0
            fakeBackend.publicCreates = 0
            view.operationPending = false
        }
        function cleanup() {
            findChild(view, "accountMenu").close()
            findChild(view, "addAccountMenu").close()
            findChild(view, "accountManager").close()
        }
        function test_privateActionAvailableWithExistingPublicAccount() {
            var button = findChild(view, "addAccountButton")
            verify(button.visible)
            verify(button.enabled)
            mouseClick(button)
            var menu = findChild(view, "addAccountMenu")
            tryCompare(menu, "opened", true)
            var action = findChild(view, "createPrivateAccountAction")
            verify(action.visible)
            mouseClick(action)
            tryCompare(fakeBackend, "privateCreates", 1)
        }
        function test_accountMenuListsPublicAndPrivateAccounts() {
            fakeBackend.accountsJson = '[{"accountId":"aaaaaaaaaaaaaaaa","accountKind":"public"},{"accountId":"bbbbbbbbbbbbbbbb","accountKind":"private"}]'
            var button = findChild(view, "accountSelector")
            mouseClick(button)
            var menu = findChild(view, "accountMenu")
            tryCompare(menu, "opened", true)
            compare(menu.count, 5) // Two accounts, separator, management, refresh.
            compare(menu.itemAt(0).checked, true)
            verify(menu.itemAt(1).text.indexOf("Private") >= 0)
            mouseClick(menu.itemAt(1))
            tryCompare(fakeBackend, "selected", "bbbbbbbbbbbbbbbb")
        }
        function test_failedInventoryDoesNotShowFirstAccountPrompt() {
            fakeBackend.accountsLoaded = false
            fakeBackend.accountsJson = "[]"
            fakeBackend.accountId = ""
            verify(!findChild(view, "emptyWalletAccountPrompt").visible)
            verify(!findChild(view, "addAccountButton").enabled)
            fakeBackend.accountsLoaded = true
            verify(findChild(view, "emptyWalletAccountPrompt").visible)
        }
        function test_managerShowsAllAccountsAndPrivacyClosesIt() {
            fakeBackend.accountsJson = '[{"accountId":"aaaaaaaaaaaaaaaa","accountKind":"public","name":"Everyday"},{"accountId":"bbbbbbbbbbbbbbbb","accountKind":"private","name":"Private savings"}]'
            var manager = findChild(view, "accountManager")
            manager.open()
            tryCompare(manager, "opened", true)
            compare(findChild(view, "managedAccountList").count, 2)
            view.privacyMode = true
            tryCompare(manager, "opened", false)
        }
        function test_busyPreventsDuplicateActions() {
            fakeBackend.accountBusy = true
            verify(!findChild(view, "addAccountButton").enabled)
            verify(!findChild(view, "accountSelector").enabled)
        }
    }
}
