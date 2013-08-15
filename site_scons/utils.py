#Distributed under the MIT licesnse.
#Copyright (c) 2013 Cospan Design (dave.mccoy@cospandesign.com)

#Permission is hereby granted, free of charge, to any person obtaining a copy of
#this software and associated documentation files (the "Software"), to deal in
#the Software without restriction, including without limitation the rights to
#use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
#of the Software, and to permit persons to whom the Software is furnished to do
#so, subject to the following conditions:
#
#The above copyright notice and this permission notice shall be included in all
#copies or substantial portions of the Software.
#
#THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#SOFTWARE.

import os
import sys
import json
import glob
import subprocess

PROJECT_BASE = os.path.abspath(
                    os.path.join(os.path.dirname(__file__), os.pardir))

DEFAULT_BUILD_DIR = "build"
DEFAULT_CONFIG_FILE = "config.json"

class ConfigurationError(Exception):
    """
    Errors associated with configuration:
        getting the configuration file for the project
        getting the default FX3 tool location
    """
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)


#def initialize_environment(env):
#    """
#    Initialize the Build environment
#
#    Args:
#        env (Scons.Envrionment): An environment to add all the tools too
#
#    Returns:
#        Nothing
#
#    Raises:
#        Nothing
#    """
#    #Get the configuration dictionary
#    config = read_config(env)
#
#    #Read where the ARM Toolchain is located
#    build_path = config["compiler_path"]
#    #Read where the elf2img is located
#    img2elf_path = config["img2elf_path"]
#    #Read where the upload tool is located
#    upload_tool = config["upload_tool"]
#
#    #Verify that the build tool actually cotains the correct tools
#    tool_base = ""
#    tool_prefix = ""
#    for base, dirs, files in os.walk(build_path):
#        for f in files:
#            if f.startswith("arm") and f.endswith("gcc"):
#                #print "Found: %s" % os.path.join(base, f)
#                tool_prefix = f.partition("gcc")[0]
#                tool_base = base
#                break
#
#    if not os.path.isfile(img2elf_path):
#        for base, dirs, files in os.walk(img2elf_path):
#            for f in files:
#                if f == "elf2img":
#                    img2elf_path = os.path.join(base, f)
#
#    #print "Found: %s" % img2elf_path
#
#    if not os.path.isfile(img2elf_path):
#        for base, dirs, files in os.walk(upload_tool):
#            for f in files:
#                if f == "cyusb_linux":
#                    upload_tool = os.path.join(base, f)
#
#    #print "Found: %s" % upload_tool
#    #Adjust the compiler prefix
#    compiler = os.path.join(tool_base, "%s%s" % (tool_prefix, "gcc"))
#    env["CC"] = compiler

def get_c_compiler_path(env):
    config = read_config(env)
    build_path = config["compiler_path"]
    #Verify that the build tool actually cotains the correct tools
    tool_base = ""
    tool_prefix = ""
    for base, dirs, files in os.walk(build_path):
        for f in files:
            if f.startswith("arm") and f.endswith("gcc"):
                #print "Found: %s" % os.path.join(base, f)
                tool_prefix = f.partition("gcc")[0]
                tool_base = base
                break

    compiler_path = os.path.join(tool_base, "%s%s" % (tool_prefix, "gcc"))
    return compiler_path

def get_cxx_compiler_path(env):
    config = read_config(env)
    build_path = config["compiler_path"]
    #Verify that the build tool actually cotains the correct tools
    tool_base = ""
    tool_prefix = ""
    for base, dirs, files in os.walk(build_path):
        for f in files:
            if f.startswith("arm") and f.endswith("g++"):
                #print "Found: %s" % os.path.join(base, f)
                tool_prefix = f.partition("g++")[0]
                tool_base = base
                break

    compiler_path = os.path.join(tool_base, "%s%s" % (tool_prefix, "g++"))
    return compiler_path


def get_compiler_version(env):
    print "get compiler version"
    compiler_path = get_c_compiler_path(env)
    version = subprocess.check_output([compiler_path, "-dumpversion"]).strip()
    return version

def get_include_paths(env):
    config = read_config(env)
    return config["include"]

def get_library_paths(env):
    config = read_config(env)
    return config["library_paths"]

def get_libraries(env):
    config = read_config(env)
    return config["libraries"]

def get_linker_script(env):
    config = read_config(env)
    return config["linker_script"]

def _resolve_paths(path_dict, file_types):
    """
    Reads in a path dictionary and returns a list of source files

    Args:
        path_dict (dictionary): a dictionary containing a relative or absolute
        reference to the source files that will be used in the build and,
        depending on the modifier, will return a list of absolute paths for
        system to build
        file_types (list of extenstions): a list of file extensions to search
        for
        

    Returns
        (list of strings): A list of paths to be built

    Raises:
        Nothing
    """
    paths = []
   
    
    for entry in path_dict:
        path = entry["path"]
        if not os.path.isabs(path):
            path = os.path.join(get_project_base(), path)

        #print "Working with Path %s" % path
        if not os.path.exists(path):
            raise ConfigurationError(
            "Path (%s) "\
            " doesn't point to an actual directory or file" % path)

        if os.path.isfile(path):
            #Found a file
            if path not in paths:
                paths.append(path)
                continue

        #This is a directory
        if "recursive" in entry.keys() and entry["recursive"]:
            #print "Recursive Search"
            fs = []
            for base, dirs, files in os.walk(path):
                for ft in file_types:
                    for f in files:
                        if f.endswith(ft):
                            fs.append(os.path.join(base, f))
            for f in fs:
                if f not in paths:
                    #print "Appending File: %s" % f
                    paths.append(f)

        else:
            #print "Just a directory"
            fs = []
            for ft in file_types:
                search_pattern = os.path.join(path, "*.%s" % ft)
                fs.append(glob.glob(search_pattern))
            for f in fs:
                if f not in paths:
                    paths.append(f)

    #print "Paths: %s" % str(paths)
    return paths


def get_c_sources(env):
    """
    Returns a list of c sources

    Args:
        env (Scons.Envrionment): An environment to add all the tools too

    Returns:
        (list of paths): paths to be fed into the compiler

    Raises:
        Nothing
    """
    config = read_config(env)
    c_sources = []
    for src in config["source"]:
        if src.endswith("c"):
            c_sources.append(src)
    return c_sources

def get_cpp_sources(env):
    config = read_config(env)
    c_sources = []
    #print "sourcs: %s" % str(config["source"])
    for src in config["source"]:
        if src.endswith("cpp"):
            c_sources.append(src)
    return c_sources

def get_all_sources(env):
    config = read_config(env)
    all_sources = config["source"]
    all_sources.extend(config["assembly"])
    return config["source"]

def get_assembly_paths(env):
    config = read_config(env)
    return config["assembly"]

def get_project_name(env):
    """
    Returns the name of the project that the scons will output

    Args:
        env (Scons.Environment): An environment to add all the tools too

    Returns:
        (string): name of the output (without an extension)

    Raises:
        Nothing
    """
    config = read_config(env)
    return config["name"]

def get_project_output_target(env):
    """
    Returns a path to the final build target output

    Args:
        env (Scons.Environment): An environment to add all the tools too

    Returns:
        (string): name of the output with the build path

    Raises:
        Nothing

    """
    config = read_config(env)
    create_build_directory(config)
    build_dir = get_build_directory(env, True)
    target_name = config["name"]
    return os.path.join(build_dir, "%s.img" % target_name)


def get_elf_target(env):
    """
    Returns a path to the elf build target output

    Args:
        env (Scons.Environment): An environment to add all the tools too

    Returns:
        (string): name of the output with the build path

    Raises:
        Nothing

    """
    config = read_config(env)
    create_build_directory(config)
    build_dir = get_build_directory(env, True)
    target_name = config["name"]
    return os.path.join(build_dir, "%s.elf" % target_name)



def get_project_base():
    """
    Returns a path to the base of this project

    Args:
        Nothing

    Return:
        (string): Path to the base of the project

    Raises:
        Nothing
    """
    return PROJECT_BASE

def read_config(env):
    """
    Read the build configuration file and creates a dictionary to be used
    throughout the project

    Args:
        env: Environmental variable where the config file is

    Return:
        Dictionary of the configuration

    Raises:
        ConfigurationError
    """

    config = {}
    #Open the configuration file
    fn = env['CONFIG_FILE']
    if not os.path.exists(fn):
        #if the configuration file name doesn't exists then
        #maybe it is at the base directory of the project
        fn = os.path.join(get_project_base(), fn)


    #if fn == DEFAULT_CONFIG_FILE:
    #    fn = os.path.join(PROJECT_BASE,
    #                      DEFAULT_CONFIG_FILE)
    try:
        config = json.load(open(fn, "r"))
    except TypeError as err:
        raise ConfigurationError(
                "Error parsing json file: %s" % str(err))

    except ValueError as err:
        raise ConfigurationError(
                "Error parsing json file: %s" % str(err))

    except IOError as err:
        raise ConfigurationError(
                "Error openning file: %s: Err: %s" % (fn, str(err)))

    source_paths = []
    include_paths = []
    libaries = []
    assembly_paths = []
    if "source" in config.keys():
        config["source"] = _resolve_paths(config["source"], file_types = ["c", "cpp", "cxx", "c++"])
    #if "libraries" in config.keys():
    #    config["libraries"] = _resolve_paths(config["libraries"], file_types = ["a"])
    if "assembly" in config.keys():
        config["assembly"] = _resolve_paths(config["assembly"], file_types = ["S", "s"])

    return config

def create_build_directory(config):
    """
    Reads in a config dictionary and creates a output build directory

    Args:
        config: Config dictionary

    Return:
        Nothing

    Raises:
        Nothing
    """
    build_dir = DEFAULT_BUILD_DIR
    if "build_dir" in config.keys():
        build_dir = config["build_dir"]

    build_dir = os.path.join(get_project_base(), build_dir)
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    return build_dir

def get_build_directory(env, absolute = False):
    """Returns the project output directory location

    Args:
        env (Scons.Envrionment): An environment to add all the tools too
        absolute (boolean):
            False (default): Relative to project base
            True: Absolute

    Returns:
        (string): strin representation of the path to the output

    Raises:
        Nothing
    """
    config = read_config(env)
    build_dir = DEFAULT_BUILD_DIR
    if "build_dir" in config.keys():
        build_dir = config["build_dir"]

    if absolute:
        build_dir = os.path.join(get_project_base(), build_dir)

    return build_dir

def get_paths(env):
    config = read_config(env)
    return config["paths"]

def get_scatter(env):
    config = read_config(env)
    return config["scatter"]
