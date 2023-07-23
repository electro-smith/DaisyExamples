#!/usr/bin/env python
import argparse
import glob
import os
import shutil
import codecs
import pathlib
import subprocess

################################################################
# String Data for clarity
################################################################

description = 'Helper script for managing Daisy examples, and creating new projects.'
usage = {
    'operation': 'Keyword argument for the desired helper action. Can be any of the following: create, copy, update, rebuild_all.',
    'destination': 'Second positional argument to set where the action should be applied. This is the final destination of the project.',
    'source': 'optional argument for selecting the project to copy from. required for the copy operation.',
    'board': 'optional argument for selecting the template when using the create operation. Default is seed. Options are: seed, field, patch, petal, pod, versio, legio, patch_sm',
    'libs': 'optional argument for specifying the path containing libDaisy and DaisySP. Used with create and update. Default is ./ .',
    'include_vgdb': 'optional flag for including debug files for Visual Studio and the VisualGDB extension. These are not included by default.'
}
supported_boards = ['seed', 'pod', 'patch',
    'field', 'petal', 'versio', 'legio', 'patch_sm']

# Dirs to be ignored by update, and other global commands
global_filter_dirs = ["libDaisy",
                      "DaisySP",
                      ".github",
                      ".vscode",
                      ".git",
                      "ci",
                      "cube",
                      "dist",
                      "utils",
                      "stmlib",
                      "libdaisy",
                      "MyProjects"]

################################################################
# Helper functions
################################################################

# Opens file at filepath,
# creates a temp file where all instances of string "a"
# are replaced with "b", and then
# removes the original file, and renames the temp file.
def rewrite_replace(filepath, a, b):
    try:
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
    except Exception as e:
        print("Cannot process file: {} \t {} into {}".format(filepath, a, b))
        print(e)

def rec_rewrite_replace(dir_path, a, b):
    for dname, dirs, files in os.walk(dir_path):
        for fname in files:
            fpath = os.path.join(dname, fname)
            rewrite_replace(fpath, a, b)

# Takes a filename and a list of extensions
# returns true if the file name ends with that extension
def has_extension(fname, ext_list):
    f, ext = os.path.splitext(fname)
    if ext in ext_list:
        return True
    else:
        return False


################################################################
# BEGIN OPERATIONS
################################################################

# Called via the 'update' operation
# Removes old, copies new from template, processes for string replacement
# only affects .vscode/, vs/ and .sln files.
def update_project(destination, libs, include_vs=False):
    basedir = os.path.abspath(destination)
    root = os.path.dirname(os.path.realpath(__file__))
    tdir = os.path.sep.join((root, 'utils', 'Template'))
    if not os.path.isdir(basedir):
        print('destination should be a directory')
        return
    proj_name = os.path.basename(basedir)
    print('updating {}...'.format(proj_name))
    # Removalsd
    # Maybe make this optional..
    rm_patts = ['*.sln', '*.vcxproj', '*.vcxproj*', '*.vgdbsettings*', '.vscode/*', 'vs/*', 'stm32.props', 'stm32.xml']
    rmlists = list(glob.glob(basedir+os.path.sep+pat) for pat in rm_patts)
    f_to_rm = list(item for sublist in rmlists for item in sublist)
    if len(f_to_rm) > 0:
        for f in f_to_rm:
            print('deleting: {}'.format(os.path.relpath(f)))
            if os.path.isdir(f):
                shutil.rmtree(f)
            else:
                os.remove(f)
    # Copying
    libs = pathlib.Path(libs).as_posix()
    cp_patts = ['.vscode/*']
    if include_vs:
        cp_patts.extend(['*.sln', '*.vgdbsettings', 'vs/*'])
    cplists = list(glob.glob(tdir+os.path.sep+pat) for pat in cp_patts)
    f_to_cp = list(item for sublist in cplists for item in sublist)
    libs = pathlib.Path(os.path.relpath(libs, destination)).as_posix()
    for f in f_to_cp:
        sname = os.path.abspath(f)
        dname = os.path.abspath(sname.replace(tdir, basedir)).replace('Template', proj_name)
        dir_path = os.path.dirname(dname)
        print('copying: {} to {}'.format(
            os.path.relpath(sname), os.path.relpath(dname)))
        if not os.path.isdir(dir_path):
            os.mkdir(dir_path)
        if os.path.isdir(sname):
            shutil.copytree(sname, dname)
        else:
            shutil.copyfile(sname, dname)
            rewrite_replace(dname, 'Template', proj_name)
            rewrite_replace(dname, '@LIBDAISY_DIR@', libs + '/libDaisy')
            rewrite_replace(dname, '@DAISYSP_DIR@', libs + '/DaisySP')


