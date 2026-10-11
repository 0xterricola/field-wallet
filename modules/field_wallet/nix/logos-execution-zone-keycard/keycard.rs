//! Keycard hardware-wallet FFI helpers used by Field Wallet.

use std::{
    ffi::{c_char, CStr, CString},
    ptr,
    str::FromStr as _,
};

use keycard_wallet::KeycardWallet;
use lee::AccountId;
use wallet::{
    account::AccountIdWithPrivacy,
    program_facades::native_token_transfer::NativeTokenTransfer,
    AccountIdentity,
};
use zeroize::Zeroizing;

use crate::{
    block_on,
    error::{print_error, WalletFfiError},
    map_execution_error,
    types::{FfiBytes32, FfiTransferResult, WalletHandle},
    wallet::get_wallet,
};

fn required_c_str(ptr: *const c_char, name: &str) -> Result<String, WalletFfiError> {
    if ptr.is_null() {
        print_error(format!("Null pointer for {name}"));
        return Err(WalletFfiError::NullPointer);
    }

    let value = unsafe { CStr::from_ptr(ptr) };

    value.to_str().map(str::to_owned).map_err(|error| {
        print_error(format!("Invalid UTF-8 in {name}: {error}"));
        WalletFfiError::InvalidUtf8
    })
}

/// Return whether a selectable Secure-Channel-V2 Keycard is available.
#[no_mangle]
pub extern "C" fn wallet_ffi_keycard_available() -> bool {
    KeycardWallet::is_keycard_available()
}

/// Verify the supplied PIN and derive the public LEE account at `key_path`.
///
/// The PIN exists only for the duration of this call and is zeroized afterward.
#[no_mangle]
pub unsafe extern "C" fn wallet_ffi_keycard_get_public_account(
    pin: *const c_char,
    key_path: *const c_char,
    out_account_id: *mut FfiBytes32,
) -> WalletFfiError {
    if out_account_id.is_null() {
        print_error("Null pointer for out_account_id");
        return WalletFfiError::NullPointer;
    }

    let pin = match required_c_str(pin, "pin") {
        Ok(value) => Zeroizing::new(value),
        Err(error) => return error,
    };

    let key_path = match required_c_str(key_path, "key_path") {
        Ok(value) => value,
        Err(error) => return error,
    };

    let account_id = match KeycardWallet::get_public_account_id_for_path_with_connect(
        pin.as_str(),
        &key_path,
    ) {
        Ok(value) => value,
        Err(error) => {
            print_error(format!("Keycard account derivation failed: {error}"));
            return WalletFfiError::InternalError;
        }
    };

    let account_id = match AccountIdWithPrivacy::from_str(&account_id) {
        Ok(AccountIdWithPrivacy::Public(account_id)) => account_id,
        Ok(AccountIdWithPrivacy::Private(_)) => {
            print_error("Keycard returned a private account identifier");
            return WalletFfiError::InvalidKeyValue;
        }
        Err(error) => {
            print_error(format!("Invalid Keycard account identifier: {error}"));
            return WalletFfiError::InvalidKeyValue;
        }
    };

    unsafe {
        *out_account_id = account_id.into();
    }

    WalletFfiError::Success
}


/// Send a public native-token transfer authorized by a Keycard.
///
/// The source account is explicitly represented as `PublicKeycard`, so LEZ
/// obtains its BIP340 authorization signature from the hardware device.
/// The caller-supplied PIN is zeroized after this call.
#[no_mangle]
pub unsafe extern "C" fn wallet_ffi_transfer_public_keycard(
    handle: *mut WalletHandle,
    from: *const FfiBytes32,
    to: *const FfiBytes32,
    amount: *const [u8; 16],
    pin: *const c_char,
    key_path: *const c_char,
    out_result: *mut FfiTransferResult,
) -> WalletFfiError {
    if from.is_null()
        || to.is_null()
        || amount.is_null()
        || out_result.is_null()
    {
        print_error("Null pointer argument");
        return WalletFfiError::NullPointer;
    }

    let pin = match required_c_str(pin, "pin") {
        Ok(value) => Zeroizing::new(value),
        Err(error) => return error,
    };

    let key_path = match required_c_str(key_path, "key_path") {
        Ok(value) => value,
        Err(error) => return error,
    };

    let wrapper = match get_wallet(handle) {
        Ok(wrapper) => wrapper,
        Err(error) => return error,
    };

    let wallet = match wrapper.core.lock() {
        Ok(wallet) => wallet,
        Err(error) => {
            print_error(format!("Failed to lock wallet: {error}"));
            return WalletFfiError::InternalError;
        }
    };

    let from_id = AccountId::new(unsafe { (*from).data });
    let to_id = AccountId::new(unsafe { (*to).data });
    let amount = u128::from_le_bytes(unsafe { *amount });

    let transfer = NativeTokenTransfer(&wallet);

    match block_on(
        transfer.send_public_transfer_with_keycard_pin(
            AccountIdentity::PublicKeycard {
                account_id: from_id,
                key_path,
            },
            AccountIdentity::Public(to_id),
            amount,
            pin,
        ),
    ) {
        Ok(tx_hash) => {
            let tx_hash = CString::new(tx_hash.to_string())
                .map_or(ptr::null_mut(), CString::into_raw);

            unsafe {
                (*out_result).tx_hash = tx_hash;
                (*out_result).success = true;
            }

            WalletFfiError::Success
        }
        Err(error) => {
            print_error(format!(
                "Keycard public transfer failed: {error:?}"
            ));

            unsafe {
                (*out_result).tx_hash = ptr::null_mut();
                (*out_result).success = false;
            }

            map_execution_error(error)
        }
    }
}
