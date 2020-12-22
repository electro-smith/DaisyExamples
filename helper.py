#!/usr/bin/env python
import argparse, glob, os, shutil, codecs
import subprocess

# Known Issues/bugs:
# Copy:
# - attempts to copy and process build/ and VisualGDB/. This fails on processing non text files.
#    - Temp solution: delete those folders, and re run.
# - launch.json is not processed. Presumably the .vscode folder doesn't show up in glob or something.

################################################################
# String Data for clarity
################################################################

description = 'Helper script for managing Daisy examples, and creating new projects.'
usage = {
    'operation': 'Keyword argument for the desired helper action. Can be any of the following: create, copy, update, rebuild_all.',
    'destination': 'Second positional argument to set where the action should be applied. This is the final destination of the project.',
    'source': 'optional argument for selecting the project to copy from. required for the copy operation.',
    'board': 'optional argument for selecting the template when using the create operation. Default is seed. Options are: seed, field, patch, petal, pod, versio'
}
supported_boards= ['seed', 'pod', 'patch', 'field', 'petal', 'versio']

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
# Removes old, copies new, processes for string replacement
def  update_project(destination):
    basedir = os.path.abspath(destination)
    root = os.path.dirname(os.path.realpath(__file__))
    tdir = os.path.sep.join((root,'utils','Template'))
    if not os.path.isdir(basedir):
        print('destination should be a directory')
        return
    proj_name = os.path.basename(basedir)
    print('updating {}...'.format(proj_name))
    # Removalsd
    # Maybe make this optional..
    rm_patts = ['*.sln', '*.vcxproj', '*.vgdbsettings', '.vscode/*', 'vs/*']
    rmlists = list(glob.glob(basedir+os.path.sep+pat) for pat in rm_patts)
    f_to_rm = list(item for sublist in rmlists for item in sublist)
    if len(f_to_rm) > 0:
        for f in f_to_rm:
            print('deleting: {}'.format(os.path.basename(f)))
            os.remove(f)
    # Copying
    cp_patts = ['*.sln', '*.vgdbsettings', '.vscode/*', 'vs/*']
    cplists = list(glob.glob(tdir+os.path.sep+pat) for pat in cp_patts)
    f_to_cp = list(item for sublist in cplists for item in sublist)
    for f in f_to_cp:
        sname = os.path.abspath(f)
        dname = os.path.abspath(f.replace(tdir, basedir))
        print('copying: {} '.format(os.path.basename(f)))
        #os.rename(sname, dname)
        shutil.copyfile(sname, dname)
        # process file within this loop
        rewrite_replace(dname, 'Template', proj_name) 
        # Process again for libdaisy dir if/when available
    print("done")

# Called via the 'copy' operation
def copy_project(destination, source):
    print('copying {} to new project: {}'.format(source, destination))
    srcAbs = os.path.abspath(source)
    srcBase = os.path.basename(srcAbs)
    destAbs = os.path.abspath(destination)
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
    else:
        print("source directory is not valid.")

# Called via the 'create' operation
def create_from_template(destination, board):
    print("creating new project: {} for platform: {}".format(destination, board))
    # Essentially need to:
    # * run copy_project on template and then rewrite the cpp file..
    #template_dir = os.path.abspath(['utils','Template'].join(os.path.sep))
    template_dir = os.path.abspath(os.path.sep.join(('utils', 'Template')))
    copy_project(destination, template_dir)
    src_file = os.path.abspath(destination + os.path.sep + os.path.basename(destination) + '.cpp')
    # Platform specific differences summarized:
    # - seed: needs hw.Configure() before init. No hw.UpdateAllControls()
    # - patch: four channels instead of two in audio callback.
    if board == 'seed':
        need_configure = True
        controls = False
    else:
        need_configure = False
        controls = True
    if board == 'patch':
        audio_channels = 4
    else:
        audio_channels  = 2
    # Rewrite  Source file
    with open(src_file, 'w') as f:
        f.write('#include "daisy_{}.h"\n'.format(board))
        f.write('#include "daisysp.h"\n\n') # extra line below
        f.write('using namespace daisy;\n')
        f.write('using namespace daisysp;\n\n') # extra line below
        f.write('Daisy{} hw;\n'.format(board.capitalize()))
        f.write('void AudioCallback(float **in, float **out, size_t size)\n')
        f.write('{\n')
        if controls:
            f.write('\thw.ProcessAllControls();\n')
        f.write('\tfor (size_t i = 0; i < size; i++)\n')
        f.write('\t{\n')
        for i in range(0, audio_channels):
            f.write('\t\tout[{}][i] = in[{}][i];\n'.format(i, i))
        f.write('\t}\n')
        f.write('}\n\n') # extra line  before main
        f.write('int main(void)\n')
        f.write('{\n')
        if need_configure:
            f.write('\thw.Configure();\n')
        f.write('\thw.Init();\n')
        f.write('\thw.StartAdc();\n')
        f.write('\thw.StartAudio(AudioCallback);\n')
        f.write('\twhile(1) {}\n')
        f.write('}\n')
    print("done")

def rebuild(destination):
    if destination:
        print("rebuild coming soon")
    else:
        print("rebuild all coming soon")
    # proc = subprocess.Popen(['./ci/build_libs.sh'],
    #                 stdout=subprocess.PIPE,
    #                 stderr=subprocess.PIPE)
    # stdout, stderr = process.communicate()
    # print(stdout, stderr)
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
    args = parser.parse_args()
    op = args.operation.casefold()
    if op == 'create':
        # create new project
        brd = args.board
        dest = args.destination
        create_from_template(dest, brd)
    elif op == 'copy':
        # copy from source to dest
        src = args.source
        dest = args.destination
        copy_project(dest, src)
    elif op == 'update':
        if args.destination:
            dest = args.destination
            update_project(dest)
        else:
            print('Update all support coming soon...')
    elif op == 'rebuild':
        rebuild(args.destination)
    else:
        print('invalid operation. please use one of the following:')
        print('create - creates a new project with the name \'destination\' for the specified board (or seed if not specified).')
        print('copy - duplicates an existing project to the specified \'destination\', replacing names and paths as necessary')
        print('update - updates debugging files and directories to latest in template. Won\'t change source files.')
        print('rebuild - rebuilds the libraries, and all examples, or a single example if destination is passed in.')

if __name__ == '__main__':
    run()
