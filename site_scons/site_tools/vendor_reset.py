"""SConst.Tool.xst

Tool-specific img 2 elf

"""
import SCons.Action
import SCons.Builder
import SCons.Util
import SCons.Tool

import os
import sys

import utils

def _vendor_reset_emitter(target, source, env):
    target = 'vendor_reset_img'
    return target, source

_vendor_reset_builder = SCons.Builder.Builder(
        action = SCons.Action.Action("$VENDOR_RESET_COMSTR"),
        emitter = _vendor_reset_emitter)

class VENDOR_RESETBuilderWarning(SCons.Warnings.Warning):
    pass

class VENDOR_RESETBuilderError(VENDOR_RESETBuilderWarning):
    pass

def _detect(env):
    try:
        return env["VENDOR_RESET_COMMAND"]
    except KeyError:
        pass
    
    config = utils.read_config(env)
    vendor_reset_command = config["upload_tool"]
    
    if vendor_reset_command:
        return vendor_reset_command
    raise SCons.Errors.StopError(
            VENDOR_RESETBuilderError,
            "Could not find Arm Assember")

def generate(env):
    env["VENDOR_RESET_COMMAND"] = _detect(env)
    config = utils.read_config(env)
    vidpid = config["target_vid_pid"]
    upload_tool = config["upload_tool"]
    
    env.SetDefault(
            VENDOR_RESET_TARGET = vidpid,
            VENDOR_RESET_COMSTR = "$VENDOR_RESET_COMMAND -vr $VENDOR_RESET_TARGET"
            )
    env.AddMethod(_vendor_reset_builder, "vendor_reset")
    return None

def exists(env):
    return _detect(env)

def vendor_reset(env, target, source):
    """
    A Pseudo-builder wrapper for the vendor_reset program
    """
    _vendor_reset_builder.__call__(env, target, source)
    return "vendor_reset_img"

