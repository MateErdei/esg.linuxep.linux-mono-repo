# Copyright 2023 Sophos Limited. All rights reserved.

STRIP_EXCLUSION_EXTENSIONS = [
    "sh",
    "json",
    "zip",
    "crt",
    "dat",
    "pem",
    "pyi",
    "py",
    "ini",
    "conf",
    "txt",
    "typed",
]

def _should_strip(ctx):
    if "STRIP" in ctx.var:
        return ctx.var["STRIP"] != "0"

    # dbg and fastbuild should not be stripped
    return ctx.var["COMPILATION_MODE"] == "opt"

def _strip_impl(ctx):
    should_strip = _should_strip(ctx)

    if ctx.attr.extract_symbols:
        if should_strip:
            mode = "extract_symbols"
        else:
            # Don't extract symbols if we don't need to strip
            return [DefaultInfo()]
    elif should_strip:
        mode = "strip"
    else:
        mode = "rename"

    inputs = []
    outputs = []

    args = ctx.actions.args()
    args.set_param_file_format("multiline")
    args.use_param_file("@%s", use_always = True)

    args.add(ctx.file._strip.path)
    inputs.append(ctx.file._strip)

    args.add(mode)

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

    for src_path, dest_path in src_to_dest_map.items():
        if ctx.attr.extract_symbols:
            # When extracting symbols, don't bother doing so on certain file extensions
            extension = dest_path.split(".")[-1]
            if extension in STRIP_EXCLUSION_EXTENSIONS:
                continue

            dest_path += ".debug"

        output = ctx.actions.declare_file(dest_path)
        outputs.append(output)
        args.add("{}={}".format(src_path, output.path))

    ctx.actions.run(
        executable = ctx.executable._strip_script,
        inputs = inputs,
        outputs = outputs,
        mnemonic = "Strip",
        arguments = [args],
    )

    return [DefaultInfo(files = depset(outputs))]

_strip = rule(
    _strip_impl,
    attrs = {
        "srcs_renamed": attr.label_keyed_string_dict(
            allow_files = True,
        ),
        "srcs_remapped": attr.label_keyed_string_dict(
        ),
        "extract_symbols": attr.bool(
            default = False,
        ),
        "_strip_script": attr.label(
            default = ":strip",
            cfg = "exec",
            executable = True,
        ),
        "_strip": attr.label(
            default = "@gcc//:strip",
            allow_single_file = True,
        ),
    },
)

def strip(name, srcs_renamed = {}, srcs_remapped = {}, **kwargs):
    _strip(
        name = name + "_stripped",
        srcs_renamed = srcs_renamed,
        srcs_remapped = srcs_remapped,
        extract_symbols = False,
        **kwargs
    )
    _strip(
        name = name + "_symbols",
        srcs_renamed = srcs_renamed,
        srcs_remapped = srcs_remapped,
        extract_symbols = True,
        **kwargs
    )
