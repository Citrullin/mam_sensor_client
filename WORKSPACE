load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

git_repository(
    name = "entangled",
    commit = "fd0606857514e8a2bb7da955f9f3c00dcc3b047e",
    remote = "https://github.com/iotaledger/entangled.git",
)

git_repository(
    name = "rules_iota",
    commit = "e08b0038f376d6c82b80f5283bb0a86648bb58dc",
    remote = "https://github.com/iotaledger/rules_iota.git",
)

git_repository(
    name = "nanopb",
    commit = "a2db482712a575ab01c608ec129c8a07454be0ef",
    remote = "https://github.com/nanopb/nanopb.git",
)

git_repository(
    name = "iota_toolchains",
    commit = "f5cc06796268468992a1caa7edb5ccea0dc30956",
    remote = "https://github.com/iotaledger/toolchains.git"
)

load("@rules_iota//:defs.bzl", "iota_deps")
iota_deps()