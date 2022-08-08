#!/usr/bin/env python3
import sys
import pathlib
import subprocess
import textwrap
import os

fn_repo = pathlib.Path(__file__).resolve().parent

# use system nm, as the extensa one does not support -f just-symbols
# fn_nm = os.getenv("NM","nm")
fn_nm = "nm"
if len(sys.argv) < 3:
    exit(0)

fn_out = pathlib.Path(sys.argv[1])

header = """
#include "main/static-module-registry.hpp"
#include "core/basemodule.hpp"
#include "core/module.hpp"
#include "core/animation.hpp"
#include "core/egress.hpp"
#include "core/animation_api.h"
#include "core/egress_api.h"
#include "core/module_api.h"

"""

body = ""
all_modules = tuple(pathlib.Path(fn) for fn in sys.argv[2:])

groups = (
    ("stmod_mod_", "Module", "Modules", "Module",
     (("init", "modno_t modno, const char *argstr, void **puserdata"),
      ("describe", ""), ("deinit", "modno_t modno, void *userdata"),
      ("flush", "modno_t modno, void *userdata"),
      ("leds_added", "modno_t modno, led_i_t count, void *userdata"),
      ("leds_removed",
       "modno_t modno, led_i_t offset, led_i_t count, void *userdata"))),
    ("stmod_anim_", "Animation", "Animations", "AnimationModule",
     (("init",
       "const led_i_t *ledv, size_t ledn, const char *argstr, void **puserdata"
       ), ("deinit", "void *userdata"), ("describe", ""),
      ("iterate",
       "	const led_i_t *ledv, size_t ledn,void *userdata, frame_time_t dt, frame_time_t t"
       ))),
    ("stmod_egress_", "Egress", "EgressModules", "EgressModule",
     (("init", "egressno_t egressno, const char *argstr, void **puserdata"),
      ("describe", ""), ("deinit", "void *userdata"),
      ("flush", "led_i_t firstLED, led_i_t count, void *userdata"))),
    ("stmod_blend_", "Blend", "BlendModules", "BlendModule",
     (("init", "const char *argstr, void **puserdata"), ("describe", ""),
      ("deinit", "void *userdata"),
      ("mix",
       "const led_i_t *ledv, size_t ledn, led_t * accum, const led_t * op2, frame_time_t dt, frame_time_t t, void * userdata"
       ))),
)


def sanitizeModuleName(id):
    return id.replace("-", "_")


def getsyms(fn: pathlib.Path()):
    p = subprocess.run([fn_nm, fn, "-f", "just-symbols"],
                       stdout=subprocess.PIPE)
    id_mod = fn.stem[6:]
    prefix = sanitizeModuleName(id_mod + "_")
    syms_bare = set()
    for ln in p.stdout.decode().splitlines():
        ln = ln.strip()
        if not ln.startswith(prefix): continue
        syms_bare.add(ln[len(prefix):])
    return prefix[:-1], id_mod, syms_bare


if True:  # install symbols

    symdef = ""
    moddef = ""
    for prefix, ctype, cident, cclass, funcs in groups:

        for fn_mod in all_modules:
            if not fn_mod.stem.startswith(prefix): continue

            prefix_mod, id_mod, syms_mod = getsyms(fn_mod)
            # id_mod = id_mod[len(prefix) - 6:]
            if len(syms_mod) < 1: continue

            # moddef += f"""  {cclass}::Define("{id_mod}",{{}});\n"""
            for id, arglist in funcs:
                if id in syms_mod:
                    header += f"""extern "C" {{ extern void {prefix_mod}_{id}({arglist});}}\n"""
                    symdef += f"""  BaseModule::DefineSymbol("{id_mod}","{id}",(void*){prefix_mod}_{id});\n"""

            for id in ("SingletonInstance", ):
                if id in syms_mod:
                    header += f"""extern "C" {{ extern int {prefix_mod}_{id}; }}\n"""
                    symdef += f"""  BaseModule::DefineSymbol("{id_mod}","{id}",(void*)&{prefix_mod}_{id});\n"""

    body += (f"""void ModuleRegistry::Install() {{\n{symdef}{moddef}\n}}""")

if False:  # build dict
    body += textwrap.dedent("""
    const ModuleRegistry::Map &ModuleRegistry::Get() {
        static ModuleRegistry::Map map{
    """)

    for prefix, ctype, cident, funcs in groups:

        for fn_mod in all_modules:
            if not fn_mod.stem.startswith(prefix): continue

            prefix_mod, syms_mod = getsyms(fn_mod)

            body += f"""{{"{prefix_mod}",{{"""

            for id, arglist in funcs:
                if id in syms_mod:
                    header += f"extern void {prefix_mod}_{id}({arglist});\n"
                    # body += f"""{{"{id}",nullptr}},"""
                    body += f"""{{"{id}",(void*){prefix_mod}_{id}}},"""

            body += "}},\n"

    if False:  # old layout
        for prefix, ctype, cident, funcs in groups:

            body += textwrap.dedent(f"""
            const std::unordered_map<std::string, ModuleRegistry::{ctype}> &{cident}() {{
            static std::unordered_map<std::string, ModuleRegistry::{ctype}> LUT{{
        """)

            for fn_mod in all_modules:
                if not fn_mod.stem.startswith(prefix): continue

                prefix_mod, syms_mod = getsyms(fn_mod)

                body += f"""{{"{prefix_mod}",{{"""

                for id, arglist in funcs:
                    if id in syms_mod:
                        header += f"extern void {prefix_mod}_{id}({arglist});\n"
                        body += f"{prefix_mod}_{id},"
                    else:
                        body += "nullptr,"

                body += "}},\n"

            body += textwrap.dedent("""
            };
            return LUT;
        }
        """)

    body += textwrap.dedent("""
        };
        return map;
    }
    """)

os.makedirs(fn_out.parent, exist_ok=True)
with open(fn_out, "w") as f:
    f.write(header)
    f.write(body)
