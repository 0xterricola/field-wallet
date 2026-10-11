{
  description = "Field Wallet Keycard-enabled LEZ wallet FFI";

  inputs = {
    upstream.url = "github:logos-blockchain/logos-execution-zone?rev=411adc8fdb3f4c3af64354c4fc26d3e4a6a3d3f4";
  };

  outputs = { self, upstream }:
    let
      systems = builtins.attrNames upstream.packages;
    in {
      packages = builtins.listToAttrs (map (system:
        let
          wallet = upstream.packages.${system}.wallet.overrideAttrs (old: {
            patches = (old.patches or []) ++ [
              ./wallet-keycard-explicit-pin.patch
              ./keycard-detection-debug.patch
            ];

            postPatch = (old.postPatch or "") + ''
              substituteInPlace lez/wallet-ffi/Cargo.toml \
                --replace-fail \
                "wallet.workspace = true" \
                $'wallet.workspace = true\nkeycard_wallet.workspace = true\nzeroize.workspace = true'

              substituteInPlace lez/wallet-ffi/src/lib.rs \
                --replace-fail \
                "pub mod generic_transaction;" \
                $'pub mod generic_transaction;\npub mod keycard;'

              cp ${./keycard.rs} lez/wallet-ffi/src/keycard.rs
            '';
          });
        in {
          name = system;
          value = {
            inherit wallet;
            default = wallet;
          };
        }
      ) systems);
    };
}
