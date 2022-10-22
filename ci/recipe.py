"""
HPCCM development container for the C++ HPC tutorial
https://github.com/NVIDIA/hpc-container-maker/
"""
nvhpc_ver = '22.9'
cuda_ver = '11.7'
gcc_ver = '12'
llvm_ver = '16'

#Stage0 += baseimage(image = f'nvcr.io/nvidia/nvhpc:{nvhpc_ver}-devel-cuda{cuda_ver}-ubuntu20.04')
Stage0 += baseimage(image = f'nvcr.io/nvidia/nvhpc:{nvhpc_ver}-devel-cuda_multi-ubuntu22.04')

Stage0 += packages(ospackages=[
    'libtbb-dev',  # Required for GCC C++ parallel STL
    'python3', 'python3-pip', 'python-is-python3', 'python3-setuptools',
    'nginx', 'zip', 'make', 'build-essential', 'curl',
    'git', 'bc', 'debianutils', 'libnuma1', 'openssh-client', 'wget', 'numactl',
    # editors
    'emacs', 'vim',
])

# Install GNU and LLVM toolchains and CMake
Stage0 += gnu(version=gcc_ver, extra_repository=True)
Stage0 += llvm(version=llvm_ver, upstream=True, extra_tools=True, toolset=True)
Stage0 += cmake(eula=True, version='3.24.2')

# Install NSight Systems and NSight Compute profilers
# These are shipped with the HPC SDK containers now
# Stage0 += nsight_systems(eula=True)
# Stage0 += nsight_compute(eula=True)

Stage0 += shell(commands=[
    'set -ex',  # Exit on first error and debug output

    # Configure the HPC SDK toolchain to pick the latest GCC
    f'cd /opt/nvidia/hpc_sdk/Linux_x86_64/{nvhpc_ver}/compilers/bin/',
    'makelocalrc -d . -x .',

    # Install required python packages for the notebooks
    'pip install --upgrade pip',
    'pip install numpy matplotlib conan gdown jupyterlab ipywidgets',

    # Install latest versions of range-v3 and std::execution
    'mkdir -p /var/tmp',
    'cd /var/tmp',
    'git clone https://github.com/ericniebler/range-v3.git',
    'cp -r range-v3 /usr/local/range-v3',
    'rm -rf range-v3',
    'git clone https://github.com/brycelelbach/wg21_p2300_std_execution.git',
    'cp -r wg21_p2300_std_execution /usr/local/execution',
    'rm -rf wg21_p2300_std_execution',
    'cd -',
    
])

# libc++abi : make sure clang with -stdlib=libc++ can find it
Stage0 += shell(commands=[
    f'ln -sf /usr/lib/llvm-{llvm_ver}/lib/libc++abi.so.1 /usr/lib/llvm-{llvm_ver}/lib/libc++abi.so',
])
Stage0 += environment(variables={
    'LD_LIBRARY_PATH': f'/usr/lib/llvm-{llvm_ver}/lib:$LD_LIBRARY_PATH',
    'LIBRARY_PATH':    f'/usr/lib/llvm-{llvm_ver}/lib:$LIBRARY_PATH',
    'OMPI_MCA_coll_hcoll_enable':'0',
})
Stage0 += copy(src='labs/', dest='/labs/')

# Install Intel's OneAPI toolchain
#Stage0 += packages(ospackages=['linux-headers-generic', 'intel-basekit', 'intel-hpckit'],
#                   apt_keys=['https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS-2023.PUB'],
#                   apt_repositories=['deb https://apt.repos.intel.com/oneapi all main']
#)
