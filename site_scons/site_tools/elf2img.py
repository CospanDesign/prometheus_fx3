"""SConst.Tool.xst

Tool-specific elf 2 img

"""
import SCons.Action
import SCons.Builder
import SCons.Util
import SCons.Tool

import os
import sys

import utils
import elf2img_utils

def _elf2img_emitter(target, source, env):
    return target, source

_elf2img_builder = SCons.Builder.Builder(
        action = SCons.Action.Action("$ELF2IMG_COMSTR"),
        suffix = ".img",
        src_suffix = ".elf",
        emitter = _elf2img_emitter)

class ELF2IMGBuilderWarning(SCons.Warnings.Warning):
    pass

class ELF2IMGBuilderError(ELF2IMGBuilderWarning):
    pass

def _detect(env):
    try:
        return env["ELF2IMG_COMMAND"]
    except KeyError:
        pass

    elf2img_command = elf2img_utils.find_elf2img(env)
    if elf2img_command:
        return elf2img_command
    raise SCons.Errors.StopError(
            ELF2IMGBuilderError,
            "Could not find Arm Assember")

def generate(env):
    env["ELF2IMG_COMMAND"] = _detect(env)
    #elf2img_outfile = elf2img_utils.get_elf2img_outfile(env)
    
    env.SetDefault(
            ELF2IMG_FLAGS = "",
            ELF2IMG_COMSTR = "$ELF2IMG_COMMAND $ELF2IMG_FLAGS -i $SOURCES -o $TARGETS")
    env.AddMethod(_elf2img_builder, "elf2img")
    return None

def exists(env):
    return _detect(env)


