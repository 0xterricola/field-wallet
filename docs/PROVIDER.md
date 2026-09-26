# Field Provider

The Field Provider is the application-facing interface between third-party
Logos applications and Field Wallet.

The provider never exposes unrestricted wallet access.

## Principles

- Applications request access; they do not choose wallet accounts silently.
- The user explicitly selects which account an application may use.
- Permissions are scoped per application and account.
- Private account information is not exposed without explicit permission.
- Connection approval and transaction approval are separate decisions.
- Connecting an account never grants permission to sign future transactions.
- Transactions are proposals until the user explicitly approves them.
- Signing and submission remain under Field's control.
- The provider API does not depend on whether an account uses a software
  signer or a hardware signer such as Keycard.

## Initial flow

### Connection approval

    dApp
      |
      | connect
      v
    Field Provider
      |
      | show requesting application
      | show requested permissions
      | user chooses account(s)
      | user approves or rejects
      v
    Session / Permission Grant

The resulting grant controls only the capabilities explicitly approved by the
user.

Connection approval does not authorize transaction signing.

### Transaction approval

    dApp
      |
      | requestTransaction(...)
      v
    Field Provider
      |
      | validate existing permission grant
      v
    Field Approval
      |
      | show requesting application
      | show selected account
      | show human-readable effects
      | show destination
      | show amount/assets
      | show program verification status
      v
    User approves or rejects
      |
      | approve
      v
    LEZ Core

Every transaction proposal requires its own approval unless a future,
explicitly designed permission model safely defines otherwise.

## Provider responsibilities

The provider should support, at minimum:

### Connection

- Request wallet connection
- Identify the requesting application
- Let the user select an account
- Grant or reject access
- Query current connection state
- Disconnect / revoke access

### Accounts

- Return only accounts granted to the requesting application
- Identify whether an account is public or private
- Never expose private signing material

### State

- Read balances/state only when permitted
- Require explicit permission before exposing private/shielded account state

### Transactions

A dApp may propose a transaction.

A proposal must not itself cause signing or submission.

Field must:

1. identify the requesting application
2. identify the selected account
3. decode the requested operation
4. present human-readable effects
5. show program verification status
6. ask the user to approve or reject
7. only then call LEZ Core

## Draft provider surface

Names are provisional until we establish the appropriate Logos module and SDK
transport.

    connect()
    disconnect()
    getConnection()
    getAccounts()
    getBalance(account)
    requestTransaction(request)

Possible wallet-side management operations:

    listConnections()
    getPermissions(app)
    revokePermissions(app)
    listPendingApprovals()

## Transaction request

A transaction request should contain enough information for Field to build a
safe approval view without trusting display text supplied by the dApp.

Conceptually:

    TransactionRequest
      requesting_app
      account
      program
      instruction
      accounts
      value/assets
      metadata

Field derives the human-readable effect summary itself.

The dApp must not be able to provide arbitrary text such as "safe transfer"
and have Field display it as authoritative.

## Sessions

A connection should create a permission grant scoped to:

    application
      -> account(s)
      -> capabilities

Initial capabilities:

    account.identity.read
    account.balance.read
    transaction.propose

For private accounts, Field must be able to distinguish identity access from
private state access. A connection may therefore expose an account identity
without automatically exposing its private balance or state.

Permission grants must be least-privilege and must never expand silently.

## Signer abstraction

Provider behavior must be independent of signer implementation.

    Provider
       |
       v
    Wallet Core
       |
       +-- Software signer
       |
       +-- Keycard signer

A connected application should not need different code when the user changes
the signer backing an account.

## Security boundary

Third-party applications must never:

- access raw private keys
- bypass Field approval
- submit transactions directly through Field without approval
- enumerate unauthorized private accounts
- read unauthorized private balances
- silently expand their permissions

## Open questions

- Exact transport between third-party Basecamp modules and field_wallet
- Session/application identity representation
- Persistent permission storage format
- Transaction instruction decoding strategy
- Program registry integration
- Whether public balance reads require approval after initial connection
- Provider SDK language/package format
