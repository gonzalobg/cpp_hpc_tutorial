C++ HPC Tutorial
===

## Instructions

Install [HPCCM] if you want to build the containers locally:

```
pip3 install --user hpccm

# And add it to the path
PATH=$PYTHONPATH:$PATH
```

To build the container and start the Jupter notebook webserver locally here are the instructions for `Docker` and `Singularity`.

### Docker

```shell
./ci/run docker-build
./ci/run docker-serve
```

### Singularity

```shell
./ci/run singularity-build
./ci/run singularity-serve
```
