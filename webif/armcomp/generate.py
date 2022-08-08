#!/usr/bin/env python3
# Copyright 2022 Peter Wagener <mail@peterwagener.net>
#
# This file is part of Freyr2.
#
# Freyr2 is free software: you can redistribute it and/or modify it under the 
# terms of the GNU General Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your option) any later 
# version.
#
# Freyr2 is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR 
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with 
# Freyr2. If not, see <https://www.gnu.org/licenses/>.

import pathlib
import re
import subprocess
import rjsmin
import rcssmin
import pprint
import xmltodict
import htmlmin
import os

fn_here = pathlib.Path(__file__).resolve().parent

fnOutDir = fn_here / "htdocs"

os.makedirs(fnOutDir, exist_ok=True)

sources = set(fn_here / "src" / v for v in os.listdir(fn_here / "src"))


def processFile(fnIn: pathlib.Path, minimize=True):
	raw = fnIn.open().read()
	return processContent(raw, fnIn.suffix, minimize, fnIn.name)


def processContent(raw: str, suffix, minimize=True, title=""):
	res = raw
	if suffix == ".js":
		if minimize:
			if False: # minimize with hjsmin
				p = subprocess.Popen(
				  ["hjsmin", "-i", "/dev/stdin", "-o", "/dev/stdout"],
				  stdin=subprocess.PIPE,
				  stdout=subprocess.PIPE)
				(sout, serr) = p.communicate(raw.encode())
				res = sout.decode()
			if True: # minimize with uglifyjs
				p = subprocess.Popen(["uglifyjs", "-c", "-m"],
				                     stdin=subprocess.PIPE,
				                     stdout=subprocess.PIPE)
				(sout, serr) = p.communicate(raw.encode())
				res = sout.decode()
			if False: #minimize with rjsmin
				res = rjsmin.jsmin(raw)
	elif suffix == ".css":
		if minimize:
			res = rcssmin.cssmin(raw)

	elif suffix == ".svg":

		def splitstyle(style):
			return {
			  k: v
			  for k, v in (item.split(":", 1) for item in style.split(";"))
			}

		def rec(node, *path):

			if type(node) == list:
				for i, v in enumerate(node):
					rec(v, *path, i)
				return
			elif type(node) != dict:
				return
			if "@mystyle" in node and False:
				style = splitstyle(node["@style"])
				style.update(splitstyle(node["@mystyle"]))
				style = ";".join(":".join(kv) for kv in style.items())
				node["@style"] = style
				del node["@mystyle"]
			for k, v in node.items():
				if k[:1] == "@": continue
				rec(v, *path, k)

		xml = xmltodict.parse(raw)
		rec(xml)
		res = xmltodict.unparse(xml)

	if len(res) < len(raw):
		print(
		  f"minimized {title} from {len(raw)} to {len(res)} ({len(res)/len(raw)*100.0:.2f}%)"
		)
	return res


def processTemplate(fnIn: pathlib.Path):

	raw = fnIn.open().read()
	iRaw = 0
	res = ""
	for m in re.finditer(r"\{\s*(.*?)\s*:\s*(.*?)\s*\}", raw):
		i0, i1 = m.span()
		if i0 > iRaw:
			res += raw[iRaw:i0]
		iRaw = i1

		cmd, args = m.groups()

		if cmd == "file":
			res += processFile(fnIn.parent / args)
		elif cmd == "file-group":
			expr = re.compile(args)
			files = tuple(sorted(v for v in sources if expr.match(str(v.name))))
			extensions = set(v.suffix for v in files)
			if len(extensions) > 0:
				suffix = extensions.pop()
			else:
				suffix = ""
			raw1 = ""
			for fn in files:
				raw2 = fn.open().read()
				print("adding", fn.name, len(raw2))
				raw1 += raw2
			res += processContent(raw1, suffix, title=args)

	if iRaw < len(raw):
		res += raw[iRaw:]

	res = htmlmin.minify(res,
	                     remove_comments=True,
	                     reduce_boolean_attributes=True)
	fnOut = fnOutDir / fnIn.name
	fnOutCompressed = fn_here / "htdocs" / f"{fnIn.name}.gz"

	fnOut.open("w").write(res)
	subprocess.run(["gzip", "--keep", "--force", fnOut])
	resCompressed = open(fnOutCompressed, "rb").read()
	print(
	  f"{fnIn.name}:{len(res)}B, compressed:{len(resCompressed)}B ({len(resCompressed)/len(res)*100.0:.2f}%)"
	)


processTemplate(fn_here / "src" / "index.html")
