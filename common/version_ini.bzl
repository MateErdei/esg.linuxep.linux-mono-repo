# Copyright 2023 Sophos Limited. All rights reserved.

load("//tools/config:sophos_versioning.bzl", "expand_version")

COMPONENT_AUTO_VERSION_TOKEN = "@ComponentAutoVersion@"

def _create_version_ini_impl(ctx):
    args = ctx.actions.args()
    args.add(ctx.attr.component_name)
    args.add(ctx.file.version_file.path)
    args.add(ctx.outputs.out.path)

    outputs = [ctx.outputs.out]

    ctx.actions.run(
        execution_requirements = {
            "no-sandbox": "1",  # To be able to access build/COMMIT_HASH without it being an action input
        },
        executable = ctx.executable._create_version_ini,
        inputs = [ctx.file.version_file],
        outputs = outputs,
        mnemonic = "CreateVersionIni",
        arguments = [args],
    )

    return [DefaultInfo(files = depset(outputs))]

_create_version_ini = rule(
    _create_version_ini_impl,
    attrs = {
        "component_name": attr.string(
            mandatory = True,
        ),
        "version_file": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "out": attr.output(
            mandatory = True,
        ),
        "_create_version_ini": attr.label(
            default = ":create_version_ini",
            cfg = "exec",
            executable = True,
        ),
    },
)

def version_ini(name, out, component_name, versioning_component_name, base_version, deps):
    # Create a template file to feed into expand_version
    native.genrule(
        name = "version_file_in",
        outs = ["version_file.in"],
        cmd_bash = "echo {} > $@".format(COMPONENT_AUTO_VERSION_TOKEN),
    )

    expand_version(
        name = "version_file",
        deps = deps,
        base_version_str = base_version,
        component_name = versioning_component_name,
        template_file = ":version_file_in",
        token = COMPONENT_AUTO_VERSION_TOKEN,
    )

    _create_version_ini(
        component_name = component_name,
        version_file = ":version_file",
        name = name,
        out = out,
    )
