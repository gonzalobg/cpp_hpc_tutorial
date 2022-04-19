"""
HPCCM deployment container for the C++ HPC tutorial
https://github.com/NVIDIA/hpc-container-maker/
"""

hpccm.include('recipe_dev.py')

Stage0 += copy(src='labs/', dest='/labs/')
Stage0 += workdir(directory='/labs/')
Stage0 += runscript(commands=[
    'jupyter-lab --no-browser --allow-root --ip=0.0.0.0 --port=8888 --NotebookApp.token="" --notebook-dir=/labs'
])
