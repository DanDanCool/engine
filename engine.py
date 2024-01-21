import jmake
import argparse
import subprocess
from pathlib import Path

jmake.setupenv()

workspace = jmake.Workspace("engine")
workspace.lang = 'cpp20'

engine = jmake.Project("engine", jmake.Target.EXECUTABLE)
files = jmake.glob('src', '**/*.cpp')
engine.add(jmake.fullpath(files))
modules = jmake.glob('src', '**/*.hpp')
engine.add_module(jmake.fullpath(modules), True)

engine.include(jmake.fullpath('src'))

debug = engine.filter("debug")
debug["debug"] = True

test = jmake.Project("test", jmake.Target.EXECUTABLE)
files.remove(jmake.fullpath('src/main.cpp')[0])
files = files + jmake.glob('test', '**/*.cpp') + jmake.glob('test', '**/*.h')
test.add(jmake.fullpath(files))
test.add_module(jmake.fullpath(modules))

test.include(jmake.fullpath('src'))
debug = test.filter("debug")
debug["debug"] = True

host = jmake.Env()
if host.os == jmake.Platform.WIN32:
    engine.define('JOLLY_WIN32', 1)
    engine.define('WIN32_LEAN_AND_MEAN', 1)
    test.define('JOLLY_WIN32', 1)
    test.define('WIN32_LEAN_AND_MEAN', 1)

vulkan = jmake.builtin('vulkan')
spirv_reflect = jmake.package("spirv_reflect", "https://github.com/DanDanCool/SPIRV-Reflect")

engine.depend(vulkan)
engine.depend(spirv_reflect)

test.depend(vulkan)
test.depend(spirv_reflect)

workspace.add(engine)
workspace.add(test)

def compileshaders(workspace, args):
    p = Path('assets/shaders')
    for shader in p.glob('*'):
        if shader.suffix not in [ '.vert', '.frag' ]:
            continue
        print(f"compiling shader {str(shader)}...")
        cmd = [ 'glslc', str(shader), '-o', str(p / f"{shader.name}.spv") ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        print(res.stdout)
        print(res.stderr)

parser = argparse.ArgumentParser()
subparser = parser.add_subparsers()
shader_parser = subparser.add_parser('shader')
shader_parser.set_defaults(func=compileshaders)

jmake.generate(workspace, parser, subparser)
