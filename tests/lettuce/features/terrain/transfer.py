# Copyright (C) 2011  Internet Systems Consortium.
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

# This script provides transfer (ixfr/axfr) test functionality
# It provides steps to perform the client side of a transfer,
# and inspect the results.
#
# Like querying.py, it uses dig to do the transfers, and
# places its output in a result structure.  It also uses a custom client
# implementation for less normal operations.
#
# This is done in a different file with different steps than
# querying, because the format of dig's output is
# very different than that of normal queries

from lettuce import *
import subprocess
import re

class TransferResult(object):
    """This object stores transfer results, which is essentially simply
       a list of RR strings. These are stored, as read from dig's output,
       in the list 'records'. So for an IXFR transfer it contains
       the exact result as returned by the server.
       If this list is empty, the transfer failed for some reason (dig
       does not really show error results well, unfortunately).
       We may add some smarter inspection functionality to this class
       later.
    """
    def __init__(self, args):
        """Perform the transfer by calling dig, and store the results.
           args is the array of arguments to pass to Popen(), this
           is passed as is since for IXFR and AXFR there can be very
           different options"""
        self.records = []

        # Technically, using a pipe here can fail; since we don't expect
        # large output right now, this works, but should we get a test
        # where we do have a lot of output, this could block, and we will
        # need to read the output in a different way.
        dig_process = subprocess.Popen(args, 1, None, None, subprocess.PIPE,
                                       None)
        result = dig_process.wait()
        assert result == 0
        for l in dig_process.stdout:
            line = l.strip()
            if len(line) > 0 and line[0] != ';':
                self.records.append(line)

def parse_addr_port(address, port):
    if address is None:
        address = "::1"   # default address
    # convert [IPv6_addr] to IPv6_addr:
    address = re.sub(r"\[(.+)\]", r"\1", address)
    if port is None:
        port = 56176            # default port
    return (address, port)

@step('An AXFR transfer of ([\w.]+)(?: from ([\d.]+|\[[0-9a-fA-F:]+\])(?::([0-9]+))?)?')
def perform_axfr(step, zone_name, address, port):
    """
    Perform an AXFR transfer, and store the result as an instance of
    TransferResult in world.transfer_result.

    Step definition:
    An AXFR transfer of <zone_name> [from <address>:<port>]

    Address defaults to ::1
    Port defaults to 56176
    """
    (address, port) = parse_addr_port(address, port)
    args = [ 'dig', 'AXFR', '@' + str(address), '-p', str(port), zone_name ]
    world.transfer_result = TransferResult(args)

@step('A customized AXFR transfer of ([\w.]+)(?: from ([\d.]+|\[[0-9a-fA-F:]+\])(?::([0-9]+))?)?(?: with pause of (\d+) seconds?)?')
def perform_custom_axfr(step, zone_name, address, port, delay):
    """Checks AXFR transfer, and store the result in the form of internal
    CustomTransferResult class, which is compatible with TransferResult.

    Step definition:
    A customized AXFR transfer of <zone_name> [from <address>:<port>] [with pause of <delay> second]

    If optional delay is specified (not None), it waits for the specified
    seconds after sending the AXFR query before starting receiving
    responses.  This emulates a slower AXFR client.

    """

    class CustomTransferResult:
        """Store transfer result only on the number of received answer RRs.

        To be compatible with TransferResult it stores the result in the
        'records' attribute, which is a list.  But its content is
        meaningless; its only use is to be used with
        check_transfer_result_count where its length is of concern.

        """
        def __init__(self):
            self.records = []

    # Build arguments and run xfr-client.py.  On success, it simply dumps
    # the number of received answer RRs to stdout.
    (address, port) = parse_addr_port(address, port)
    args = ['/bin/sh', 'run_python-tool.sh', 'tools/xfr-client.py',
            '-s', address, '-p', str(port)]
    if delay is not None:
        args.extend(['-d', delay])
    args.append(zone_name)
    client = subprocess.Popen(args, 1, None, None, subprocess.PIPE,
                              subprocess.PIPE)
    (stdout, stderr) = client.communicate()
    result = client.returncode
    world.last_client_stdout = stdout
    world.last_client_stderr = stderr
    assert result == 0, "xfr-client exit code: " + str(result) +\
                        "\nstdout:\n" + str(stdout) +\
                        "stderr:\n" + str(stderr)
    num_rrs = int(stdout.strip())

    # Make the result object, storing dummy value (None) for the number of
    # answer RRs in the records list.
    world.transfer_result = CustomTransferResult()
    world.transfer_result.records = [None for _ in range(0, num_rrs)]

@step('An IXFR transfer of ([\w.]+) (\d+)(?: from ([^:]+)(?::([0-9]+))?)?(?: over (tcp|udp))?')
def perform_ixfr(step, zone_name, serial, address, port, protocol):
    """
    Perform an IXFR transfer, and store the result as an instance of
    TransferResult in world.transfer_result.

    Step definition:
    An IXFR transfer of <zone_name> <serial> [from <address>:port] [over <tcp|udp>]

    Address defaults to 127.0.0.1
    Port defaults to 56176
    If either tcp or udp is specified, only this protocol will be used.
    """
    if address is None:
        address = "127.0.0.1"
    if port is None:
        port = 56176
    args = [ 'dig', 'IXFR=' + str(serial), '@' + str(address), '-p', str(port), zone_name ]
    if protocol is not None:
        assert protocol == 'tcp' or protocol == 'udp', "Unknown protocol: " + protocol
        if protocol == 'tcp':
            args.append('+tcp')
        elif protocol == 'udp':
            args.append('+notcp')
    world.transfer_result = TransferResult(args)

@step('transfer result should have (\d+) rrs?')
def check_transfer_result_count(step, number_of_rrs):
    """
    Check the number of rrs in the transfer result object created by
    the AXFR transfer or IXFR transfer step.

    Step definition:
    transfer result should have <number> rr[s]

    Fails if the number of RRs is not equal to number
    """
    assert int(number_of_rrs) == len(world.transfer_result.records),\
        "Got " + str(len(world.transfer_result.records)) +\
        " records, expected " + str(number_of_rrs)

@step('full result of the last transfer should be')
def check_full_transfer_result(step):
    """
    Check the complete output from the last transfer call.

    Step definition:
    full result of the last transfer should be <multiline value>

    Whitespace is normalized in both the multiline value and the
    output, but the order of the output is not.
    Fails if there is any difference between the two. Prints
    full output and expected value upon failure.
    """
    records_string = "\n".join(world.transfer_result.records)
    records_string = re.sub("[ \t]+", " ", records_string)
    expect = re.sub("[ \t]+", " ", step.multiline)
    assert records_string.strip() == expect.strip(),\
        "Got:\n'" + records_string + "'\nExpected:\n'" + expect + "'"
