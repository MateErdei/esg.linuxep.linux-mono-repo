# Copyright 2024 Sophos Limited. All rights reserved.

def _patch_osquery_for_openssl_config_impl(ctx):
    patched_osquery = ctx.actions.declare_file(ctx.label.name)

    args = ctx.actions.args()
    args.add(ctx.file.src.path)
    args.add(patched_osquery.path)

    outputs = [patched_osquery]

    ctx.actions.run(
        executable = ctx.executable._patch_osquery_for_openssl_config,
        inputs = [ctx.file.src],
        outputs = outputs,
        mnemonic = "PatchOsqueryForOpensslConfig",
        arguments = [args],
    )

    return [DefaultInfo(files = depset(outputs))]

patch_osquery_for_openssl_config = rule(
    _patch_osquery_for_openssl_config_impl,
    attrs = {
        "src": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "_patch_osquery_for_openssl_config": attr.label(
            default = "//imports/thirdparty/osquery:patch_osquery_for_openssl_config",
            cfg = "exec",
            executable = True,
        ),
    },
)
