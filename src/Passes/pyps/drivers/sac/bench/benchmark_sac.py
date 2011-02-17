#!/usr/bin/env python
# -*- encoding: utf-8 -*-
from __future__ import with_statement # to cope with python2.5

#import pyrops
import pyps
import workspace_gettime as gt
import workspace_remote as rt
import workspace_check as ck
import memalign
import sac
import sys
import os
import time
import shutil
import bench_cfg
import shlex
import copy

from subprocess import *
from optparse import OptionParser
import re

def benchrun(s,calms=None,calibrate_out=None):
	def do_benchmark(ws, wcfg, cc_cfg, compile_f, args, n_iter, name_custom):
		times = {wcfg.module: [0]}
		benchname = cc_cfg.name() + "+" + name_custom
		ccp = pyps.ccexecParams(compilemethod=compile_f,CC=cc_cfg.cc,CFLAGS=cc_cfg.cflags,args=args,outfile=benchname)
		try:
			if doBench:
				times = ws.benchmark(ccexecp=ccp,iterations=n_iter)
				benchtimes[benchname] = {'time': times[wcfg.module][0], 'cc_cmd': ccp.cc_cmd, 'cc_stderr': ccp.cc_stderr}
			else:
				good,out = ws.check_output(ccexecp=ccp)
				if not good:
					msg = "Validation case %s-%s failed !" % (wcfg.name(),benchname)
					errors.append(msg)
					print >>sys.stderr, msg
					if opts.strict: raise RuntimeError(msg)
		except RuntimeError, e:
			errors.append("Benchmark: %s\n%s" % (benchname, str(e)))
			print >> sys.stderr, e
			if opts.strict: raise
	
	def do_calibrate(ws, cc_cfg_ref, args, arg_n, calms, module_name):
		''' Calibrate the args of a workspace using the reference compiler, such as it takes a given running time. '''
		ccp = pyps.ccexecParams(compilemethod=ws.compile,CC=cc_cfg_ref.cc,CFLAGS=cc_cfg_ref.cflags,args=args,outfile=cc_cfg_ref.name()+"+calibrate")
		size_kernel = int(args[arg_n])

		# First, it looks at the time taken by the workspace with the default argument.
		# Then, it computes an approximate value (supposing that the running time varies linearely
		# with the kernel size).
		# And, finally, it makes a dichotomy in order to find a good kernel size approximation.

		# Define a tolerance for the running time (in ms)
		TOLERANCE_MS = 5
		def make_args(size_kernel):
			args_ar_tmp = copy.deepcopy(args)
			args_ar_tmp[arg_n] = str(size_kernel)
			return args_ar_tmp

		def get_run_time(size_kernel):
			''' Return the running time with the current kernel size in ms '''
			ccp.args = make_args(size_kernel)
			times = ws.benchmark(ccexecp=ccp,iterations=5) # Return time in us
			return times[module_name][0]/1000

		cur_runtime = get_run_time(size_kernel)
		print "Org runtime:",cur_runtime
		# Compute the first approximation of size_kernel
		size_kernel = size_kernel * calms/cur_runtime
		cur_runtime = get_run_time(size_kernel)
		print "Approx runtime:",cur_runtime

		if cur_runtime < calms:
			low_size = size_kernel
			high_size = size_kernel*2
		else:
			high_size = size_kernel
			low_size = min(size_kernel/10,100)
		# Do the dichotomy if useful
		while abs(cur_runtime-calms) > TOLERANCE_MS:
			new_size = (high_size+low_size)/2
			cur_runtime = get_run_time(new_size)
			if cur_runtime < calms:
				low_size = new_size
			else:
				high_size = new_size

		return make_args(size_kernel)

	doBench = s.default_mode=="benchmark"
	doCalibrate = calms != None
	#wk_parents = [sac.workspace,memalign.workspace]
	wk_parents = [sac.workspace]
	if doBench:
		wk_parents.append(gt.workspace)
	else:
		wk_parents.append(ck.workspace)
	if s.remoteExec:
		wk_parents.append(rt.workspace)
	for wcfg in s.workspaces:
		wcfg.load()
		benchtimes = {}
		srcs = map(lambda s: str(s), wcfg.files)
		wcfg.module = str(wcfg.module)
		if doBench:
			cflags = "-D__PYPS_SAC_BENCHMARK "
		else:
			cflags = "-D__PYPS_SAC_VALIDATE "
		if "include_dirs" in wcfg:
			cflags += "-I" +  str(" -I".join(wcfg.include_dirs))
		s.cc_reference.load()
		ccp_ref=None
		if not doBench:
			args = shlex.split(str(wcfg.args_validate))
			ccp_ref = pyps.ccexecParams(CC=s.cc_reference.cc,CFLAGS=s.cc_reference.cflags,args=args)
		with pyps.workspace(*srcs,
				       parents = wk_parents,
				       driver = s.default_driver,
				       remoteExec = s.remoteExec,
				       cppflags = cflags,
				       deleteOnClose=False,
				       recoverIncludes=False,
				       ccexecp_ref=ccp_ref) as ws:
			m = ws[wcfg.module]
			if doBench:
				args = wcfg.args_benchmark
				n_iter = wcfg.iterations_bench
				m.benchmark_module()
			else:
				args = wcfg.args_validate
				n_iter = 1
			args = shlex.split(str(args))

