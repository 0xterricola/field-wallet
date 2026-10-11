{
  description = "Field Wallet core module for Logos";

  nixConfig = {
    extra-substituters = [ "https://cache.nix.logos.co/public" ];
    extra-trusted-public-keys = [ "public:l4HrXgL4nw246+LBh2SOJyhz64BoGegOYLheT/iIAPU=" ];
  };

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";
    nix-bundle-lgx.url = "github:logos-co/nix-bundle-lgx";

    lez_core = {
    url = "path:./nix/lez-core-keycard";

    # Field core and lez_core must use the same Keycard-enabled wallet FFI.
    inputs.wallet_ffi.follows = "keycard_wallet_ffi";
  };

    keycard_wallet_ffi.url = "path:./nix/logos-execution-zone-keycard";
  };

  outputs = inputs@{ logos-module-builder, ... }:
    logos-module-builder.lib.mkLogosModule {
      src = ./.;
      configFile = ./metadata.json;
      flakeInputs = inputs;

      externalLibInputs = {
        wallet_ffi = {
          input = inputs.keycard_wallet_ffi;
          packages.default = "wallet";
        };
      };
    };
}
