"""
HPCCM development container for the C++ HPC tutorial
https://github.com/NVIDIA/hpc-container-maker/
"""
import platform
nvhpc_ver = '24.3'
cuda_ver = '12.3'
gcc_ver = '13'
llvm_ver = '18'
cmake_ver = '3.27.2'
boost_ver = '1.75.0'

Stage0 += baseimage(image = f'nvcr.io/nvidia/nvhpc:{nvhpc_ver}-devel-cuda{cuda_ver}-ubuntu22.04')

arch = 'x86_64'
if platform.machine() == 'aarch64':
    arch = 'aarch64'

Stage0 += packages(ospackages=[
    'libtbb-dev',  # Required for GCC C++ parallel STL
    'python3', 'python3-pip', 'python-is-python3', 'python3-setuptools', 'python3-dev',
    'nginx', 'zip', 'make', 'build-essential', 'curl',
    'git', 'bc', 'debianutils', 'libnuma1', 'openssh-client', 'wget', 'numactl',
])

# Install GNU and LLVM toolchains and CMake
Stage0 += gnu(version=gcc_ver, extra_repository=True)
Stage0 += llvm(version=llvm_ver, upstream=True, extra_tools=True, toolset=True, _trunk_version='19')
Stage0 += cmake(eula=True, version=cmake_ver)

Stage0 += shell(commands=[
    'set -ex',  # Exit on first error and debug output

    # Workaround docker runtime bug that fails to link libnvidia-ml as .so: nvbugs/4248302
    f'ln -s /usr/lib/{arch}-linux-gnu/libnvidia-ml.so.1 /usr/lib/{arch}-linux-gnu/libnvidia-ml.so',
    
    # Backport libstdc++ ranges bugfix to GCC 12:
    # https://gcc.gnu.org/pipermail/libstdc++/2023-July/056266.html
    f"sed -i 's@std::is_same<typename std::iterator_traits<_IteratorType>::iterator_category, std::random_access_iterator_tag>@std::integral_constant<bool, std::random_access_iterator<_IteratorType>>@g' /usr/include/c++/{gcc_ver}/pstl/execution_impl.h",
    
    # Configure the HPC SDK toolchain to pick the latest GCC
    f'cd /opt/nvidia/hpc_sdk/Linux_{arch}/{nvhpc_ver}/compilers/bin/',
    'makelocalrc -d . -x .',

    # Install required python packages for the notebooks
    'pip install --upgrade pip',
    'pip install numpy matplotlib gdown jupyterlab ipywidgets pandas seaborn conan jupyterlab-nvidia-nsight',

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
    # Workaround nvfortran limit of 64k thread blocks
    'NVCOMPILER_ACC_GANGLIMIT': '67108864', # (1 << 26)
})
#Stage0 += copy(src='labs/', dest='/labs/')
Stage0 += copy(src='include/cartesian_product.hpp', dest='/usr/include/cartesian_product.hpp')
Stage0 += copy(src='include/ranges', dest=f'/usr/include/c++/{gcc_ver}/ranges')

Stage0 += environment(variables={
    'CPPTUT_NVHPC_VER': nvhpc_ver,
    'CPPTUT_CUDA_VER': cuda_ver,
    'CPPTUT_ARCH': arch,
})

# Install AdaptiveCpp stdpar:
Stage0 += boost(version=boost_ver)
Stage0 += shell(commands=[
    'set -ex',
    'git clone --recurse-submodules -b develop https://github.com/AdaptiveCpp/AdaptiveCpp',
    'cd AdaptiveCpp',
    'git submodule update --recursive',
    f'cmake -Bbuild -H.  -DCMAKE_C_COMPILER="$(which clang-{llvm_ver})" -DCMAKE_CXX_COMPILER="$(which clang++-{llvm_ver})" -DCMAKE_INSTALL_PREFIX=/opt/adaptivecpp  -DWITH_CUDA_BACKEND=ON  -DWITH_CPU_BACKEND=ON',
    'cmake --build build --target install -j $(nproc)',
])
Stage0 += environment(variables={
    'PATH':'$PATH:/opt/adaptivecpp/bin',
    'ACPP_APPDB_DIR': '/src/',
})

Stage0 += shell(commands=[
    f'echo "#define MDSPAN_USE_PAREN_OPERATOR 1"|cat - /opt/nvidia/hpc_sdk/Linux_{arch}/{nvhpc_ver}/compilers/include/experimental/mdspan > /tmp/out && mv /tmp/out /opt/nvidia/hpc_sdk/Linux_{arch}/{nvhpc_ver}/compilers/include/experimental/mdspan',
    f'echo "namespace std {{ using namespace ::std::experimental; }}" >> /opt/nvidia/hpc_sdk/Linux_{arch}/{nvhpc_ver}/compilers/include/experimental/mdspan',
])
