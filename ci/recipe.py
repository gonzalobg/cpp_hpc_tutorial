"""
HPCCM development container for the C++ HPC tutorial
https://github.com/NVIDIA/hpc-container-maker/
"""
import platform
nvhpc_ver = '23.5'
cuda_ver = '12.1'
gcc_ver = '12'
llvm_ver = '17'

Stage0 += baseimage(image = f'nvcr.io/nvidia/nvhpc:{nvhpc_ver}-devel-cuda{cuda_ver}-ubuntu22.04')

arch = 'x86_64'
if platform.machine() == 'aarch64':
    arch = 'aarch64'

Stage0 += packages(ospackages=[
    'libtbb-dev',  # Required for GCC C++ parallel STL
    'python3', 'python3-pip', 'python-is-python3', 'python3-setuptools', 'python3-dev',
    'nginx', 'zip', 'make', 'build-essential', 'curl',
    'git', 'bc', 'debianutils', 'libnuma1', 'openssh-client', 'wget', 'numactl',
    # editors
#    'emacs', 'vim',
])

# Install GNU and LLVM toolchains and CMake
Stage0 += gnu(version=gcc_ver, extra_repository=True)
Stage0 += llvm(version=llvm_ver, upstream=True, extra_tools=True, toolset=True)
Stage0 += cmake(eula=True, version='3.24.2')

Stage0 += shell(commands=[
    'set -ex',  # Exit on first error and debug output

    # Configure the HPC SDK toolchain to pick the latest GCC
    f'cd /opt/nvidia/hpc_sdk/Linux_{arch}/{nvhpc_ver}/compilers/bin/',
    'makelocalrc -d . -x .',

    # Install required python packages for the notebooks
    'pip install --upgrade pip',
    'pip install numpy matplotlib gdown jupyterlab ipywidgets pandas seaborn conan',

    # Install latest versions of range-v3, NVIDIA std::execution, and NVTX3
    'mkdir -p /var/tmp',
    'cd /var/tmp',
    'git clone --depth=1 https://github.com/ericniebler/range-v3.git',
    'cp -r range-v3/include/* /usr/include/',
    'rm -rf range-v3',
    'git clone --depth=1 https://github.com/nvidia/stdexec.git',
    'cp -r stdexec/include/* /usr/include/',
    'rm -rf stdexec',
    'git clone --depth=1 --branch=release-v3 https://github.com/NVIDIA/NVTX.git',
    'cp -r NVTX/c/include/nvtx3 /usr/include/nvtx3',
    'rm -rf NVTX',
    'cd -',

    # libc++abi: make sure clang with -stdlib=libc++ can find it
    f'ln -sf /usr/lib/llvm-{llvm_ver}/lib/libc++abi.so.1 /usr/lib/llvm-{llvm_ver}/lib/libc++abi.so',

    # Install HPC SDK mdspan systemwide:
    f'ln -sf /opt/nvidia/hpc_sdk/Linux_{arch}/{nvhpc_ver}/compilers/include/experimental/mdspan /usr/include/mdspan',
    f'ln -sf /opt/nvidia/hpc_sdk/Linux_{arch}/{nvhpc_ver}/compilers/include/experimental/__p0009_bits /usr/include/__p0009_bits',

    # Put the labs include directory in the systemwide path:
    f'ln -sf /src/include /usr/include/labs',
])


Stage0 += environment(variables={
    'LD_LIBRARY_PATH': f'/usr/lib/llvm-{llvm_ver}/lib:$LD_LIBRARY_PATH',
    'LIBRARY_PATH':    f'/usr/lib/llvm-{llvm_ver}/lib:$LIBRARY_PATH',
    # Simplify running HPC-X on systems without InfiniBand
    'OMPI_MCA_coll_hcoll_enable':'0',
    # We do not need VFS for the lab, and using it from a container in a 'generic' way is not trivial:
    'UCX_VFS_ENABLE': 'n',
    # Allow HPC-X to oversubscribe the CPU with more ranks than cores without using mpirun --oversubscribe
    'OMPI_MCA_rmaps_base_oversubscribe' : 'true',
    # Select matplotdir config directory to silence warning
    'MPLCONFIGDIR': '/tmp/matplotlib',
    # DLI course needs to run as root:
    'OMPI_ALLOW_RUN_AS_ROOT': '1',
    'OMPI_ALLOW_RUN_AS_ROOT_CONFIRM': '1',
    # Workaround hwloc binding:
    'OMPI_MCA_hwloc_base_binding_policy': 'none',
})
#Stage0 += copy(src='labs/', dest='/labs/')
Stage0 += copy(src='include/cartesian_product.hpp', dest='/usr/include/cartesian_product.hpp')

# Install Intel's OneAPI toolchain
#Stage0 += packages(ospackages=['linux-headers-generic', 'intel-basekit', 'intel-hpckit'],
#                   apt_keys=['https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS-2023.PUB'],
#                   apt_repositories=['deb https://apt.repos.intel.com/oneapi all main']
#)
