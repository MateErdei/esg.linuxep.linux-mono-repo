# Copyright 2021-2023 Sophos Limited. All rights reserved.
"""
Provides version expanstion from files/bazel variables.
"""

load("//tools/config:copy_file.bzl", "copy_file_action")
load("//tools/config:manifest_sign.bzl", "manifest_sign")
load("//tools/config:sophos_utils.bzl", "add_files_with_path_to_parameters", "get_path_relative_to_package", "get_target_compatable_with", "to_single_file")
load("//tools/config:_providers.bzl", "ManifestInfo", "SDDSImportInfo", "ZipInfo")
load("//tools/config:zip.bzl", "strip_prefix", "zip_asset")
load("//tools/config:sophos_sdds_package.bzl", "sdds_import")
load("@rules_license//rules:compliance.bzl", "check_license")

def expand_version_impl(ctx):
    """Replaces token in the file with the build version of a particular component.

    Args:
      ctx: Build context.
    Returns:
        DefaultInfo - Expanded template
    """
    if ctx.attr.out:
        output_file = ctx.actions.declare_file(ctx.attr.out)
    else:
        output_file = ctx.actions.declare_file(ctx.attr.name)

    args = ctx.actions.args()
    args.add("--component_name", ctx.attr.component_name)
    args.add("--input", ctx.file.template_file)
    args.add("--token", ctx.attr.token)
    args.add("--out", output_file)

    if ctx.attr.base_version_str:
        if ctx.attr.base_version_file:
            fail("base_version and base_version_file are mutually exclusive.")
        args.add("--base_version", ctx.attr.base_version_str)
    elif ctx.attr.base_version_file:
        args.add("--base_version_file", ctx.file.base_version_file)
    else:
        fail("You must provide one of base_version or base_version_file.")

    args.add("--bazel_stamp_vars", ctx.version_file.path)

    # need to declare a dependency so it gets re-versioned if any inputs change
    inputs = depset(
        [ctx.file.template_file, ctx.version_file],
        transitive = [dep.files for dep in ctx.attr.deps],
    )

    ctx.actions.run(
        mnemonic = "ExpandVersion",
        outputs = [output_file],
        inputs = inputs,
        executable = ctx.executable._versioning_client,
        use_default_shell_env = True,  # Ensure PATH etc is defined
        arguments = [args],
    )

    return [
        DefaultInfo(files = depset([output_file])),
    ]

expand_version = rule(
    implementation = expand_version_impl,
    attrs = {
        "_versioning_client": attr.label(
            default = "//common/copied_from_monorepo:apply_version_template",
            executable = True,
            cfg = "exec",
            doc = "Evecutable to produce a versioned output",
        ),
        "deps": attr.label_list(
            mandatory = True,
            doc = """
        A list of depenencies which if changed require a new version of this rule to be evaulated.
        """,
            allow_files = True,
        ),
        "template_file": attr.label(
            allow_single_file = True,
            doc = "File to expand using (token) substitiutions.",
        ),
        "base_version_str": attr.string(
            default = "",
            doc = "Dotted base version as a build variable",
        ),
        "base_version_file": attr.label(
            default = None,
            allow_single_file = True,
            doc = "Dotted base version as a build file (first line contains base version",
        ),
        "component_name": attr.string(
            default = "",
            doc = "Component name sent to the signing service.",
        ),
        "token": attr.string(
            doc = "Token to replace in template file with full version.",
        ),
        "out": attr.string(
            doc = "Output file name. If not provided, the rule name is used.",
        ),
    },
)

