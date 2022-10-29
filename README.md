C++ HPC Tutorial
===

## Instructions

### Pre-requisites

To build the container locally, a properly configured container runtime is required. 
Both Docker and Singularity containers are supported. 

The containers are generated from a single single description at [`/ci/recipe.py`](./ci/recipe.py) using the [HPC Container Maker][HPCCM] Python application, which requires a Python installation and can be installed with `pip`:

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

To build the container and start the Jupter notebook webserver locally here are the instructions for `Docker` and `Singularity`. The Jupyter notebook webserver provides an URL that can be used to connect to it from a webbrowser. When running it on a cluster, one might need to use SSH port forwarding to forward a local port to the compute node.

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
