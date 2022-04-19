C++ HPC Tutorial
===

## Instructions

Install [HPCCM] if you want to build the containers locally:

```
pip3 install --user hpccm

# And add it to the path
PATH=$PYTHONPATH:$PATH
```

### Docker

```shell
./ci/run docker-build
./ci/run docker-serve-dev
```

### Singularity

```shell
./ci/run singularity-build
./ci/run singularity-serve-dev
```
