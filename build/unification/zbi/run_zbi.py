#!/usr/bin/env python2.7
# Copyright 2020 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Runs the zbi tool, taking care of unwrapping response files.'''

import argparse
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--zbi',
                        help='Path to the zbi tool',
                        required=True)
    parser.add_argument('--original-depfile',
                        help='Path to the depfile generated by the zbi tool',
                        required=True)
    parser.add_argument('--final-depfile',
                        help='Path to the depfile generated for GN',
                        required=True)
    parser.add_argument('--rspfile',
                        help='Path to the response file for the zbi tool',
                        required=True)
    args, zbi_args = parser.parse_known_args()

    # Run the zbi tool.
    command = [
        args.zbi,
        '--depfile', args.original_depfile,
    ] + zbi_args
    with open(args.rspfile, 'r') as rspfile:
        command.extend([l.strip() for l in rspfile.readlines()])
    subprocess.check_output(command)

    # Write the final depfile.
    with open(args.original_depfile, 'r') as original_depfile:
        contents = original_depfile.read().strip()
    with open(args.final_depfile, 'w') as final_depfile:
        final_depfile.write(contents + ' ' + args.rspfile)


if __name__ == '__main__':
    main()
