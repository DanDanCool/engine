import jmake

workspace = jmake.Workspace("engine")

engine = jmake.Project("engine", jmake.Target.EXECUTABLE)
files = jmake.glob('src', '**/*.cpp') + jmake.glob('src', '**/*.h')
engine.add(jmake.fullpath(files))

debug = engine.filter("debug")
debug["debug"] = True

host = jmake.Host()
if host.os == jmake.Platform.WIN32:
    engine.define('JOLLY_WIN32', 1)
    engine.define('WIN32_LEAN_AND_MEAN', 1)

workspace.add(engine)
jmake.generate(workspace)
