with import <nixpkgs> {};
let
  kernel = linuxPackages_4_14.kernel;

  in stdenv.mkDerivation {
    name = "env";
    buildInputs = [
      bashInteractive
      numactl
      pkg-config
      boost
      linuxHeaders
      cmake
      gcc8
      valgrind
      protobuf
      vimPlugins.vim-clang-format
      clang-tools
      gflags
    ];


    NIXOS_KERNELDIR = "${kernel.dev}/lib/modules/${kernel.modDirVersion}/build";
  }