# Called via the 'copy' operation
# copies _all_ files from source directory,
# deletes old build directories.
def copy_project(destination, source, include_vs=False):
    print('copying {} to new project: {}'.format(source, destination))
    srcAbs = os.path.abspath(source)
    srcBase = os.path.basename(srcAbs)
    destAbs = os.path.abspath(destination)
    destBase = os.path.basename(destAbs)
    # First check if src is valid directory.
    if os.path.isdir(srcAbs):
        # Then make a copy of the folder renaming it to be the same
        if include_vs:
            shutil.copytree(srcAbs, destAbs, ignore = shutil.ignore_patterns('VisualGDB'))
        else:
            shutil.copytree(
                srcAbs, destAbs, ignore = shutil.ignore_patterns('vs', 'Template.sln'))
        # remove build/ and VisualGDB/ if necessary.
        exclude_dirs=['build', 'vs/VisualGDB', 'VisualGDB']
        bdirs=list(os.path.sep.join((destAbs, exclusion))
                   for exclusion in exclude_dirs)
        for d in bdirs:
            if os.path.isdir(d):
                print('deleting build files: {}', d)
                shutil.rmtree(d)
        # Go through and rename all the files first.
        found=glob.glob(destAbs + os.path.sep + '*' + \
                        os.path.sep + srcBase + '*', recursive = True)
        found += glob.glob(destAbs + os.path.sep + '*' + \
                           srcBase + '*', recursive = True)
        for f in found:
            s=os.path.abspath(f)
            d=os.path.abspath(f.replace(srcBase, destBase))
            os.rename(s, d)
        allFiles=glob.glob(destAbs + os.path.sep + '*', recursive = True)
        allFiles += glob.glob(destAbs + os.path.sep + '**' + os.path.sep + '*')
        allFiles += glob.glob(os.path.sep.join((destAbs, '.vscode', '*')))
        # remove unacceptable extensions
        avoid_exts=['.ai', '.png']
        procFiles=list(f for f in allFiles if not has_extension(f, avoid_exts))
        # process files with internal text replacement
        for f in procFiles:
            if not os.path.isdir(f) and os.path.exists(f):
                rewrite_replace(f, srcBase, destBase)
    else:
        print("source directory is not valid.")

# Called via the 'create' operation
def create_from_template(destination, board, libs, include_vs = False):
    print("creating new project: {} for platform: {}".format(destination, board))
    # Essentially need to:
    # * run copy_project on template and then rewrite the cpp file..

    libs=pathlib.Path(os.path.relpath(libs, destination)).as_posix()
    file_path=pathlib.Path(__file__).as_posix().replace('helper.py', '')

    template_dir=os.path.join(file_path, 'utils', 'Template')
    copy_project(destination, template_dir, include_vs)

    libdaisy_dir=libs + "/libDaisy/"
    rec_rewrite_replace(destination, "@LIBDAISY_DIR@", libdaisy_dir)

    daisysp_dir=libs + "/DaisySP/"
    rec_rewrite_replace(destination, "@DAISYSP_DIR@", daisysp_dir)

    src_file=pathlib.Path(destination + os.path.sep + \
                          os.path.basename(destination) + '.cpp')
    # Copy resources/diagram files (if available)
    dfiles=glob.glob(os.path.sep.join(('resources', '*')))
    dfiles += glob.glob(os.path.sep.join(('resources', '**', '*')))
    dfiles=list(f for f in dfiles if board in f)
    if len(dfiles) > 0:
        dsrc=list(os.path.abspath(f) for f in dfiles)
        ddest=list(os.path.abspath(os.path.sep.join((destination, f)))
                   for f in dfiles)
        # Make resources/ and resources/png/
        os.mkdir(os.path.sep.join((destination, 'resources')))
        os.mkdir(os.path.sep.join((destination, 'resources', 'png')))
        for s, d in zip(dsrc, ddest):
            shutil.copy(s, d)

    # Platform specific differences summarized:
    # - seed: needs hw.Configure() before init. No hw.UpdateAllControls()
    # - patch: four channels instead of two in audio callback.
    if board == 'seed':
        controls=False
    else:
        controls=True
    if board == 'patch':
        audio_channels=4
    else:
        audio_channels=2

    board_class_names={
        'seed': "DaisySeed",
        'field': "DaisyField",
        'pod': "DaisyPod",
        'patch': "DaisyPatch",
        'patch_sm': "DaisyPatchSM",
        'petal': "DaisyPetal",
        'versio': "DaisyVersio",
        'legio': "DaisyLegio"
    }
    # Rewrite  Source file
    with open(src_file, 'w') as f:
        f.write('#include "daisy_{}.h"\n'.format(board))
        f.write('#include "daisysp.h"\n\n') # extra line below
        f.write('using namespace daisy;\n')
        if board == 'patch_sm':
            f.write('using namespace patch_sm;\n') # extra line below
        f.write('using namespace daisysp;\n\n') # extra line below
        f.write('{} hw;\n\n'.format(board_class_names.get(board)))
        f.write('void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)\n')
        f.write('{\n')
        if controls:
            f.write('\thw.ProcessAllControls();\n')
        f.write('\tfor (size_t i = 0; i < size; i++)\n')
        f.write('\t{\n')
        if board == 'patch_sm':
            f.write('\t\tOUT_L[i] = IN_L[i];\n')
            f.write('\t\tOUT_R[i] = IN_R[i];\n')
        else:
            for i in range(0, audio_channels):
                f.write('\t\tout[{}][i] = in[{}][i];\n'.format(i, i))
        f.write('\t}\n')
        f.write('}\n\n') # extra line  before main
        f.write('int main(void)\n')
        f.write('{\n')
        f.write('\thw.Init();\n')
        # Include Audio Setting Options
        f.write('\thw.SetAudioBlockSize(4); // number of samples handled per callback\n')
        f.write('\thw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);\n')
        if board != "seed" and board != "patch_sm":
            f.write('\thw.StartAdc();\n')
        f.write('\thw.StartAudio(AudioCallback);\n')
        f.write('\twhile(1) {}\n')
        f.write('}\n')

