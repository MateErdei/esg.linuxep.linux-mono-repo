# Copyright 2023 Sophos Limited. All rights reserved.
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

def _system_library_impl(ctx):
    previous_linker_inputs = ctx.attr.src[CcInfo].linking_context.linker_inputs.to_list()
    linking_context = None
    linker_inputs = []
    for previous_linker_input in previous_linker_inputs:
        has_static = False
        for library in previous_linker_input.libraries:
            if library.objects or library.pic_objects or library.pic_static_library or library.static_library:
                has_static = True
                break
        if not has_static:
            linker_inputs.append(previous_linker_input)

    linking_context = cc_common.create_linking_context(linker_inputs = depset(linker_inputs))
    return [CcInfo(linking_context = linking_context, compilation_context = ctx.attr.src[CcInfo].compilation_context)]

system_library = rule(
    _system_library_impl,
    attrs = {
        "src": attr.label(providers = [CcInfo]),
    },
)

def shared_library_helper(name, deps, visibility = None):
    native.cc_shared_library(
        name = name + "_shared",
        shared_lib_name = "lib{}.so".format(name),
        deps = deps,
        visibility = visibility,
    )

    cc_library(
        name = "_{}_combined".format(name),
        deps = deps,
    )

    system_library(
        name = name + "_headers",
        src = ":_{}_combined".format(name),
    )

    cc_library(
        name = name,
        srcs = [
            ":{}_shared".format(name),
            #            "lib{}.so".format(name),
        ],
        deps = [
            ":" + name + "_headers",
        ],
        visibility = visibility,
    )
