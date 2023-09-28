# Copyright 2023 Sophos Limited. All rights reserved.
load("//tools/config:_providers.bzl", "ZipInfo")

def _create_thininstaller_asset_impl(ctx):
    name = ctx.label.name
    zipfile = ctx.actions.declare_file(name)
    sha256 = ctx.actions.declare_file(name + ".sha256")
    manifest = ctx.actions.declare_file(name + ".manifest.json")
    outputs = [zipfile, sha256, manifest]

    args = ctx.actions.args()
    args.add(ctx.file.installer_tar.path)
    args.add(ctx.file.compatibilty_script.path)
    args.add(ctx.executable._zipper.path)
    args.add(zipfile)

    inputs = [ctx.file.installer_tar, ctx.file.compatibilty_script, ctx.executable._zipper]

    ctx.actions.run(
        executable = ctx.executable._create_thininstaller_asset,
        inputs = inputs,
        outputs = outputs,
        mnemonic = "CreateThininstallerAsset",
        arguments = [args],
    )

    return [
        DefaultInfo(files = depset([zipfile])),
        ZipInfo(
            zip = zipfile,
            zip_sha256 = sha256,
            zip_manifest = manifest,
        ),
    ]

create_thininstaller_asset = rule(
    _create_thininstaller_asset_impl,
    attrs = {
        "installer_tar": attr.label(allow_single_file = True),
        "compatibilty_script": attr.label(allow_single_file = True),
        "_zipper": attr.label(
            default = "//tools/src/zipper",
            cfg = "exec",
            executable = True,
        ),
        "_create_thininstaller_asset": attr.label(
            default = "//thininstaller:create_thininstaller_asset",
            cfg = "exec",
            executable = True,
        ),
    },
)
