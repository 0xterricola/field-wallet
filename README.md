# Field Wallet

Field is a desktop wallet and permissioned wallet-provider module for Logos Execution Zones (LEZ), with a Wallet Provider SDK planned for third-party Logos applications. It builds on the official `lez_core` module: LEZ handles wallet primitives and transaction execution; Field provides wallet presentation, application permissions, and the approval boundary.

**Current stage: working wallet UI and provider backend, with the SDK and complete transaction experience still in development.** Field is being built toward [Logos λPrize LP-0021: LEZ Wallet and Provider SDK ($20,000)](https://github.com/logos-co/lambda-prize/blob/master/prizes/LP-0021.md).

The initial repository review used commit `42d0f17` on 2026-09-29; account management has since been extended as described below. This inventory distinguishes implemented code and automated checks from live-network verification. It does not claim a completed prize submission or newly verified end-to-end testnet operation.

## What is implemented

### Wallet interface

- **Create local wallets:** create the default wallet or additional named wallets, using a password passed to LEZ and an optional sequencer URL. Creation displays the returned recovery phrase; continuing clears it from the UI.
- **Manage multiple saved wallets:** choose, open, and switch between wallet vaults; edit their local display names and copy displayed account IDs. Switching saves and closes the previous wallet, opens the selected wallet, and attempts recovery of the previous wallet if opening fails.
- **Open existing wallet files:** the macOS folder picker checks for `config.json` and `storage.json` in the selected directory. This opens existing storage; it is not seed-phrase restoration.
- **Create public or private accounts:** use **+ Add account** on the dashboard, including when the wallet already has an account. Successful creation selects the returned account. Empty wallets also offer both types during onboarding. The UI flow is covered by simulated tests; live LEZ creation and restart persistence still need verification.
- **Track and select accounts:** the **Accounts** menu lists the current wallet’s accounts; **Manage accounts…** shows their saved names, public/private types, addresses, and selected marker. Names and the last selection are stored separately for each wallet.
- **Read an account balance:** the dashboard shows the selected account and its raw native balance. It is not a token portfolio or a total across accounts.
- **Use Simple or Advanced presentation:** Advanced currently adds account/core diagnostics and wallet storage paths.
- **Hide the interface with privacy mode:** use the bottom-right privacy toggle or **Cmd+L / Ctrl+L**. This is a visual privacy screen; it keeps the wallet open and does not provide authentication, encryption, or a wallet lock.

**One wallet can contain multiple public and private accounts.** The wallet dropdown switches vaults; the Accounts controls manage accounts inside the selected vault. Public and private accounts are separate entries, not automatically paired wallets.

### Provider and transaction backend

The [core API](modules/field_wallet/src/field_wallet_module.h) implements the following for authenticated Logos module callers. These APIs are not yet a packaged SDK or a connected approval UI.

| Area | Implemented behavior |
| --- | --- |
| Application identity | Uses the Logos runtime's caller identity, including module name and optional instance, rather than accepting a dApp-supplied identity as authority. |
| Capability requests | Requests separate `account.identity.read`, `account.balance.read`, and `transaction.propose` capabilities. |
| Account disclosure | Returns account identities only for the caller's approved identity grants. |
| Balance access | Requires a separate balance-read grant for the account, including private accounts. |
| Grant management | The trusted wallet UI module can grant requested capabilities against owned accounts and revoke a caller/account binding. |
| Native transfers | Supports proposals for public → public, private → public, public → owned private, and private → owned private transfers. Private destinations must belong to the active wallet when proposed. |
| Approval execution | Proposals do not submit transactions. A separate trusted approval call rechecks permission and account kind, records execution, then calls LEZ. |
| Request status | The originating dApp can query `pending`, `executing`, `succeeded`, `rejected`, `execution_failed`, or `indeterminate` requests. |
| Persistence | Approved permissions and transaction requests are saved locally. Interrupted executions become indeterminate after restart instead of automatically being submitted again. |

There are also unit-tested helpers for decoding token transfer instructions and token holding state. **They are not connected to token balance discovery, token transfers, or the wallet UI.** The transfer codec targets the documented LEZ `v0.2.5-rc2` format; testnet 0.3 compatibility still needs integration validation.

## What is still a preview or missing

The interface includes design work ahead of its functionality:

- **Send and Receive** have no action handlers yet; **Activity** has no transaction history data.
- Home, Assets, Activity, Connections, and Settings labels do not yet form complete navigation.
- The approval dialog contains a hardcoded example. Its buttons dismiss the preview; its “source verified” text is not a registry result.
- Seed-phrase import and Keycard connection screens are unwired.
- There is no published Provider SDK package, Field wallet CLI, faucet mini-app, or testimonial mini-app in this repository.
- Generic contract calls, token approval summaries, program-registry verification, gas reporting, and a complete dApp connection/approval experience remain to be implemented.

The development launcher starts the GUI; it is not the wallet CLI required by LP-0021.

## Architecture

The current implementation is two C++20 Logos modules and a Qt Quick interface:

```text
FieldWalletView.qml
        |
field_wallet_ui — Qt backend / Qt Remote Objects (.rep)
        |
        | generated Logos module calls
        v
field_wallet — wallet lifecycle, permissions, transaction requests
        ^                                      |
        | authenticated provider calls         | LEZ module calls
        |                                      v
Third-party Logos modules                   lez_core
                                               |
                                      configured LEZ sequencer
```

`field_wallet` is a core module built with `mkLogosModule` and depends on `lez_core`. `field_wallet_ui` is a `ui_qml` module built with `mkLogosQmlModule` and depends on `field_wallet`. Both currently report version `0.1.0`.

Field delegates wallet creation/open/save/close, account operations, balance reads, and native transfers to LEZ. Field does not implement its own LEZ cryptography, proving system, or token program.

### Storage and trust boundaries

- LEZ's `wallet_dir()` determines the default wallet location. The default vault uses `config.json`, `storage.json`, and `statistics.json`; named vaults live under `wallets/<wallet-name>/` below that location.
- Wallet display names, per-account names, and the selected account identity are stored with Qt `QSettings`, scoped to the wallet storage path (and account ID for account names). These are local display preferences, not a backup of keys. Renaming a label does not rename the vault directory or change an account ID.
- Provider grants live in `permissions.json` and transaction requests in `transactions.json` under Field's module-instance persistence directory. Pending capability requests are in memory only.
- These provider stores currently belong to the **module instance**, not individual saved wallets. Switching vaults does not rebind or clear them. Per-vault permission and pending-request handling needs explicit implementation and integration tests.
- Wallet-management and approval calls trust the authenticated `field_wallet_ui` module name. The current runtime integration does not bind that authority to an individual UI instance; see [approval authorization](modules/field_wallet/src/approval_authorization.h).
- Granting account access does not authorize future transaction execution. Each proposal requires the separate approval-execution call. Connecting that boundary to a real user prompt is still work to do.

Field's inspected source has no analytics, price-service integration, or remote backup feature. Wallet data remains in the local LEZ/Field storage paths, with network operations delegated to LEZ. A full dependency/network review is still needed before claiming complete compliance with the prize's privacy requirements.

### Repository map

| Path | Purpose |
| --- | --- |
| [modules/field_wallet/src/](modules/field_wallet/src/) | Wallet lifecycle, caller identity, capabilities, approvals, persistence, native-transfer requests, and token codec helpers. |
| [modules/field_wallet/tests/](modules/field_wallet/tests/) | Unit tests for backend helpers and state handling. |
| [modules/field_wallet_ui/src/](modules/field_wallet_ui/src/) | Qt backend, plugin, remote-object interface, QML, and packaged artwork. |
| [modules/field_wallet_ui/run-field-wallet](modules/field_wallet_ui/run-field-wallet) | Local development launcher. |
| Module `flake.nix`, `flake.lock`, `CMakeLists.txt`, `metadata.json` files | Build definitions, pinned dependencies, generated interfaces, and module packaging. |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Original design direction, including future signer and UI work. Its proposed crates/packages/apps tree is not the current repository layout. |
| [docs/PROVIDER.md](docs/PROVIDER.md) | Provider design principles and draft SDK surface; some historical open questions are already answered by the core implementation. |
| [docs/branding/field-brand-board.png](docs/branding/field-brand-board.png) | Brand reference. |

## Run the current development app

The existing local development setup uses Nix-generated artifacts and the Logos standalone UI host. From the repository root:

```sh
./modules/field_wallet_ui/run-field-wallet
```

The launcher expects `modules/field_wallet_ui/result-ui-dev/bin/run-logos-standalone-ui` to exist. It sets the working directory, supplies the Field Wallet window title, and forwards additional arguments. Generated `result*` paths are ignored by Git and are not included in a fresh checkout.

To build that launcher, use Nix with flakes enabled and the [module builder's documented development target](https://github.com/logos-co/logos-module-builder#ui-modules-the-dev-loop). Starting from the repository root:

```sh
cd modules/field_wallet_ui
nix build '.#ui-dev' -o result-ui-dev
./run-field-wallet
```

The UI flake includes Field core through `path:../field_wallet`; each module has its own lock file. Both CMake projects require C++20 and `LOGOS_MODULE_BUILDER_ROOT`, provided through the Logos build environment. There is no root-level flake.

The development wrapper supports local QML reloads, `DEV_QML_PATH` for an explicit QML directory, and `LOGOS_QML_HOT_RELOAD=0` to disable reloads. C++ changes require a rebuild and relaunch.

These commands follow the upstream builder documentation and existing launcher layout. The account-management UI plugin builds, but a fresh full development bundle currently fails because the pinned LEZ interface lacks `close`, which the existing Field core uses for wallet switching. The working local core/runtime was preserved for this UI update. Clean-checkout and both-platform verification remain outstanding.

### Current GUI walkthrough

1. Launch Field. Choose an existing saved wallet or create a wallet with a password. Leaving the sequencer URL blank selects `https://testnet.lez.logos.co`.
2. For a new wallet, record the recovery phrase, then select **I've backed it up** to continue. Seed restoration inside Field is not implemented yet.
3. Use **+ Add account → Create private account** (or **Create public account**). LEZ may initialize a new wallet with a public account already; that no longer hides account creation. After success, the dashboard selects the new account. Use **Accounts → Manage accounts…** to name, inspect, and select accounts.
4. Use the wallet dropdown or **All wallets…** to switch wallets, create another named wallet, or edit a display name. New vault names allow 1–48 letters, digits, hyphens, or underscores; `default` is reserved.
5. On macOS, opening an existing wallet folder requires `config.json` and `storage.json` directly inside it. The native folder picker is not implemented on other platforms yet.
6. Use **Advanced** for the current diagnostics and the privacy button or **Cmd+L / Ctrl+L** to obscure the interface.

There are no working Field CLI commands or user-facing send steps to document yet. Adding those walkthroughs is part of completing the features below.

## Development and validation

The backend test suite registers **117 unit cases across 17 test source files**, covering caller identity, capability separation, trusted approval access, ownership parsing, input validation, persistence, transaction-state transitions/restart recovery, LEZ result handling, and token codecs. See the [test build definition](modules/field_wallet/tests/CMakeLists.txt).

The [Logos test framework](https://github.com/logos-co/logos-test-framework) documents the builder's `unit-tests` target. From the repository root, the expected build/run sequence for this module's named test executable is:

```sh
cd modules/field_wallet
nix build '.#unit-tests' -L -o result-tests
./result-tests/bin/field_wallet_tests
```

The core tests exercise helpers and state logic. Additional [UI account tests](modules/field_wallet_ui/tests/README.md) compile the actual Qt backend against a simulated Logos transport and exercise the QML controls. They cover account creation/selection, per-wallet names and preferences, failure handling, and privacy-screen interaction without accessing real wallets. These checks do not establish live LEZ creation/persistence, transaction approvals, token support, or network transfers. No real-sequencer integration harness or CI workflow is currently tracked in the repository.

Preserve the working multi-wallet selection/switching, privacy screen, and development launcher while adding functionality. Changes should stay in Field; use the official LEZ interfaces rather than duplicating or modifying upstream internals as part of Field work.

## LP-0021 alignment

The authoritative target is the [full LP-0021 specification](https://github.com/logos-co/lambda-prize/blob/master/prizes/LP-0021.md), with context in the [prize announcement](https://forum.logos.co/t/new-prize-lp-0021-lez-wallet-and-provider-sdk-20-000/1949). The following assessment uses the specification supplied and reviewed on 2026-09-29. It is a development checklist, not a claim of acceptance.

| Required outcome | Repository evidence today | Remaining work / evidence |
| --- | --- | --- |
| Native and fungible-token assets on public/private accounts | Native account/balance APIs and four native-transfer proposal/execution paths; token codec helpers. | Working send/receive UI, fungible-token holdings/transfers, and end-to-end demonstrations for public/private paths. |
| Multiple public/private accounts with easy switching | Core account APIs, saved-wallet switching, and per-wallet account creation/selection/naming controls with simulated tests. | Verify public/private creation and persistence end-to-end on LEZ, including restarts. |
| Documented Provider SDK for access, balances/state, transfers and contract interactions | Permissioned module APIs and provider design notes. | Package/document the SDK, add the missing state/contract surface, and provide usable integration examples. |
| Connection → account selection → approval before signing/submission | Backend request, grant, revoke, proposal, reject, and execute operations. | Wire real dApp prompts to those operations and show caller, account, asset, amount, destination, and decoded contract effects, including token approvals. |
| Program verification and transaction costs | Approval artwork only; no registry or gas integration. | Show verification from a registry meeting or similar to LP-0023's source-verification requirements, and estimated/used gas when supported by testnet 0.3. |
| Faucet and testimonial reference apps | Neither app exists here. | SDK-driven faucet account selection with errors/rate limits; an on-chain testimonial program/app with text, optional username, and a submission identifier. |
| Basecamp GUI, CLI, and installable catalog releases | Core/QML modules, Nix builds, and a development GUI launcher. | Complete GUI flows and CLI; publish through a `logos-modules-release-base` catalog using `logos-modules-release-action`; supply its `logos-repo.json` URL. |
| Canonical testnet 0.3 operation | Default endpoint matches the specification. | Prove deployed end-to-end behavior; an endpoint setting alone does not prove compatibility. |
| Durable keys/accounts and permission-safe transactions | LEZ storage delegation, local journals, helper tests, and separate permission/approval operations. | Wallet lifecycle/network-drop tests, real consent integration, and correct provider-state handling across vault switches. |
| No mandatory third-party services or unapproved disclosure | No Field analytics, pricing, or remote-backup integration found. | Validate the full runtime/dependency path; any optional service must be disclosed and disableable without loss of wallet function. All analytics must be strictly opt-in; sharing addresses, balances, or transaction contents requires explicit opt-in. |
| Encrypted Logos Storage for remote persistence, if needed | No remote persistence feature. | If introduced, encrypt before upload and use Logos Storage. |
| Polished, responsive Linux/macOS experience | Live desktop wallet flows, async creation/switching, and busy/error display; macOS folder picker. | Finish navigation and transaction previews; verify packaging, installation, and responsiveness on both platforms. |
| Real-sequencer integration tests in green default-branch CI | Backend unit-test sources. | Add standalone-sequencer end-to-end coverage and CI, including restart/drop scenarios. |
| Setup/CLI/Basecamp docs and worked SDK examples | Current setup/account walkthrough plus design notes. | Reproducible clean-environment demo, complete CLI/Basecamp instructions, and faucet/testimonial SDK tutorials. |
| Public, rights-cleared MIT and Apache-2.0 submission | Repository has no license files or submission bundle. | Establish the required licensing and assemble the public deliverables, narrated walkthrough, adoption evidence, and FURPS self-assessment. |

### Adoption is required alongside the build

No adoption evidence is recorded in this repository. LP-0021 requires all of the following:

- **10 independent third-party developers**, independent of one another and the submitting team, each shipping a functional SDK-based Basecamp UI app with a public repository, genuine development history, and catalog publication under the specification's installation requirement.
- **150 on-chain testimonials from at least 150 distinct accounts**, submitted through the reference app on the official Logos zone and identifying this wallet/submission. Prior unrelated testnet activity matters to evaluation.
- Those testimonials spread over **at least two months**, with **at least 30 new testimonials in each month**, attributable by submission identifier and on-chain timestamps.
- **30 Discord testimonials and 30 Twitter/X testimonials** describing actual use, supported by credible account histories.

Start developer onboarding and real user feedback as soon as a usable SDK and reference flow ship, while completing the remaining wallet work. Keep app/repository links, testimonial transaction references, monthly counts, and social links as evidence. Meeting numerical thresholds alone does not establish acceptance; the specification also evaluates authenticity and the full functionality, usability, reliability, performance, and supportability criteria.

LP-0021 is a first-qualifying-submission prize. The specification requires a narrated demo, a clean-environment demo script, and a FURPS self-assessment, and limits submissions to three per team with at most one submission/review per week. Check the linked specification for current terms before submitting.

The narrated walkthrough must demonstrate key/multi-account setup including a private account, faucet funding, sending/receiving, fungible-token usage, and the testimonial app's full connect → account selection → approval → on-chain creation flow.

## Proposed roadmap

This sequence combines the original design goals with the implementation gaps and LP-0021 requirements. It is a proposed work order, not a committed delivery schedule.

1. **Complete the native transaction experience.** Validate account creation/persistence on LEZ, then add send/receive flows, live approval prompts, transaction outcomes/history, and vault-aware provider state. Preserve current wallet switching and privacy mode.
2. **Make the provider usable by other developers.** Ship a documented SDK, connect/account-selection/grant/revoke flows, balance/state access, and a safe contract-proposal surface with Field-derived summaries.
3. **Ship the reference apps early.** Implement the faucet and testimonial program/app through the same SDK and approval path; establish a submission identifier and begin authentic adoption tracking in parallel with development.
4. **Complete asset and approval coverage.** Integrate fungible-token holdings/transfers for public/private accounts, token/contract effect decoding, program source verification, and gas display where available. Validate codec compatibility with testnet 0.3.
5. **Finish recovery and platform usability.** Wire seed restoration/local backup workflows through LEZ, complete navigation and settings, and verify Linux/macOS behavior and responsiveness.
6. **Make installation and evaluation reproducible.** Add the wallet CLI, catalog releases, real-sequencer CI, clean-checkout setup/demo instructions, required licensing, narrated demo, FURPS assessment, and adoption evidence.

**Optional later work:** a signer abstraction and Keycard integration remain part of the original Field vision, but hardware wallets are outside LP-0021's required scope. NFTs are also optional; the existing decoder does not make them a supported wallet feature. Blockchain-layer accounts, staking, bridging, and multi-zone management belong to the follow-up LP-0022 scope. Price feeds, portfolio analytics, fiat ramps, and a new token program are not priorities for this prize.
