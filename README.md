# Daisy_Examples

If you are just getting started with Daisy, check out our [Getting Started Wiki page!](https://github.com/electro-smith/DaisyWiki/wiki/1.-Getting-Started)

This repo is home to a functional pipeline utilizing libDaisy and DaisySP libraries.

Examples are broken down by hardware platform.

Included as well are:
- Both libraries as git submodules
- Scripts for rebuildling the libraries as well as all examples.
- cube/ folder with .ioc files and example projects that use STM32CubeMX generated code via the STM32 HAL.

## Getting Started

First off there are a few ways to clone and initialize the repo (with its submodules).

You can do either of the following:

```
git clone --recursive https://github.com/Electro-Smith/Daisy_Examples
```

or 

```
git clone https://github.com/Electro-Smith/Daisy_Examples
git submodule update --init
```

## Updating the submodules

To pull everything for the repo and submodules:

```
git pull --recurse-submodules
```

to only pull changes for the submodules:

```
git submodule update --remote
```

Alternatively, you can simply run git commands from within the submodule and they will be treated as if you were in that repository instead of Daisy_Examples

## Testing Changes to the libraries

More documentation coming soon, but a common scenario would be one where a user is making changes to their forks of a library.

As the libraries' submodules are tracked to origin-master by default, there are a few ways to go about this.

1. You can checkout a branch from within the repo as normal, and test and work on the library as you would.
2. Each example has conditional setters in their Makefile for variables: `DAISYSP_DIR` and `LIBDAISY_DIR`. These can be set externally to a destination of the users choice.

There are some known issues when using Windows here. 

Our test environment for the Make builds using MINGW64 (via Git Bash). 

For the variables to work correctly in that environment you would have to set these paths to absolute paths using Windows syntax (i.e. `C:\Users\name\path\to\libdaisy`). Make internally converts this to a linux style path.








# Things coming

Along with several more exciting examples, we'll add examples of other integrations and more here.

- gen~ examples with exported code and how to use that.
- pd export via hvcc examples.


