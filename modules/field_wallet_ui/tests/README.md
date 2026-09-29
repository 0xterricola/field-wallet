# Field UI account tests

These tests use generated Qt Remote Objects interfaces and the real
`FieldWalletBackend.cpp`, with a test-only replacement for the Logos transport.
They use temporary settings and simulated accounts; no wallet files, keys,
sequencer, or real transactions are accessed.

With Qt 6 Core, RemoteObjects, Test, Quick, Controls, Layouts, and their development
tools available in the environment, run from the repository root:

```sh
cmake -S modules/field_wallet_ui/tests/account_backend -B /tmp/field-account-tests
cmake --build /tmp/field-account-tests
ctest --test-dir /tmp/field-account-tests --output-on-failure
QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software QT_QUICK_CONTROLS_STYLE=Basic \
  qmltestrunner -input modules/field_wallet_ui/tests/account_ui
```

For a split Qt installation, set `CMAKE_PREFIX_PATH`/component package directories
for CMake and pass `-import <Qt QML import directory>` to `qmltestrunner`.

Coverage includes creating a private account alongside a public account,
selecting the returned account, changing selection and balance, remembered
selection and names per wallet, invalid renames, unknown accounts, malformed
inventories, failed/uncertain creation, and balance failures. QML checks exercise
the visible Add account action, mixed account menu, account manager, busy state,
empty-wallet gating, and closing account management when privacy mode activates.

## Manual LEZ check still required

1. Use a test wallet that already has a public account.
2. Choose **+ Add account → Create private account**.
3. Confirm a distinct account appears with a Private label and becomes selected.
4. Name the accounts under **Accounts → Manage accounts…** and switch between them.
5. Restart Field before switching wallets (wallet switching itself calls save),
   reopen the test wallet, and confirm both accounts still exist.
6. Switch to another wallet and back; check that its names and selected account
   remain separate. Check the privacy toggle and Cmd/Ctrl+L.

Wallet-key persistence is delegated to LEZ and is not proven by the simulated
backend tests. Do not treat a successful mock test as a successful live creation.
