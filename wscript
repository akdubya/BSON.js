import Options
from os import unlink, symlink, popen
from os.path import exists, abspath
from shutil import copy

srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')
  opt.tool_options('compiler_cc')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('compiler_cc')
  conf.check_tool('node_addon')
  
def build(bld):
  bson = bld.new_task_gen(features = 'cc')
  bson.cppflags = ['-O3']
  bson.cflags = ['-fPIC']
  bson.source = 'deps/bson/bson.c'
  bson.includes = 'deps/bson/'
  bson.name = 'bson'
  bson.target = 'bson'
  bson.before = 'cxx'
  bson.install_path = None

  binding = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  binding.cxxflags = ['-g']
  binding.target = 'binding'
  binding.source = 'src/types.cc src/encode.cc src/decode.cc src/binding.cc'
  binding.add_objects = 'bson'
  binding.includes = 'deps/bson'