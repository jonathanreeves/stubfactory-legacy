# stubfactory

Automatic unit test stub generation.

## BUILDING stubfactory

This utility is meant to be built and linked against libclang. The easiest
way to do this is to download the llvm and clang projects and build them.
This will generate libclang.dylib, which you can then use when building
and linking stubfactory. For example if you have a folder called
<code>llvm-clang</code> at the same level as your stubfactory repo,
you can build stubfactory with the following command line:

<code>clang++ -o stubfactory stubfactory.cpp -Wall -I ../llvm-clang/llvm/tools/clang/include/ -L ../llvm-clang/build/lib/ -lclang -std=c++11 -rpath ../llvm-clang/build/lib/</code>

For detailed instructions on building llvm/clang please refer to the llvm/clang
documentation.

## USING stubfactory

The stubfactory executable creates a clang translation unit for and input
file and thus behaves like a regular instance of clang. Use it the same
way that you would use the compiler, with the same input args, etc. Note
however that if you're writing C++ and your convention is to name your
header files as standard .h (instead of .hpp), you need to tell the
clang translation unit to treat the input as C++ by passing the '-x c++'
argument. So for example if you have a file called 'deviceClass.h` which
declares a class, you would generate stubs from it as follows:

<code>./stubfactory -x c++ -DSOME_DEFINE=2 deviceClass.h </code>

Each method stub will have the following set of global variables declared
on its behalf:
 -  call count: the number of times this function has been called
 -  arg0, arg1, arg2, ...: The last captured function input arguments
 -  hook: A callback function hook that can be set when writing a unit test
       to add functionality to the stub
 -  return: The desired return value for the stub
