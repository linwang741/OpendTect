#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : A. Huck
# DATE     : Apr 2019
#
# tools for managing OS commands
#

import sys
import os
import signal
import subprocess
import json

from odpy.common import isWin, getExecPlfDir, get_log_stream, get_std_stream, std_msg

def getODCommand(execnm,args=None):
  cmd = list()
  cmd.append( os.path.join(getExecPlfDir(args),execnm) )
  return appendDtectArgs( cmd, args )

def appendDtectArgs( cmd, args=None ):
  if args == None:
    return cmd
  if 'dtectdata' in args:
    cmd.append( '--dataroot' )
    cmd.append( args['dtectdata'][0] )
  if 'survey' in args:
    cmd.append( '--survey' )
    cmd.append( args['survey'][0] )
  return cmd

def getPythonExecNm():
  return sys.executable

def getPythonCommand(scriptfile,posargs=None,dict=None,args=None):
  cmd = list()
  cmd.append( getPythonExecNm() )
  cmd.append( scriptfile )
  cmd = appendDtectArgs( cmd, args )
  cmd.append( '--dtectexec' )
  cmd.append( getExecPlfDir(args) )
  if args != None and 'proclog' in args:
    cmd.append( '--proclog' )
    cmd.append( args['proclog'] )
  if args != None and 'syslog' in args:
    cmd.append( '--syslog' )
    cmd.append( args['syslog'] )
  for posarg in posargs:
    cmd.append( posarg )
  if dict != None:
    cmd.append( '--dict' )
    cmd.append( json.dumps(dict) )
  return cmd

def execCommand( cmd, background=False ):
  if background:
    return startDetached( cmd )
  else:
    return startAndWait( cmd )

def isRunning( proc ):
  if isinstance(proc,subprocess.Popen):
    return proc.poll() == None and proc.returncode == None
  try:
    os.getpgid(proc)
  except OSError:
    return False
  return True

def killproc( proc=None ):
  if isinstance(proc,subprocess.Popen):
    proc.terminate()
    proc.wait(3)
    return proc.returncode
  if pid != None:
    os.kill( pid, signal.SIGTERM )
  return None

# INTERNAL, you should not need to use those:
def startAndWait( cmd ):
  try:
    completedproc = subprocess.run( cmd, check=True, stdout=subprocess.PIPE, \
                                    stderr=get_std_stream() ).stdout
  except subprocess.CalledProcessError as err:
    std_msg( 'Failed: ', err )
    raise
  return completedproc

def startDetached( cmd ):
 try:
   runningproc = subprocess.Popen( cmd, start_new_session=True, \
                                   stdout=get_log_stream(), \
                                   stderr=get_std_stream() )
 except subprocess.CalledProcessError as err:
   std_msg( 'Failed: ', err )
   raise
 return runningproc

