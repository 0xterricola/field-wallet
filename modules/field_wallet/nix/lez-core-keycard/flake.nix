{
  description = "Field Wallet Keycard-enabled LEZ Core";

  inputs = {
    upstream.url =
      "github:logos-blockchain/logos-execution-zone-module?rev=b89e5d24df5babc2490d2e5d47756fb1f33435dd";

    logos-module-builder.follows =
      "upstream/logos-module-builder";

    nix-bundle-lgx.follows =
      "upstream/nix-bundle-lgx";

    nixpkgs.follows =
      "upstream/logos-module-builder/nixpkgs";

    wallet_ffi.url =
      "path:../logos-execution-zone-keycard";
  };

  outputs = inputs@{
    logos-module-builder,
    nixpkgs,
    upstream,
    ...
  }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      packagesFor = system:
        let
          pkgs = import nixpkgs {
            inherit system;
          };

          patchedSrc = pkgs.applyPatches {
            name = "field-lez-core-keycard-source";
            src = upstream.outPath;

            patches = [
              ./lez-core-keycard.patch
            ];
          };

          module =
            logos-module-builder.lib.mkLogosModule {
              # Crucial: codegen AND compilation now receive the
              # already-patched source tree.
              src = patchedSrc;

              # Metadata itself is unchanged, so it can be read
              # directly from the pinned upstream source at eval time.
              configFile =
                "${upstream.outPath}/metadata.json";

              flakeInputs = inputs;

              externalLibInputs = {
                wallet_ffi = {
                  input = inputs.wallet_ffi;
                  packages.default = "wallet";
                };
              };
            };
        in
          module.packages.${system};
    in {
      packages =
        builtins.listToAttrs (
          map
            (system: {
              name = system;
              value = packagesFor system;
            })
            systems
        );
    };
}
