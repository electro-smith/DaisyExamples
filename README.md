# Daisy_Examples

**THIS REPO IS STILL BEING CONFIGURED AND SET UP, NOT ALL SCRIPTS, ETC WILL WORK YET**

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


# Things coming

Along with several more exciting examples, we'll add examples of other integrations and more here.

- gen~ examples with exported code and how to use that.
- pd export via hvcc examples.


