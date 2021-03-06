# Copyright (C) 2013  Internet Systems Consortium.
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SYSTEMS CONSORTIUM
# DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
# INTERNET SYSTEMS CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
# FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import errno
import os
import select
import signal

import bundy_config
import bundy.log
import bundy.config
from bundy.server_common.logger import logger
from bundy.log_messages.server_common_messages import *

class BUNDYServerFatal(Exception):
    """Exception raised when the server program encounters a fatal error."""
    pass

class BUNDYServer:
    """A mixin class for common BUNDY server implementations.

    It takes care of common initialization such as setting up a module CC
    session, and running main event loop.  It also handles the "shutdown"
    command for its normal behavior.  If a specific server class wants to
    handle this command differently or if it does not support the command,
    it should override the _command_handler method.

    Specific modules can define module-specific class inheriting this class,
    instantiate it, and call run() with the module name.

    Methods to be implemented in the actual class:
      _config_handler: config handler method as specified in ModuleCCSession.
                       must be exception free; errors should be signaled by
                       the return value.
      _mod_command_handler: can be optionally defined to handle
                            module-specific commands.  should conform to
                            command handlers as specified in ModuleCCSession.
                            must be exception free; errors should be signaled
                            by the return value.
      _setup_module: can be optionally defined for module-specific
                     initialization.  This is called after the module CC
                     session has started, and can be used for registering
                     interest on remote modules, etc.  If it raises an
                     exception, the server will be immediately stopped.
                     Parameter: None, Return: None
      _shutdown_module: can be optionally defined for module-specific
                        finalization. This is called right before the
                        module CC session is stopped. If it raises an
                        exception, the server will be immediately
                        stopped.
                        Parameter: None, Return: None

    """
    # Will be set to True when the server should stop and shut down.
    # Can be read via accessor method 'shutdown', mainly for testing.
    __shutdown = False

    # ModuleCCSession used in the server.  Defined as 'protectd' so tests
    # can refer to it directly; others should access it via the
    # 'mod_ccsession' accessor.
    _mod_cc = None

    # Will be set in run().  Define a tentative value so other methods can
    # be tested directly.
    __module_name = ''

    # Basically constant, but allow tests to override it.
    _select_fn = select.select

    def __init__(self):
        self._read_callbacks = {}
        self._write_callbacks = {}
        self._error_callbacks = {}

    @property
    def shutdown(self):
        return self.__shutdown

    @property
    def mod_ccsession(self):
        return self._mod_cc

    def _setup_ccsession(self):
        """Create and start module CC session.

        This is essentially private, but allows tests to override it.

        """
        self._mod_cc = bundy.config.ModuleCCSession(
            bundy_config.get_specfile_location(self.__module_name),
            self._config_handler, self._command_handler)
        self._mod_cc.start()

    def _trigger_shutdown(self):
        """Initiate a shutdown sequence.

        This method is expected to be called in various ways including
        in the middle of a signal handler, and is designed to be as simple
        as possible to minimize side effects.  Actual shutdown will take
        place in a normal control flow.

        This method is defined as 'protected'.  User classes can use it
        to shut down the server.

        """
        self.__shutdown = True

    def _run_internal(self):
        """Main event loop.

        This method is essentially private, but allows tests to override it.

        """

        logger.info(PYSERVER_COMMON_SERVER_STARTED, self.__module_name)
        cc_fileno = self._mod_cc.get_socket().fileno()
        while not self.__shutdown:
            try:
                read_fds = list(self._read_callbacks.keys())
                read_fds.append(cc_fileno)
                write_fds = list(self._write_callbacks.keys())
                error_fds = list(self._error_callbacks.keys())

                (reads, writes, errors) = \
                    self._select_fn(read_fds, write_fds, error_fds)
            except select.error as ex:
                # ignore intterruption by signal; regard other select errors
                # fatal.
                if ex.args[0] == errno.EINTR:
                    continue
                else:
                    raise

            for fileno in reads:
                if fileno in self._read_callbacks:
                    for callback in self._read_callbacks[fileno]:
                        callback()

            for fileno in writes:
                if fileno in self._write_callbacks:
                    for callback in self._write_callbacks[fileno]:
                        callback()

            for fileno in errors:
                if fileno in self._error_callbacks:
                    for callback in self._error_callbacks[fileno]:
                        callback()

            if cc_fileno in reads:
                # this shouldn't raise an exception (if it does, we'll
                # propagate it)
                self._mod_cc.check_command(True)

        self._shutdown_module()
        self._mod_cc.send_stopping()

    def _command_handler(self, cmd, args):
        logger.debug(logger.DBGLVL_TRACE_BASIC, PYSERVER_COMMON_COMMAND,
                     self.__module_name, cmd)
        if cmd == 'shutdown':
            self._trigger_shutdown()
            answer = bundy.config.create_answer(0)
        else:
            answer = self._mod_command_handler(cmd, args)

        return answer

    def _mod_command_handler(self, cmd, args):
        """The default implementation of the module specific command handler"""
        return bundy.config.create_answer(1, "Unknown command: " + str(cmd))

    def _setup_module(self):
        """The default implementation of the module specific initialization"""
        pass

    def _shutdown_module(self):
        """The default implementation of the module specific finalization"""
        pass

    def watch_fileno(self, fileno, rcallback=None, wcallback=None,
                     xcallback=None):
        """Register the fileno for the internal select() call.

        *callback's are callable objects which would be called when
        read, write, error events occur on the specified fileno.
        """
        if rcallback is not None:
            if fileno in self._read_callbacks:
                self._read_callbacks[fileno].append(rcallback)
            else:
                self._read_callbacks[fileno] = [rcallback]

        if wcallback is not None:
            if fileno in self._write_callbacks:
                self._write_callbacks[fileno].append(wcallback)
            else:
                self._write_callbacks[fileno] = [wcallback]

        if xcallback is not None:
            if fileno in self._error_callbacks:
                self._error_callbacks[fileno].append(xcallback)
            else:
                self._error_callbacks[fileno] = [xcallback]

    def unwatch_fileno(self, fileno, on_read, on_write, on_error):
        """Unregister a fileno for the internal select() call.

        This is a cancel operation for callbacks registered in watch_fileno().
        Currently, on_xxx parameters are expected to be bool, and if True
        all callbacks for 'fileno' will be removed.  If we see the need in
        future, this can be extended so we can specify a particular callback
        to cancel.

        At least one callback must be specified for 'fileno' corresponding to
        on_xxx that is True.  Otherwise ValueError will be called.  This
        also means this method cannot be called multiple times for the
        same 'fileno' and same operation (read/write/error).

        This method can be safely called from a callback function for the
        corresponding operation.

        """
        if on_read:
            if not fileno in self._read_callbacks.keys():
                raise ValueError('fileno not watched for read: ' + str(fileno))
            del self._read_callbacks[fileno]
        if on_write:
            if not fileno in self._write_callbacks.keys():
                raise ValueError('fileno not watched for write: ' + str(fileno))
            del self._write_callbacks[fileno]
        if on_error:
            if not fileno in self._error_callbacks.keys():
                raise ValueError('fileno not watched for error: ' + str(fileno))
            del self._error_callbacks[fileno]

    def run(self, module_name):
        """Start the server and let it run until it's told to stop.

        Usually this must be the first method of this class that is called
        from its user.

        Parameter:
          module_name (str): the Python module name for the actual server
            implementation.  Often identical to the directory name in which
            the implementation files are placed.

        Returns: values expected to be used as program's exit code.
          0: server has run and finished successfully.
          1: some error happens

          """
        try:
            self.__module_name = module_name
            shutdown_sighandler = \
                lambda signal, frame: self._trigger_shutdown()
            signal.signal(signal.SIGTERM, shutdown_sighandler)
            signal.signal(signal.SIGINT, shutdown_sighandler)
            self._setup_ccsession()
            self._setup_module()
            self._run_internal()
            logger.info(PYSERVER_COMMON_SERVER_STOPPED, self.__module_name)
            return 0
        except BUNDYServerFatal as ex:
            logger.error(PYSERVER_COMMON_SERVER_FATAL, self.__module_name,
                         ex)
        except Exception as ex:
            logger.error(PYSERVER_COMMON_UNCAUGHT_EXCEPTION, type(ex).__name__,
                         ex)

        return 1
