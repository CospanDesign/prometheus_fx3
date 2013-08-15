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

def _upload_emitter(target, source, env):
    target = 'upload_img'
    return target, source

_upload_builder = SCons.Builder.Builder(
        action = SCons.Action.Action("$UPLOAD_COMSTR"),
        src_suffix = ".img",
        emitter = _upload_emitter)

class UPLOADBuilderWarning(SCons.Warnings.Warning):
    pass

class UPLOADBuilderError(UPLOADBuilderWarning):
    pass

def _detect(env):
    try:
        return env["UPLOAD_COMMAND"]
    except KeyError:
        pass
    
    config = utils.read_config(env)
    upload_command = config["upload_tool"]
    
    if upload_command:
        return upload_command
    raise SCons.Errors.StopError(
            UPLOADBuilderError,
            "Could not find Arm Assember")

def generate(env):
    env["UPLOAD_COMMAND"] = _detect(env)
    config = utils.read_config(env)
    upload_flags = config["upload_flags"]
    
    env.SetDefault(
            UPLOAD_FLAGS = upload_flags,
            UPLOAD_COMSTR = "$UPLOAD_COMMAND $UPLOAD_FLAGS $SOURCES"
            )
    env.AddMethod(_upload_builder, "upload")
    return None

def exists(env):
    return _detect(env)

def upload(env, target, source):
    """
    A Pseudo-builder wrapper for the upload program
    """
    config = utils.read_config(env)

    _upload_builder.__call__(env, target, source)
    return "upload_img"

