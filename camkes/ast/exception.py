#!/usr/bin/env python
# -*- coding: utf-8 -*-

#
# Copyright 2015, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

from __future__ import absolute_import, division, print_function, \
    unicode_literals
from camkes.internal.seven import cmp, filter, map, zip

class ASTError(Exception):
    '''
    An error resulting from inconsistency detected in the AST.
    '''
    def __init__(self, message, entity):
        self.entity = entity
        msg = []
        filename = entity.filename
        if filename is not None:
            msg.extend([filename, ':'])
        lineno = entity.lineno
        if lineno is not None:
            msg.extend([str(lineno), ':'])
        if len(msg) > 0:
            msg.append(' ')
        msg.append(message)
        super(ASTError, self).__init__(''.join(msg))