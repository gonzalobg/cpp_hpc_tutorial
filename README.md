C++ HPC Tutorial
===

## Instructions

### Pre-requisites

To build the container locally, a properly configured container runtime is required. 
Both Docker and Singularity are supported.

Building the container requires the Docker or Singularity container descriptions.
This project uses [HPC Container Maker] to generate these descriptions from a single portable container description.
[HPPCM] is a Python application.
Running it requires Python, and it can be installed using Python's package manager `pip`:

```
pip3 install --user hpccm
```

After installation, the `hpccm` binary should be on your path:

```
hpccm --version
```

If this fails, check that the path at which `pip` installs python binaries, usually `PYTHONPATH` is in your `PATH`:

```shell
# You can add it to your PATH as follows:
PATH=$PATH:$PYTHONPATH
```

### Building container and serving Jupyter Notebooks

To build the container and start the Jupter notebook webserver locally here are the instructions for `Docker` and `Singularity`.

[HPCCM]:

#### Docker

```shell
./ci/run docker-build
./ci/run docker-serve
```

#### Singularity

```shell
./ci/run singularity-build
./ci/run singularity-serve
```
