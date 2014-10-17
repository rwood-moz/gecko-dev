# -*- coding: utf-8 -*-

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
from __future__ import print_function, unicode_literals

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

import taskcluster_graph
import os

ROOT=os.path.dirname(os.path.realpath(__file__))

@CommandProvider
class TryGraph(object):
    @Command('trygraph', category="ci",
        description="Create taskcluster try server graph")
    @CommandArgument('--commit',
        help='Commit message to be parsed')
    def create_graph(self, commit=""):
        """ Create the taskcluster graph from the try commit message.

        :param args: commit message (ex: "– try: -b o -p linux64_gecko -u gaia-unit -t none")
        """
        print(taskcluster_graph.load_task(
            os.path.join(ROOT, 'builds', 'b2g_desktop.yml')
        ))
