#!/usr/bin/env python
# encoding: utf-8
# Rafaël Kooi 2019

from waflib import Context, TaskGen
import json, os, re

# msvcdeps recognizes the English /showIncludes prefix. A localized cl.exe
# silently omits header dependencies and can link incompatible struct layouts.
# This changes compiler diagnostics only, not the game's language.
os.environ['VSLANG'] = '1033'

# Some installations retain Portuguese diagnostics despite VSLANG. Normalize
# only the compiler's include lines before msvcdeps consumes its stdout. The
# accent may already be mojibake from the Windows output code page.
if not getattr(Context.Context, '_sa_msvc_headers_normalized', False):
	_original_cmd_and_log = Context.Context.cmd_and_log
	def _cmd_and_log_with_msvc_headers(self, cmd, **kwargs):
		result = _original_cmd_and_log(self, cmd, **kwargs)
		if isinstance(cmd, (list, tuple)) and cmd and os.path.basename(str(cmd[0])).lower() == 'cl.exe' and isinstance(result, str):
			result = re.sub(r'^Observa[^:\r\n]*: incluindo arquivo:', 'Note: including file:', result, flags=re.MULTILINE)
		return result
	Context.Context.cmd_and_log = _cmd_and_log_with_msvc_headers
	Context.Context._sa_msvc_headers_normalized = True

banned_extensions_list = ["rc", "asm", "masm"]

def audit_game_header_dependencies(bld):
	entries=[]
	for group in bld.groups:
		for generator in group:
			for task in getattr(generator,'compiled_tasks',[]):
				if not task.inputs or task.inputs[0].name not in ('fx_cs_shared.cpp','cs_weapon_parse.cpp'):
					continue
				deps=bld.node_deps.get(task.uid(),[])
				entries.append(dict(target=str(getattr(generator,'target','')),source=task.inputs[0].abspath(),
					dependencies=len(deps),weapon_header=any(node.name=='cs_weapon_parse.h' for node in deps)))
	if entries:
		bld.bldnode.make_node('msvc_header_dependency_audit.json').write(json.dumps(entries,indent=2))
		if any(not entry['weapon_header'] for entry in entries):
			bld.fatal('MSVC header dependency scan failed for Counter-Strike weapon data; do not install these DLLs.')

@TaskGen.feature('c', 'cxx', 'fc')
@TaskGen.after_method('propagate_uselib_vars')
def add_pdb_per_object(self):
	"""For msvc/fortran, specify a unique compile pdb per object, to work
	around LNK4099. Flags are updated with a unique /Fd flag based on the
	task output name. This is separate from the link pdb.
	"""
	if not hasattr(self, 'compiled_tasks'):
		return

	link_task = getattr(self, 'link_task', None)

	for task in self.compiled_tasks:
		if task.inputs and task.env.CC_NAME == 'msvc':
			game=self.bld.srcnode.find_node('game')
			gameui=self.bld.srcnode.find_node('gameui')
			if task.inputs[0].is_child_of(game) or task.inputs[0].is_child_of(gameui):
				# Invalidate objects produced before localized header scanning worked.
				for flags in ('CFLAGS','CXXFLAGS'):
					task.env.append_unique(flags,'/DSA_HEADER_DEPS_VERSION=1')
				if not getattr(self.bld,'_sa_header_audit_registered',False):
					self.bld._sa_header_audit_registered=True
					self.bld.add_post_fun(audit_game_header_dependencies)
		if task.env.env:
			task.env.env = dict(task.env.env, VSLANG='1033')
		if task.inputs and (task.inputs[0].name.lower().split(".")[-1] in banned_extensions_list):
			continue

		add_pdb = False
		for flagname in ('CFLAGS', 'CXXFLAGS', 'FCFLAGS'):
			# several languages may be used at once
			for flag in task.env[flagname]:
				if flag[1:].lower() == 'zi':
					add_pdb = True
					break

		if add_pdb:
			node = task.outputs[0].change_ext('.pdb')
			pdb_flag = '/Fd:' + node.abspath()

			for flagname in ('CFLAGS', 'CXXFLAGS', 'FCFLAGS'):
				buf = [pdb_flag]
				for flag in task.env[flagname]:
					if flag[1:3] == 'Fd' or flag[1:].lower() == 'fs' or flag[1:].lower() == 'mp':
						continue
					buf.append(flag)
				task.env[flagname] = buf

			if link_task and not node in link_task.dep_nodes:
				link_task.dep_nodes.append(node)
			if not node in task.outputs:
				task.outputs.append(node)
