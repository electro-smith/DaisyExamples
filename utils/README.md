# Utilities

Various scripts, etc. for helpful stuff within the Daisy ecosystem.

## copy_project.py

Copies an entire project folder to that of a new name.

requires: python >= 3.x

ex:
`python copy_project.py SourceProjectName DestinationProjectName`

### Known Issues

Using a common word when copying the project causes issues, when trying to do this rename as something else as an intermediary. 

i.e. changing MidiCCTest to MidiNoteTest you would:

```
copy_project.py MidiCCTest FooProj
copy_project.py FooProj MidiNoteTest
rm -r FooProj
```

this will be resolved soon.

