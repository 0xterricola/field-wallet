# Field Wallet Architecture

Field is a privacy-preserving wallet and provider SDK for Logos Execution Zones.

## Design principle

Field should build on the official Logos Execution Zone wallet/module stack rather
than reimplement LEZ cryptography, private execution, proofs, or transaction
construction.

Field's primary responsibilities are the wallet UX, provider SDK, permissions,
approval flows, signer abstraction, and application-facing interfaces.

## High-level architecture

    Third-party LEZ app
            |
            v
    Field Provider SDK
            |
            v
    Field Provider / Permission Layer
            |
            +-- connection requests
            +-- account grants
            +-- balance/state permissions
            +-- transaction proposals
            +-- approval requests
            +-- human-readable transaction effects
            +-- program verification status
            |
            v
       Field Wallet
            |
            +-- Simple UI
            +-- Advanced UI
            +-- CLI
            |
            v
       Signer Backend
            |
            +-- Software wallet
            +-- Keycard
            |
            v
      Official LEZ Core
            |
            v
    Logos Execution Zone

## Field responsibilities

Field owns:

- Wallet UX
- Simple and Advanced modes
- Wallet creation, open, restore, and import workflows
- Account presentation and switching
- Asset presentation
- dApp connection permissions
- Account-selection prompts
- Transaction approval prompts
- Human-readable transaction summaries
- Program verification presentation
- Provider sessions
- Provider SDK
- CLI UX
- Faucet reference application
- Testimonial reference application
- Signer abstraction
- Keycard integration
- Local wallet metadata such as labels and preferences

## LEZ Core responsibilities

The current LEZ Core interface already exposes:

- wallet creation
- opening wallet storage
- saving wallet state
- mnemonic-based storage restoration
- public account creation
- private account creation
- account listing
- balance lookup
- generic public transactions
- generic private transactions
- program deployment

Field should call these official interfaces rather than duplicate them.

## Provider security model

Third-party applications should interact with Field through the Provider SDK.

    dApp
      |
      | connect
      v
    Field
      |
      | user selects account
      | user approves access
      v
    Permission grant
      |
      | transaction request
      v
    Approval prompt
      |
      | user approves
      v
    LEZ Core

A dApp must not receive unrestricted wallet access.

Signing, transaction submission, and access to private account information
must pass through Field's permission and approval layer.

## Signers

Field should expose a signer abstraction.

    Signer
      |
      +-- SoftwareSigner
      |
      +-- KeycardSigner

The Provider SDK should not need to know which signer backs an account.

### Software signer

The initial implementation should use the official LEZ wallet/account
implementation and local persistence mechanisms.

### Keycard signer

Keycard support should reuse the existing Logos/LEZ Keycard work where
possible.

The currently inspected LEZ Core module still contains TODOs for Keycard
support, so the exact integration boundary needs further investigation.

Keycard support should not block the core LP-0021 implementation.

## User modes

Field exposes two presentation modes over the same wallet and security model.

### Simple mode

Focused on everyday wallet use:

- Balance
- Accounts
- Send
- Receive
- Assets
- Basic app connections
- Settings

### Advanced mode

Adds power-user and developer visibility:

- Detailed account information
- Activity
- Connected applications
- Permission grants
- Pending approvals
- Program verification
- Transaction details
- Keycard management
- Provider/developer information

## Proposed repository structure

    field-wallet/
    ├── docs/
    ├── crates/
    │   ├── wallet-core/
    │   ├── basecamp-module/
    │   ├── cli/
    │   └── keycard/
    ├── packages/
    │   └── provider-sdk/
    ├── apps/
    │   ├── basecamp-ui/
    │   ├── faucet/
    │   └── testimonial/
    └── tests/

These package boundaries are provisional until the Logos module interfaces are
fully inspected.

## Upstream repositories

Initial architecture investigation uses:

- logos-execution-zone-module
- logos-execution-zone-wallet-ui
- lez-programs

Checked revisions:

- logos-execution-zone-module: 825d2a4
- logos-execution-zone-wallet-ui: 67395b2
- lez-programs: 494dba8

## Confirmed upstream capabilities

From the checked-out LEZ Core header:

- create_new
- open
- save
- restore_storage
- create_account_public
- create_account_private
- list_accounts
- get_balance
- send_generic_public_transaction
- send_generic_private_transaction
- send_program_deployment_transaction

The existing wallet UI backend also exposes public/private account creation,
balances, sync, transfers, create-new, and open-existing workflows.

## Initial implementation priority

Field's first major implementation target should therefore be the provider,
permission, and approval layer rather than reimplementing underlying LEZ wallet
primitives.

## Non-goals for LP-0021

Field will not initially implement:

- LEZ cryptography from scratch
- a new ZK system
- a new token program
- NFTs
- staking
- bridging
- multi-zone management
- fiat on/off ramps
- portfolio price infrastructure
