# Copyright 2023-2024 Sophos Limited. All rights reserved.

load(":version_ini.bzl", "version_ini")
load("//tools/config:copy_file.bzl", "copy_file")
load("//tools/config:zip.bzl", "zip_asset")
load("//common:strip.bzl", "strip")
load("//tools/config:sophos_sdds_package.bzl", "collate_sdds_packages")
load("//tools/config:sophos_versioning.bzl", "version_info")

def _generate_spv_template_impl(ctx):
    args = ctx.actions.args()
    args.add("--output", ctx.outputs.out.path)
    args.add("--product_line_id", ctx.attr.product_line_id)
    args.add("--product_line_canonical_name", ctx.attr.product_line_canonical_name)
    if len(ctx.attr.spv_features) > 0:
        args.add("--features")
        args.add_all(ctx.attr.spv_features)
    if len(ctx.attr.spv_platform) > 0:
        args.add("--platforms")
        args.add_all(ctx.attr.spv_platform)
    if len(ctx.attr.spv_roles) > 0:
        args.add("--roles")
        args.add_all(ctx.attr.spv_roles)
    if len(ctx.attr.spv_target_types) > 0:
        args.add("--target_types")
        args.add_all(ctx.attr.spv_target_types)

    outputs = [ctx.outputs.out]

    ctx.actions.run(
        executable = ctx.executable._generate_spv_template,
        outputs = outputs,
        mnemonic = "GenerateSpvTemplate",
        arguments = [args],
    )

    return [DefaultInfo(files = depset(outputs))]

_generate_spv_template = rule(
    _generate_spv_template_impl,
    attrs = {
        "out": attr.output(
            mandatory = True,
        ),
        "product_line_id": attr.string(
            mandatory = True,
        ),
        "product_line_canonical_name": attr.string(
            mandatory = True,
        ),
        "spv_features": attr.string_list(),
        "spv_platform": attr.string_list(),
        "spv_roles": attr.string_list(),
        "spv_target_types": attr.string_list(),
        "_generate_spv_template": attr.label(
            default = ":generate_spv_template",
            cfg = "exec",
            executable = True,
        ),
    },
)

def spl_sdds_packages(
        name,
        line_id,
        component_name,
        version_info,
        srcs_remapped = {},
        srcs_renamed = {},
        version_ini_locations = [],
        spv_features = [],
        spv_roles = [],
        spv_target_types = [],
        symbols_strip_prefix = [],
        correct_rpaths = False,
        rpath_exceptions = {},
        **kwargs):
    version_ini_renames = {}
    if len(version_ini_locations) > 0:
        version_ini(
            name = "{}_version_ini".format(name),
            out = "{}_VERSION.ini".format(name),
            component_name = component_name,
            deps = srcs_remapped.keys() + srcs_renamed.keys(),
            version_info = version_info,
        )

        for i in range(len(version_ini_locations)):
            copy_file(
                name = "{}_version_ini_{}".format(name, i),
                src = ":{}_version_ini".format(name),
                out = "{}_VERSION.ini.{}".format(name, i),
            )
            version_ini_renames["{}_VERSION.ini.{}".format(name, i)] = version_ini_locations[i]

    strip(
        name = "{}_files".format(name),
        srcs_remapped = srcs_remapped,
        srcs_renamed = srcs_renamed | version_ini_renames,
        rpath_exceptions = rpath_exceptions,
        correct_rpaths = correct_rpaths,
    )

    _generate_spv_template(
        name = "{}_spv_template".format(name),
        out = "{}_spv_template.xml".format(name),
        product_line_id = line_id,
        product_line_canonical_name = component_name,
        spv_features = spv_features,
        spv_platform = select({
            "//tools/config:linux_arm64_dbg": ["LINUX_ARM64"],
            "//tools/config:linux_arm64_rel": ["LINUX_ARM64"],
            "//tools/config:linux_x64_dbg": ["LINUX_INTEL_LIBC6"],
            "//tools/config:linux_x64_rel": ["LINUX_INTEL_LIBC6"],
        }),
        spv_roles = spv_roles,
        spv_target_types = spv_target_types,
    )

    collate_sdds_packages(
        name = name,
        files = [":{}_files_stripped".format(name)],
        spv_template = ":{}_spv_template".format(name),
        spv_version_token = "@ComponentAutoVersion@",
        tags = ["manual"],
        version_info = version_info,
    )

    zip_asset(
        name = "{}_symbols".format(name),
        srcs = [":{}_files_symbols".format(name), "//base/products/distribution:addSymbols.sh"],
        remap_paths = {
            "base/products/distribution": "",
            symbols_strip_prefix: "",
        },
        strip_prefix = "/",
        **kwargs
    )
