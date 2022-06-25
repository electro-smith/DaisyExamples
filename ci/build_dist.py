#!/usr/bin/env python
#
# Scans list of directories provided for examples containing a binary file,
# and compiles a dist/ folder containing a matching hierarchy of binary files
# and some metadata associated with them.
#
# This is primarily for providing easy access to pre-compiled examples for the compiler
# Web Programmer application
#
# Examples must be compiled locally (or in the cloud service running this), otherwise
# there won't be any binary files to grab from, and the script will skip over everything.
#
##############################
# Script Begin ###############
##############################
import argparse
import os
import shutil
import json
import glob


class Example(object):
    def __init__(self, name, ogdir):
        self.name = name
        self.description = "no desc available yet"
        self.url = 'https://raw.githubusercontent.com/electro-smith/'
        self.platform = ogdir.replace(os.path.sep, '_')
        self.url += 'DaisyExamples/master/' + ogdir.replace(os.path.sep, '/') + '/' + self.name + '/README.md'
        self.apath = os.path.abspath('/'.join((ogdir, name)))
        flist = glob.glob('{}/build/*.bin'.format(self.apath))
        if len(flist) > 0:
            self.buildpath = flist[0]
        else:
            self.buildpath = None
        self.destpath = './dist/{}/{}.bin'.format(self.platform, self.name)

    def Valid(self):
        if self.buildpath is not None:
            return True
        else:
            return False

    # packs necessary data and returns json object
    def DumpObject(self):
        myobj = {}
        myobj['name'] = self.name
        myobj['platform'] = self.platform
        myobj['filepath'] = self.destpath
        myobj['description'] = self.description
        myobj['url'] = self.url
        return myobj

    def DumpJson(self, filepointer):
        myobj = self.DumpObject()
        return json.dump(myobj, filepointer)

    def CopyToDeploy(self):
        if not os.path.isdir('./dist'):
            os.mkdir('./dist')
        if not os.path.isdir(os.path.dirname(self.destpath)):
            os.mkdir(os.path.dirname(self.destpath))
        if self.buildpath is not None:
            shutil.copy(self.buildpath, self.destpath)


def run():
    # Parse arguments
    parser = argparse.ArgumentParser(
        description='generates the dist/ directory, containing binaries for all examples, and a json file containing simple metadata for each example.')
    parser.add_argument('directory_list', nargs='*',
                        help='list of directories separated by spaces to use as inputs for the dist folder')
    parser.add_argument('-e', '--exclude_list', nargs='*',
                        help='list of directories separated by spaces to ignore searching')
    parser.add_argument('-r', '--rewrite', action='store_true',
                        help='When set, this will cause the script to completely clear the dist/ directory before executing.')
    parser.add_argument('-u', '--human-readable', action='store_true',
                        help='When set, this will use indentation in the json output. By default the output will be a single line of text.')
    args = parser.parse_args()

    if not args.exclude_list:
        filter_dirs = ["libDaisy",
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
    else:
        filter_dirs = args.exclude_list

    # Prior to novemenber 2021 this would be a default-list, and was required.
    if not args.directory_list:
        # directories = [ 'seed', 'pod', 'patch', 'field', 'petal', 'versio', 'legio', 'patch_sm' ]
        directories = list(
            filter(lambda x: x not in filter_dirs and os.path.isdir(x), os.listdir('.')))
    else:
        directories = list(args.directory_list)

    # navigate directories, and create examples to add to JSON, and dist/
    olist = []
    for d in directories:
        for root, dirs, files in os.walk(d):
            if ('Makefile') in files:
                ex_name = os.path.basename(root)
                ex_path = os.path.normpath(os.path.join(root, '..'))
                newobj = Example(ex_name, ex_path)
                olist.append(newobj)

    if args.rewrite and os.path.isdir('./dist'):
        shutil.rmtree('./dist')

    # Creating New Build Dir
    for example in olist:
        example.CopyToDeploy()
    jsonout = list(example.DumpObject() for example in olist if example.Valid())

    # Creating JSON file
    with open('./dist/examples.json', 'w') as f:
        if args.human_readable:
            json.dump(jsonout, f, indent=4)
        else:
            json.dump(jsonout, f)

if __name__ == '__main__':
    run()

