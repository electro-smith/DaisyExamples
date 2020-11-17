# Formats a .json file for the Programmer webapp to have some info for the 
# precompiled examples
#
# Secondarily, this also packs up a "bin/" directory with all precompiled 
# examples
#
# TODO:
# - Check for an ".example.cfg" file in each example directory
#    this can contain Description, as well as breadboard images, etc.
#
# Data we want:
# [
#   {
#       "name": "Name"
#       "platform": "seed"
#       "filepath": "./bin/platform/Name.bin"
#       "description": "Short Description"
#       "url" : "raw.githubusercontent.com/electro-smith.../README.md"
#   },
#   {
#       etc. . . 
#   },
# ]
#
##############################
# Script Begin ###############
##############################
import os
import shutil
import json
import glob

class Example(object):
    def __init__(self, name, ogdir):
        self.name = name
        self.description = "no desc available yet"
        self.url = 'https://raw.githubusercontent.com/electro-smith/'
        # Special case for DaisySP examples
        if 'DaisySP' in ogdir:
            self.platform = 'seed'
            self.url += 'DaisySP/master/examples/' + self.name + '/README.md'
        else:
            self.platform = ogdir
            self.url += 'DaisyExamples/master/' + self.platform + '/' + self.name + '/README.md'
        self.apath = os.path.abspath('/'.join((ogdir,name)))
        flist = glob.glob('{}/build/*.bin'.format(self.apath))
        if len(flist) > 0:
            self.buildpath = flist[0]
        else:
            self.buildpath = None
        self.destpath = './bin/{}/{}.bin'.format(self.platform, self.name)

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
        if not os.path.isdir('./bin'):
            os.mkdir('./bin')
        if not os.path.isdir(os.path.dirname(self.destpath)):
            os.mkdir(os.path.dirname(self.destpath))
        if self.buildpath is not None:
            shutil.copy(self.buildpath, self.destpath)


def run():
    directories = [ 'DaisySP/examples', 'seed', 'pod', 'patch', 'field', 'petal', 'versio' ]
    examples = {}

    # Scan directories for example projects
    for d in directories:
        examples[d] = list(o for o in os.listdir(os.path.abspath(d)) 
            if os.path.isdir(os.path.sep.join((d,o))))
    olist = []
    for ex_key in examples:
        for ex in examples[ex_key]:
            newobj = Example(ex, ex_key)
            olist.append(newobj)

    # Creating New Build Dir
    for example in olist:
        example.CopyToDeploy()
    jsonout = list(example.DumpObject() for example in olist if example.Valid())
    
    # Creating JSON file
    with open('./bin/examples.json', 'w') as f:
        json.dump(jsonout, f)

if __name__ == '__main__':
    run()

