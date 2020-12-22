# Utilities

Various scripts, etc. for helpful stuff within the Daisy ecosystem.

## copy_project.py

Duplicates existing project and replaces text/paths as necessary.

`copy_project.py` has been removed, and `helper.py copy` file in the root directory should be used instead.

See root level readme for usage.

## gen_json_details.py

Generates json metadata for examples.

This script has been removed and was replaced with `ci/build_dist.py` for use in automation.

## fix_style_examples.sh

This runs clang-format with the `-i` flag on the files checked by the CI style checks.

To run:

```sh
./util/fix_style_examples.sh
```

To run local check:

```sh
./ci/local_style_check.sh
```

The local check should return 1 empty line if all is successful.

## run-clang-format.py

common clang-format python interface. Used by CI and for running local checks.
