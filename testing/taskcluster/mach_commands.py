from __future__ import print_function, unicode_literals

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

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
        if commit is None:
            print("TODO: Parse stuff from hg")
        print(commit)
