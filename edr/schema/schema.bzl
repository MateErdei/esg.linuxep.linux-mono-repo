# Copyright 2020-2024 Sophos Limited. All rights reserved.
"""
Rule for building table plugin schema.
"""

def _schema_impl(ctx):
    args = ctx.actions.args()

    json_output = ctx.actions.declare_file("schema.json")
    args.add("--output-dir", json_output.dirname)
    outputs = [json_output]

    args.add("--headers")
    args.add_all(ctx.files.table_headers)

    table_headers = [ctx.attr.table_headers.files]
    ctx.actions.run(
        mnemonic = "Schema",
        inputs = depset(transitive = table_headers),
        outputs = outputs,
        arguments = [args],
        executable = ctx.executable._schema_builder,
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(files = depset(outputs)),
    ]

schema = rule(
    _schema_impl,
    attrs = {
        "table_headers": attr.label(
            mandatory = True,
            doc = "Query files to be built into the query pack",
        ),
        "_schema_builder": attr.label(
            default = ":schema_builder",
            executable = True,
            cfg = "exec",
        ),
    },
)
