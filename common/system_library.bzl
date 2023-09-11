# Copyright 2023 Sophos Limited. All rights reserved.

"""
Effectively makes a cc_library that has a dynamic library as a src use the system equivalent at runtime.
The dynamic library's filename should be the same as that of the system library.

More technically, it changes the CcInfo provider such that it leaves compilation_context the same, but changes
linking_context to remove the library from 'libraries' (otherwise Bazel will try to link the input at runtime),
and instead links it manually using compiler flags.
This is GCC specific but could potentially be made cross-platform using cc_common.create_link_variables
"""

def _system_library_impl(ctx):
    previous_linker_inputs = ctx.attr.src[CcInfo].linking_context.linker_inputs.to_list()
    linking_context = None
    for previous_linker_input in previous_linker_inputs:
        if len(previous_linker_input.libraries) != 1:
            continue

        dynamic_library = previous_linker_input.libraries[0].dynamic_library
        user_link_flags = ["-L{}".format(dynamic_library.dirname), "-l:{}".format(dynamic_library.basename)]
        additional_inputs = depset([dynamic_library])
        linker_input = cc_common.create_linker_input(owner = previous_linker_input.owner, user_link_flags = user_link_flags, additional_inputs = additional_inputs)
        linking_context = cc_common.create_linking_context(linker_inputs = depset([linker_input]))

    if not linking_context:
        fail("Invalid library input, must have exactly one dynamic library as a dependency")

    return [CcInfo(linking_context = linking_context, compilation_context = ctx.attr.src[CcInfo].compilation_context)]

system_library = rule(
    _system_library_impl,
    attrs = {
        "src": attr.label(providers = [CcInfo]),
    },
)
