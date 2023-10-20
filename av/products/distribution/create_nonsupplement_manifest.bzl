# Copyright 2023 Sophos Limited. All rights reserved.

def _create_nonsupplement_manifest_impl(ctx):
    inputs = []
    outputs = []

    args = ctx.actions.args()
    args.set_param_file_format("multiline")
    args.use_param_file("@%s", use_always = True)

    src_to_dest_map = {}

    # srcs_renamed
    for src, dest_path in ctx.attr.srcs_renamed.items():
        src_files = src.files.to_list()
        if len(src_files) != 1:
            fail(
                "Target {} expands to multiple files, should only refer to one".format(src),
                "srcs_renamed",
            )

        src_file = src_files[0]
        inputs.append(src_file)
        src_to_dest_map[src_file.path] = dest_path

    # srcs_remapped
    for src, remaps_string in ctx.attr.srcs_remapped.items():
        remaps_list = remaps_string.split(",")
        remaps = {}
        for remap in remaps_list:
            prefix, replacement = remap.split(" -> ")
            remaps[prefix] = replacement

        src_files = src.files.to_list()
        inputs += src_files
        for src_file in src_files:
            dest_path = src_file.short_path  # Using short_path instead of path to exclude the execroot
            num_remaps = len(remaps) / 2
            for prefix, replacement in remaps.items():
                if dest_path.startswith(prefix):
                    dest_path = replacement + dest_path[len(prefix):]
                    break

            src_to_dest_map[src_file.path] = dest_path

    output = ctx.actions.declare_file(ctx.label.name)
    outputs = [output]
    args.add(output.path)

    for src_path, dest_path in src_to_dest_map.items():
        args.add("{}={}".format(src_path, dest_path))

    ctx.actions.run(
        executable = ctx.executable._executable,
        inputs = inputs,
        outputs = outputs,
        mnemonic = "Manifest",
        arguments = [args],
    )

    return [DefaultInfo(files = depset(outputs))]

create_nonsupplement_manifest = rule(
    _create_nonsupplement_manifest_impl,
    attrs = {
        "srcs_renamed": attr.label_keyed_string_dict(
            allow_files = True,
        ),
        "srcs_remapped": attr.label_keyed_string_dict(
        ),
        "_executable": attr.label(
            default = "//av/products/distribution:create_nonsupplement_manifest",
            cfg = "exec",
            executable = True,
        ),
    },
)
