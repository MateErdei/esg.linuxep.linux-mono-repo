# Copyright (C) 2021 Sophos Limited. All Rights Reserved.
"""Tools to build SDDS3 suites/packages/supplements"""

####################################################################################
# Build SDDS3 package
####################################################################################
def _build_sdds3_package_impl(ctx):
    outputs = []
    outfile = ctx.actions.declare_file(ctx.attr.package)
    outputs.append(outfile)

    sdds_import = ctx.file.sdds_import

    args = ctx.actions.args()
    args.add(ctx.executable._make_sdds3_package.path)
    args.add("--fileset", sdds_import.dirname)
    args.add("--package-dir", "{}".format(outfile.dirname))
    if (ctx.attr.version):
        args.add("--version", ctx.attr.version)
    ctx.actions.run(
        mnemonic = "BuildSdds3Package",
        inputs = [sdds_import],
        outputs = outputs,
        arguments = [args],
        executable = ctx.executable._env,
        tools = [ctx.executable._make_sdds3_package, ctx.executable._sdds3_builder_exe],
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(files = depset(outputs)),
    ]

build_sdds3_package = rule(
    _build_sdds3_package_impl,
    attrs = {
        "package": attr.string(
            doc = "Fully-qualified name of the generated package",
        ),
        "version": attr.string(
            doc = "Override the version of from the SDDS-Import.xml file",
        ),
        "sdds_import": attr.label(
            allow_single_file = True,
        ),
        "_env": attr.label(
            default = ":env",
            executable = True,
            cfg = "host",
        ),
        "_make_sdds3_package": attr.label(
            default = "//redist/sdds3:make_sdds3_package",
            executable = True,
            cfg = "host",
        ),
        "_sdds3_builder_exe": attr.label(
            default = "//redist/sdds3:sdds3_builder_exe",
            executable = True,
            cfg = "host",
        ),
    },
)

####################################################################################
# Copy prebuilt SDDS3 packages
####################################################################################
def _copy_prebuilt_sdds3_package_impl(ctx):
    outputs = []
    outfile = ctx.actions.declare_file(ctx.attr.package)
    outputs.append(outfile)

    prebuilt_package = ctx.file.prebuilt_package
    ctx.actions.symlink(target_file = prebuilt_package, output = outfile)

    return [
        DefaultInfo(files = depset(outputs)),
    ]

copy_prebuilt_sdds3_package = rule(
    _copy_prebuilt_sdds3_package_impl,
    attrs = {
        "package": attr.string(
            doc = "Fully-qualified name of the generated package",
        ),
        "prebuilt_package": attr.label(
            allow_single_file = True,
        ),
    },
)

####################################################################################
# Copy prebuilt SDDS3 supplements
####################################################################################
def _copy_prebuilt_sdds3_supplement_impl(ctx):
    outputs = []
    outfile = ctx.actions.declare_file(ctx.attr.supplement)
    outputs.append(outfile)

    prebuilt_supplement = ctx.file.prebuilt_supplement
    ctx.actions.symlink(target_file = prebuilt_supplement, output = outfile)

    return [
        DefaultInfo(files = depset(outputs))
    ]

copy_prebuilt_sdds3_supplement = rule(
    _copy_prebuilt_sdds3_supplement_impl,
    attrs = {
        "supplement": attr.string(
            doc = "Fully-qualified name of the generated supplement",
        ),
        "prebuilt_supplement": attr.label(
            allow_single_file = True,
        ),
    },
)

####################################################################################
# Build SDDS3 suite
####################################################################################
def _build_sdds3_suite_impl(ctx):
    outputs = []
    outfile = ctx.actions.declare_file(ctx.attr.suite)
    outputs.append(outfile)

    metadata = ctx.file.metadata

    args = ctx.actions.args()
    args.add(ctx.executable._maker.path)
    args.add("--suite-dir", outfile.dirname)
    args.add("--metadata", metadata.path)
    args.add("--sdds3-builder", ctx.executable._sdds3_builder_exe.path)

    packages = depset(transitive = [f.files for f in ctx.attr.packages])
    args.add_all(packages, before_each = "--package")

    sdds_imports =  depset(transitive = [i.files for i  in ctx.attr.sdds_imports])
    args.add_all(sdds_imports, before_each = "--import")
    _allowed_integrity_duplicates = None
    inputs = depset(direct = [metadata], transitive = [packages, sdds_imports])

    ctx.actions.run(
        mnemonic = "BuildSdds3Suite",
        inputs = inputs,
        outputs = outputs,
        arguments = [args],
        executable = ctx.executable._env,
        tools = [ctx.executable._maker, ctx.executable._sdds3_builder_exe],
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(files = depset(outputs)),
    ]

build_sdds3_suite = rule(
    _build_sdds3_suite_impl,
    attrs = {
        "suite": attr.string(
            doc = "The name of the suite that will be generated",
        ),
        "sdds_imports": attr.label_list(
            doc = "List of SDDS-Import.xml files included in the suite",
        ),
        "packages": attr.label_list(
            doc = "List of built SDDS3 packages included in the suite",
        ),
        "metadata": attr.label(
            allow_single_file = True,
        ),
        "_env": attr.label(
            default = ":env",
            executable = True,
            cfg = "host",
        ),
        "_maker": attr.label(
            default = ":maker",
            executable = True,
            cfg = "host",
        ),
        "_sdds3_builder_exe": attr.label(
            default = "//redist/sdds3:sdds3_builder_exe",
            executable = True,
            cfg = "host",
        ),
    },
)

####################################################################################
# Build SDDS3 supplement
####################################################################################
def _build_sdds3_supplement_impl(ctx):
    outputs = []
    outfile = ctx.actions.declare_file(ctx.attr.supplement)
    outputs.append(outfile)

    args = ctx.actions.args()
    args.add(ctx.executable._maker.path)
    args.add("--supplement-dir", outfile.dirname)
    args.add("--sdds3-builder", ctx.executable._sdds3_builder_exe.path)
    packages = depset(transitive = [f.files for f in ctx.attr.packages])
    args.add_all(packages, before_each = "--package")

    sdds_imports =  depset(transitive = [i.files for i  in ctx.attr.sdds_imports])
    args.add_all(sdds_imports, before_each = "--import")

    args.add_all(ctx.attr.release_tags, before_each = "--tag")

    ctx.actions.run(
        mnemonic = "BuildSdds3Supplement",
        inputs =  depset(transitive = [packages, sdds_imports]),
        outputs = outputs,
        arguments = [args],
        executable = ctx.executable._env,
        tools = [ctx.executable._maker, ctx.executable._sdds3_builder_exe],
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(files = depset(outputs)),
    ]

build_sdds3_supplement = rule(
    _build_sdds3_supplement_impl,
    attrs = {
        "supplement": attr.string(
            doc = "The name of the supplement that will be generated",
        ),
        "sdds_imports": attr.label_list(
            doc = "List of SDDS-Import.xml files included in the supplement",
        ),
        "packages": attr.label_list(
            doc = "List of built SDDS3 packages included in the supplement",
        ),
        "release_tags": attr.string_list(
            doc = "List of tags for each package included in the supplement",
        ),
        "_env": attr.label(
            default = ":env",
            executable = True,
            cfg = "host",
        ),
        "_maker": attr.label(
            default = ":maker",
            executable = True,
            cfg = "host",
        ),
        "_sdds3_builder_exe": attr.label(
            default = "//redist/sdds3:sdds3_builder_exe",
            executable = True,
            cfg = "host",
        ),
    },
)

# vim: set ft=python sw=4 sts=4 et:
