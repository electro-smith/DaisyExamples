#!/usr/bin/env python
import glob
import os
import shutil
import argparse
import fileinput
import codecs

# Opens file at filepath, 
# creates a temp file where all instances of string "a"
# are replaced with "b", and then 
# removes the original file, and renames the temp file.
def rewrite_replace(filepath, a, b):
    in_name = os.path.abspath(filepath)
    in_base = os.path.basename(in_name)
    t_name = in_name.replace(in_base, 'tmp_{}'.format(in_base))
    with codecs.open(in_name, 'r', encoding='utf-8') as fi, \
        codecs.open(t_name, 'w', encoding='utf-8') as fo:
        for line in fi:
            newline = line.replace(a, b)
            fo.write(newline)
    os.remove(in_name)
    os.rename(t_name, in_name)

# Takes a filename and a list of extensions
# returns true if the file name ends with that extension
def has_extension(fname, ext_list):
    f, ext = os.path.splitext(fname)
    if ext in ext_list:
        return True
    else:
        return False

## Main Script
parser = argparse.ArgumentParser(description='copies existing directory containing visual studio project, and replaces instances of the project name within all necessary files. The project filenames and directory name should match.')
parser.add_argument('source', help='directory containing the project to duplicate (often a template).')
parser.add_argument('destination', help='name of the target project (this text will replace the \'source\' text within all internal files.')
args = parser.parse_args()
src = args.source
dest = args.destination
# Get some helper variables for directory/file names.
srcAbs = os.path.abspath(src)
srcBase = os.path.basename(srcAbs)
destAbs = os.path.abspath(dest)
destBase = os.path.basename(destAbs)
# First check if src is valid directory.
if os.path.isdir(srcAbs):
    # Then make a copy of the folder renaming it to be the same
    shutil.copytree(srcAbs, destAbs)
    # Go through and rename all the files first.
    found = glob.glob(destAbs+ os.path.sep + '*' + os.path.sep + srcBase + '*', recursive=True)
    found += glob.glob(destAbs+ os.path.sep + '*' + srcBase + '*', recursive=True)
    for f in found:
        s = os.path.abspath(f)
        d = os.path.abspath(f.replace(srcBase, destBase))
        os.rename(s, d)
    allFiles = glob.glob(destAbs + os.path.sep + '*', recursive=True)
    allFiles += glob.glob(destAbs + os.path.sep + '**' + os.path.sep + '*')
    # remove unacceptable extensions
    avoid_exts = ['.ai', '.png']
    procFiles = list()
    for f in allFiles:
        if not has_extension(f, avoid_exts):
            procFiles.append(f)
    # process files with internal text replacement
    for f in procFiles:
        if not os.path.isdir(f) and os.path.exists(f):
            rewrite_replace(f, srcBase, destBase)
    print("done")
