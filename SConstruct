##
# @author Alexander Breuer (alex.breuer AT uni-jena.de)
#
# @section DESCRIPTION
# Entry-point for builds.
##
import SCons
import platform


print( '####################################' )
print( '### Tsunami Lab                  ###' )
print( '###                              ###' )
print( '### https://scalable.uni-jena.de ###' )
print( '####################################' )
print()
print('runnning build script')

# configuration
vars = Variables()

vars.AddVariables(
  EnumVariable( 'mode',
                'compile modes, option \'san\' enables address and undefined behavior sanitizers',
                'release',
                allowed_values=('release', 'debug', 'release+san', 'debug+san' )
              )
)

# exit in the case of unknown variables
if vars.UnknownVariables():
  print( "build configuration corrupted, don't know what to do with: " + str(vars.UnknownVariables().keys()) )
  exit(1)

# defines plattform: 0 = Linux, 1 = Mac
os_system = platform.system()
if os_system == "Linux":
    plattform_choice = 0
elif os_system == "Darwin":  # Darwin is the underlying OS for MacOS
    plattform_choice = 1
elif os_system == "Windows": 
    plattform_choice = 2
else:
    print(f"Unsupported OS: {os_system}")
    exit(1)

# create environment
env = Environment( variables = vars )

if(plattform_choice == 0): # Linux
  env.Append(CCFLAGS=['-lOpenCL'])
  env.Append(LINKFLAGS=['-lOpenCL'])
  env.Append(LIBS=['OpenCL'])
elif(plattform_choice == 1): # Mac
  env['CXX'] = '/opt/homebrew/bin/g++-13'
  env['CC'] = '/opt/homebrew/bin/gcc-13'

  # Add lib paths
  env.Append(CPPPATH=['/opt/homebrew/include'])
  env.Append(LIBPATH=['/opt/homebrew/lib'])

  env.Append(LINKFLAGS=['-framework', 'OpenCL'])
elif(plattform_choice == 2): # Windows
  env.Append(CPPPATH=['C:\msys64\mingw64\include'])
  env.Append(LIBPATH=['C:\msys64\mingw64\lib'])
  env.Append(CCFLAGS=['-lOpenCL'])
  env.Append(LINKFLAGS=['-lOpenCL'])
  env.Append(LIBS=['OpenCL'])


# set compiler
cxx_compiler = ARGUMENTS.get('comp', "g++")

if cxx_compiler == 'g++':
  pass
else:
  env['CXX'] = "/opt/intel/oneapi/compiler/2023.2.2/linux/bin/intel64/icpc"
  env.Append( CXXFLAGS = ['-qopt-report=5'])


# Add NetCDF include
env.Append(LIBS=['netcdf'])
# Added parallelization
env.Append(CCFLAGS=['-fopenmp'])
env.Append(LINKFLAGS=['-fopenmp'])

# generate help message
Help( vars.GenerateHelpText( env ) )

# add default flags
env.Append( CXXFLAGS = [ '-std=c++17',
                         '-Wall',
                         '-Wextra',
                         '-Wpedantic',
                         '-Werror' ] )

# set optimization mode
if 'debug' in env['mode']:
  env.Append( CXXFLAGS = [ '-g',
                           '-O0' ] )
else:
  env.Append( CXXFLAGS = [ '-O2' ] )
  #env.Append( CXXFLAGS = [ '-O3' ] )
  #env.Append( CXXFLAGS = [ '-Ofast' ] )

# add sanitizers
if 'san' in  env['mode']:
  env.Append( CXXFLAGS =  [ '-g',
                            '-fsanitize=float-divide-by-zero',
                            '-fsanitize=bounds',
                            '-fsanitize=address',
                            '-fsanitize=undefined',
                            '-fno-omit-frame-pointer' ] )
  env.Append( LINKFLAGS = [ '-g',
                            '-fsanitize=address',
                            '-fsanitize=undefined' ] )

# add Catch2
env.Append( CXXFLAGS = [ '-isystem', 'submodules/Catch2/single_include' ] )

# get source files
VariantDir( variant_dir = 'build/src',
            src_dir     = 'src' )

env.sources = []
env.tests = []

Export('env')
SConscript( 'build/src/SConscript' )
Import('env')

env.Program( target = 'build/tsunami_lab',
             source = env.sources + env.standalone )

env.Program( target = 'build/tests',
             source = env.sources + env.tests )
