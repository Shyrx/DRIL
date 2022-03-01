{
  description = "DRIL flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.11";
    yaka-gistre.url = "git+file:///home/gawain/github/yaka-gistre";
    futils.url = "github:numtide/flake-utils";
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs = {
        flake-utils.follows = "futils";
        nixpie.follows = "nixpie";
      };
    };

  };

  outputs =
    { self
    , yaka-gistre
    , nixpkgs
    , futils
    , pre-commit-hooks
    }@inputs:
    let
      inherit (nixpkgs) lib;
      inherit (lib) attrValues recursiveUpdate optional;
      inherit (futils.lib) eachSystem;

      pkgImport = pkgs: system: withOverrides:
        import pkgs {
          inherit system;
          config = {
            allowUnfree = true;
          };
        };

      pkgset = system: {
        pkgs = pkgImport nixpkgs system true;
      };

      systems = [
        "x86_64-linux"
      ];
      anySystemOutputs = { };

      multiSystemOutputs = eachSystem systems (system: rec {

        yakalib = yaka-gistre.${system};
        apps = yaka-gistre.apps.${system};
        pkgs = yaka-gistre.pkgs.${system};
        checks = {
          pre-commit = pre-commit-hooks.lib.${system}.run {
            src = ./.;
            hooks.nixpkgs-fmt.enable = true;
          };
        };

        devShell = pkgs.mkShell {
          inherit (self.checks.${system}.pre-commit) shellHook;
          buildInputs = with pkgs; [
            qemu
            sl
            gcc
          ];
        };
      });
    in
    recursiveUpdate multiSystemOutputs anySystemOutputs;
}

