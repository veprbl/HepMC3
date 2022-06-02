name: HepMC3 Build Matrix for Windows and MacOSX

on: [push, pull_request]

jobs:
  nativeosx:
    if: "contains(github.event.head_commit.message, 'github')"
    name: "${{ matrix.config.name }} ${{ matrix.python-version }}"
    runs-on: ${{ matrix.config.os }}
    strategy:
      fail-fast: false
      matrix:
        python-version: ["3.9"]
#        python-version: ["2.7", "3.9"]
        config: 
        - {
            name: "MacOSX-12",
            os: macos-12,
            cc: "clang", 
            cxx: "clang++"
          }
#        - {
#            name: "MacOSX-11",
#            os: macos-11,
#            cc: "clang", 
#            cxx: "clang++"
#          }
#        - {
#            name: "MacOSX-10.15",
#            os: macos-10.15,
#            cc: "clang", 
#            cxx: "clang++"
#          }

    steps:
    - uses: actions/checkout@v2
    - name: Set up Python
      uses: actions/setup-python@v2
      with:
          python-version: "${{ matrix.python-version }}"
    - name: Build
      run:  |
        cmake -S. -B BUILD -DHEPMC3_ENABLE_ROOTIO:BOOL=OFF -DHEPMC3_ENABLE_TEST:BOOL=ON
        cmake --build BUILD 
        cd BUILD
        ctest . 
  nativewin:
    if: "contains(github.event.head_commit.message, 'github')"
    name: "${{ matrix.config.name }} ${{ matrix.python-version }}"
    runs-on: ${{ matrix.config.os }}
    strategy:
      fail-fast: false
      matrix:
        python-version: ["3.9"]
#        python-version: ["2.7", "3.9"]
        config: 
        - {
            name: "Windows Latest MSVC",
            os: windows-latest,
            cc: "cl", 
            cxx: "cl",
            environment_script: "C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise/VC/Auxiliary/Build/vcvars64.bat"
          }
    steps:
      - uses: actions/checkout@v2
      - name: Set up Python
        uses: actions/setup-python@v2
        with:
          python-version: "${{ matrix.python-version }}"
      - name: Build
        run:  |
          cmake -S. -B BUILD -DHEPMC3_ENABLE_ROOTIO:BOOL=OFF -DHEPMC3_ENABLE_TEST:BOOL=ON
          cmake --build BUILD 
          chdir BUILD
          ctest . -C Debug

  mingw:
    if: "contains(github.event.head_commit.message, 'github')"
    name: "windows-latest  ${{ matrix.sys }}"
    runs-on: windows-latest
    defaults:
      run:
        shell: msys2 {0}
    strategy:
      fail-fast: false
      matrix:
        include:
          - { sys: mingw64, env: x86_64 }
#          - { sys: mingw32, env: i686 }
    steps:
    # Force version because of https://github.com/msys2/setup-msys2/issues/167
    - uses: msys2/setup-msys2@v2
      with:
        msystem: ${{matrix.sys}}
        install: >-
          mingw-w64-${{matrix.env}}-gcc
          mingw-w64-${{matrix.env}}-gcc-fortran
          mingw-w64-${{matrix.env}}-ninja
          mingw-w64-${{matrix.env}}-python-pip
          mingw-w64-${{matrix.env}}-python
          mingw-w64-${{matrix.env}}-python-numpy
          mingw-w64-${{matrix.env}}-python-setuptools
          mingw-w64-${{matrix.env}}-cmake
          mingw-w64-${{matrix.env}}-make
    - uses: actions/checkout@v2
    - name: Build
      run:  |
        cmake -S. -B BUILD -DHEPMC3_ENABLE_ROOTIO:BOOL=OFF -DHEPMC3_ENABLE_TEST:BOOL=ON
        cmake --build BUILD 
        cd BUILD
        ctest .
