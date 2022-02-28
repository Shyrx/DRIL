{
  description = "DRIL flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-21.11";
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
        inherit (pkgset system) pkgs;
        # Executed by `nix run .#<name>`
        # apps = ;
        # pkgs = ;
        # devshell = ;
        # Executed by `nix flake check`
        # checks."<system>"."<attr>" = derivation;
        # Executed by `nix build .#<name>`
        # packages."<system>"."<attr>" = derivation;
        # Executed by `nix build .`
        checks = {
          pre-commit = pre-commit-hooks.lib.${system}.run {
            src = ./.;
            hooks.nixpkgs-fmt.enable = true;
          };
        };

        devShell = pkgs.mkShell {
          inherit (self.checks.${system}.pre-commit) shellHook;
          buildInputs = with pkgs; [
            hello
            sl
          ];
        };
      });
    in
    recursiveUpdate multiSystemOutputs anySystemOutputs;
}