def rebuild(destination):
    if destination:
        print("rebuild coming soon")
    else:
        print("rebuild all coming soon")
        # The below does not work as expected on windows.
        # Seems to use wsl's bash, and the default arm-none-eabi in that location
        # I'll look into solutions for Windows, though the below may work on other
        # platforms already.
        # -----------------
        # proc = subprocess.Popen(['bash', '-c', './ci/build_libs.sh'],
        #                 stdout=subprocess.PIPE,
        #                 stderr=subprocess.PIPE,
        #                 shell=False)
        # stdout, stderr = proc.communicate()
        # print(stdout.decode('utf-8'), stderr.decode('utf-8'))
        # proc = subprocess.Popen(['./ci/build_examples.sh'],
        #                 stdout=subprocess.PIPE,
        #                 stderr=subprocess.PIPE)
        # stdout, stderr = process.communicate()
        # print(stdout, stderr)


################################################################
# BEGIN SCRIPT
################################################################

def run():
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('operation', help=usage.get('operation'))
    parser.add_argument('destination', help =usage.get('destination'), nargs='?')
    parser.add_argument('-s', '--source', help=usage.get('source'))
    parser.add_argument('-b', '--board', help=usage.get('board'), default='seed', choices=supported_boards)
    parser.add_argument('-l', '--libs', help=usage.get('libs'), default='.')
    parser.add_argument('--include_vgdb', help=usage.get('include_vgdb'),  action='store_true')
    args = parser.parse_args()
    op = args.operation.casefold()
    if op == 'create':
        # create new project
        brd = args.board
        dest = args.destination
        libs = args.libs
        create_from_template(dest, brd, libs, args.include_vgdb)
    elif op == 'copy':
        # copy from source to dest
        src = args.source
        dest = args.destination
        copy_project(dest, src, args.include_vgdb)
    elif op == 'update':
        if args.destination:
            dest = args.destination
            libs = args.libs
            update_project(dest, libs, args.include_vgdb)
        else:
            dirs_to_search = list(
                filter(lambda x: x not in global_filter_dirs and os.path.isdir(x), os.listdir('.')))
            for dir in dirs_to_search:
                example_dirs = []
                for root, dirs, files in os.walk(dir):
                    if 'Makefile' in files:
                        example_dirs.append(root)
                for ex in example_dirs:
                    try:
                        update_project(ex, args.libs, args.include_vgdb)
                    except Exception as e:
                        print('Unable to update example: {}'.format(ex))
                        print(e)
    elif op == 'rebuild':
        rebuild(args.destination)
    else:
        print('invalid operation. please use one of the following:')
        print('create - creates a new project with the name \'destination\' for the specified board (or seed if not specified).')
        print('copy - duplicates an existing project to the specified \'destination\', replacing names and paths as necessary')
        print('update - updates debugging files and directories to latest in template. Won\'t change source files.')
        print('rebuild - rebuilds the libraries, and all examples, or a single example if destination is passed in.')
    print('done')

if __name__ == '__main__':
    run()
