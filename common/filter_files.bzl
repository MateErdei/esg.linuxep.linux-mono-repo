def _filter_files_impl(ctx):
    srcs = []
    srcs += ctx.files.srcs
    if ctx.attr.include_runfiles:
        for src in ctx.attr.srcs:
            runfiles = src[DefaultInfo].default_runfiles
            if runfiles:
                srcs += runfiles.files.to_list()

    filtered_srcs = []
    for src in srcs:
        is_allowed = True
        for prefix in ctx.attr.disallowed_prefixes:
            if src.path.startswith(prefix):
                is_allowed = False
                break
        if not is_allowed:
            continue
        for suffix in ctx.attr.disallowed_suffixes:
            if src.path.endswith(suffix):
                is_allowed = False
                break
        if not is_allowed:
            continue
        for prefix in ctx.attr.allowed_prefixes:
            is_allowed = False
            if src.path.startswith(prefix):
                is_allowed = True
                break
        if not is_allowed:
            continue
        filtered_srcs.append(src)

    return [DefaultInfo(files = depset(filtered_srcs))]

filter_files = rule(
    _filter_files_impl,
    provides = [DefaultInfo],
    attrs = {
        "srcs": attr.label_list(
            providers = [DefaultInfo],
            allow_files = True,
        ),
        "allowed_prefixes": attr.string_list(),
        "disallowed_prefixes": attr.string_list(),
        "disallowed_suffixes": attr.string_list(),
        "include_runfiles": attr.bool(
            doc = "Whether to include runfile dependencies",
            default = False,
        ),
    },
)