def collate_sdds_packages(
        name,
        spv_template,
        versioning_component_name,
        spv_version_token,
        base_version_str,
        files = None,
        make_sdds3_package = None,
        sdds3_builder_exe = None,
        manifest_name = "",
        manifest_full_name = "",
        manifest_path_map = None,
        license_path = None,
        legacy_manifest_sign = False,
        copyright_notices_path = None,
        **kwargs):
    """Macro to produce all sophos packages and collating the output.

    In the specified folder it will stage the dependency files, and create the following output.

    Primary output folders:
    //package/{name}/
    //package/{name}_sdds3/


    All {files} are copied under //package/{name}/ with a path relative to those files package root
    It then generates the following files
    //package/{name}/SDDS-Import.xml
    //package/{name}/manifest.dat if manifest_name == "" else //package/{name}/{manifest_name}.manifest.dat

    Then an SDDS3 package is generated from the SDDS2 package (//package/{name}/ -> //package/{name}_sdds3/)

    Output Targets:
    {name} - filegroup of SDDS3 package file and SDDS-Import.xml + all dependent files
    {name}_sdds3 - SDDS3 zipped asset
    {name}_sdds2 - SDDS2 fileset in a zipped asset
    {name}.packages - filegroup of SDDS2 and SDDS3 packages.

    Then it will create an SDDS3 import zip file.
    Args:
        name: name of the resultant filegroup
        spv_template: template file to expand into spv file for generating the SDDS-Import.xml
        files: Files to add relative to their package
        versioning_component_name: name to give to the versioning service
        spv_version_token: string to replace in the template file for the version
        base_version_str: version to base the build version from in the SDDS import file.
        make_sdds3_package: executable to wrap making the SDDS3 package
        sdds3_builder_exe: executable from SAU to make the SDDS3 zip file
        manifest_name: name to prepend to the manifest file.
        manifest_full_name: complete name of the manifest file.
        manifest_path_map: if provided don't generate the default manifest, and files must be empty.
                        files will be ingested from the ManifestInfo provided relative to the offset provided in the key.
        license_path: path of the existing license file to overwrite otherwise set to default of license.txt in the root
        legacy_manifest_sign: Don't use enable SHA-1 signing
        copyright_notices_path: Text file to place copyright_notices in
        **kwargs: forwarded to all rules
    """

    target_compatible_with = [] + kwargs.pop("target_compatible_with", [])
    target_compatible_with += get_target_compatable_with()

    if manifest_path_map and files:
        fail("files must be empty if manifest_path_map is provided.")
    if manifest_path_map and (manifest_name or manifest_full_name):
        fail("manifest_name or manifest_full_name parameter should not be specied when using manifest_path_map.")
    if not (manifest_path_map or files):
        fail("You must specify either manifest_path_map or files.")

    testonly = kwargs.pop("testonly", None)

    if not manifest_path_map:
        # By default we need automatically generate a manifest
        # implementation is via manifest_path_map interface
        if manifest_full_name:
            manifest = ":.manifest_{}/{}".format(name, manifest_full_name)
        else:
            manifest = ":.manifest_{}/{}manifest.dat".format(name, manifest_name)

        manifest_sign(
            name = manifest[1:],
            testonly = testonly,
            srcs = files,
            legacy = legacy_manifest_sign,
            target_compatible_with = target_compatible_with,
            **kwargs
        )
        manifest_path_map = {manifest: ""}

    version = ":{}_versioned_spv".format(name)
    expand_version(
        name = version[1:],
        testonly = testonly,
        deps = manifest_path_map.keys(),
        token = spv_version_token,
        base_version_str = base_version_str,
        component_name = versioning_component_name,
        template_file = spv_template,
        target_compatible_with = target_compatible_with,
        **kwargs
    )

    sdds_impt = ":{}/SDDS-Import.xml".format(name)
    sdds_import(
        name = sdds_impt[1:],
        testonly = testonly,
        manifest_path_map = manifest_path_map,
        spv_file = version,
        target_compatible_with = target_compatible_with,
        **kwargs
    )

    if not license_path:
        license_path = "{}/thirdparty_licenses.txt".format(name)

    if not copyright_notices_path:
        copyright_notices_path = "{}/copyright_notices.txt".format(name)

    check_license(
        name = "{}_thirdparty_licenses".format(name),
        testonly = testonly,
        check_conditions = False,
        license_texts = license_path,
        copyright_notices = copyright_notices_path,
        report = "{}_thirdparty_licenses_report.txt".format(name),
        deps = manifest_path_map.keys(),
        target_compatible_with = target_compatible_with,
    )

    sdds2 = ":{}_sdds2".format(name)

    zip_asset(
        name = sdds2[1:],
        testonly = testonly,
        srcs = [sdds_impt, license_path],
        strip_prefix = strip_prefix.from_zip_pkg(name),
        target_compatible_with = target_compatible_with,
        visibility = ["//visibility:public"],
        **kwargs
    )

    native.filegroup(
        name = "{}.packages".format(name),
        testonly = testonly,
        srcs = [
            sdds2,
        ],
        target_compatible_with = target_compatible_with,
        visibility = ["//visibility:public"],
        **kwargs
    )