#			if wcfg.memalign:
#				ws.memalign()

			if doCalibrate:
				new_args = do_calibrate(ws, s.cc_reference, args, wcfg.arg_kernel_size, calms, wcfg.module)
				print new_args
				wcfg.args_benchmark = " ".join(new_args)
				continue 

			if doBench:
				do_benchmark(ws, wcfg, s.cc_reference, ws.compile, args, n_iter, "ref")
				for cc in s.ccs_nosac:
					cc.load()
					do_benchmark(ws, wcfg, cc, ws.compile, args, n_iter, "nosac")

			if not doBench:
				m.sac()
				# If we are in validation mode, validate the generic SIMD implementation thanks
				# to s.cc_reference
				do_benchmark(ws, wcfg, s.cc_reference, ws.compile, args, n_iter, "ref+sac")

			if "ccs_sac" in s:
				for cc in s.ccs_sac:
					cc.load()
					do_benchmark(ws, wcfg, cc, ws.simd_compile, args, n_iter, "sac")


		wstimes[wcfg.name()] = benchtimes
	
	if doCalibrate:
		bench_cfg.workspaces.save(calibrate_out)
		return


parser = OptionParser(usage = "%prog")
parser.add_option("-m", "--mode", dest = "mode",
				  help = "benchmark mode: validation or benchmark")
parser.add_option("-s", "--session", dest = "session_name",
				  help = "session to use (defined in sessions.cfg")
parser.add_option("-d", "--driver", dest = "driver",
				  help = "driver to use (avx|sac|3dnow|neon)")
parser.add_option("--cflags", "--CFLAGS", dest = "cflags",
				  help = "additionnal CFLAGS for all compilations")
parser.add_option("--normalize", dest = "normalize", action = "store_true",
				  default = False, help = "normalize timing results")
parser.add_option("--remote-host", dest = "remoteHost",
				  help = "compile and execute sources on a remote machine (using SSH)")
parser.add_option("--control-master-path", dest = "controlMasterPath",
				  help = "path to the SSH control master (if wanted) [optional]")
parser.add_option("--remote-working-directory", dest = "remoteDir",
				  help = "path to the remote directory that will be used")
parser.add_option("--outfile", dest="outfile",
				  help = "write the results into a file [default=stdout]")
parser.add_option("--strict", dest="strict", action="store_true",
		help = "stop the program as soon as an exception occurs.")
parser.add_option("--calibrate", dest="calms",
				  help = "Calibrate the argument of the workspaces so that they take a given running time (in ms). For benchmark mode only")
parser.add_option("--calibrate-etc-out", dest="calout",
				  help = "Output workspaces configuration file for --calibrate")
parser.add_option("--with-etc-dir", dest="etc_dir",
				  help = "Use this directory for configuration files")
parser.add_option("--with-etc-wk", dest="etc_wk_file",
				  help = "Use this configuration file for workspaces (override the one that will be used by default, or with --with-etc-dir)")
(opts, _) = parser.parse_args()

wstimes = {}
errors = []

etc_dir = "etc/"
if opts.etc_dir != None:
	etc_dir = opts.etc_dir
wk_file = None
if opts.etc_wk_file != None:
	wk_file = opts.etc_wk_file

bench_cfg.init(etc_dir=etc_dir,wk_file=wk_file)

session = bench_cfg.sessions.get(opts.session_name)
session.load()

if opts.remoteHost:
	if opts.remoteDir == None:
		raise RuntimeError("--remote-working-directory option is required !")
	session.remoteExec = rt.remoteExec(host=opts.remoteHost, controlMasterPath=opts.controlMasterPath, remoteDir=opts.remoteDir)
elif "default_remote" in session:
	session.default_remote.load()
	session.remoteExec = rt.remoteExec(host=session.default_remote.host, controlMasterPath=session.default_remote.control_path, remoteDir=session.default_remote.remote_working_dir)
else:
	session.remoteExec = False

if opts.driver:
	session.default_driver = opts.driver

if opts.mode:
	session.default_mode = opts.mode


calms = None
calout = None
if opts.calms:
	calms = int(opts.calms)
	if opts.calout == None:
		raise RuntimeError("--calibrate requires --calibrate-etc-out !")
	calout = opts.calout

benchrun(session,calms,calout)

if opts.outfile:
	outfile = open(opts.outfile, "w")
else:
	outfile = sys.stdout

if session.default_mode == "benchmark":
	outfile.write("\t")
	cc_ref_name = session.cc_reference.name()+"+ref"
	columns = [cc_ref_name]
	for cc in session.ccs_nosac: columns.append(cc.name()+"+nosac")
	if "ccs_sac" in session:
		for cc in session.ccs_sac: columns.append(cc.name()+"+sac")
	for c in columns:
		outfile.write(c+"\t")
	outfile.write("\n")
	for wsname, benchtimes in wstimes.iteritems():
		outfile.write(wsname + "\t")
		if cc_ref_name not in benchtimes:
			print >>sys.stderr, "Warning: reference compilater %s not computed. Normalisation disabled." % cc_ref_name
			opts.normalize = False

		for benchname in columns:
			if benchname not in benchtimes:
				outfile.write("NA\t")
				continue
			res = benchtimes[benchname]
			bencht = res['time']
			if opts.normalize:
				bencht = float(benchtimes[cc_ref_name]['time']) / float(bencht)

			outfile.write(str(bencht))
			outfile.write("\t")
		if opts.normalize:
			outfile.write("("+str(float(benchtimes[cc_ref_name]['time']))+")")
		outfile.write("\n")
	outfile.write("\n")
	
	for wsname, benchtimes in wstimes.iteritems():
		outfile.write("Details for workspace %s:\n" % wsname)
		outfile.write("---------------------\n")
		for benchname, res in benchtimes.iteritems():
			outfile.write("Compiler configuration: "+benchname+"\n")
			outfile.write("CC command: "+res['cc_cmd']+"\n")
			outfile.write("CC stderr output: "+res['cc_stderr']+"\n\n")
		outfile.write("\n")



if len(errors) > 0:
	outfile.write("\nErrors:\n")
	outfile.write("-------\n\n")
	outfile.write("\n".join(errors))
	exit(1)
