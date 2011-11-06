/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2011, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                Ken Stevens, Steve McClure, Markus Armbruster
 *
 *  Empire is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  files.c: Create all the misc files
 *
 *  Known contributors to this file:
 *     Thomas Ruschak
 *     Ken Stevens, 1995
 *     Steve McClure, 1998
 *     Markus Armbruster, 2004-2011
 */

#include <config.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "file.h"
#include "game.h"
#include "land.h"
#include "misc.h"
#include "nat.h"
#include "nuke.h"
#include "optlist.h"
#include "plane.h"
#include "power.h"
#include "prototypes.h"
#include "sect.h"
#include "ship.h"
#include "tel.h"
#include "trade.h"
#include "version.h"

static void
print_usage(char *program_name)
{
    printf("Usage: %s [OPTION]...\n"
	   "  -e CONFIG-FILE  configuration file\n"
	   "                  (default %s)\n"
	   "  -f              force overwrite of existing game\n"
	   "  -h              display this help and exit\n"
	   "  -v              display version information and exit\n",
	   program_name, dflt_econfig);
}

int
main(int argc, char *argv[])
{
    char buf[255];
    char *filename;
    struct natstr nat;
    int i;
    int opt;
    char *config_file = NULL;
    int force = 0;

    while ((opt = getopt(argc, argv, "e:fhv")) != EOF) {
	switch (opt) {
	case 'e':
	    config_file = optarg;
	    break;
	case 'f':
	    force = 1;
	    break;
	case 'h':
	    print_usage(argv[0]);
	    exit(0);
	case 'v':
	    printf("%s\n\n%s", version, legal);
	    exit(0);
	default:
	    fprintf(stderr, "Try -h for help.\n");
	    exit(1);
	}
    }

    if (argv[optind]) {
	fprintf(stderr, "%s: does not take operands\n", argv[0]);
	fprintf(stderr, "Try -h for help.\n");
	exit(1);
    }

    empfile_init();
    if (emp_config(config_file) < 0)
	exit(1);
    empfile_fixup();

    if (mkdir(gamedir, S_IRWXU | S_IRWXG) < 0 && errno != EEXIST) {
	fprintf(stderr, "Can't make game directory %s (%s)\n",
		gamedir, strerror(errno));
	exit(1);
    }
    if (chdir(gamedir)) {
	fprintf(stderr, "Can't chdir to %s (%s)\n",
		gamedir, strerror(errno));
	exit(EXIT_FAILURE);
    }

    if (!force) {
	printf("WARNING: this blasts the existing game in %s (if any)\n",
	       gamedir);
	printf("continue? ");
	if (!fgets(buf, sizeof(buf), stdin) || (*buf != 'y' && *buf != 'Y'))
	    exit(1);
    }

    for (i = 0; i < EF_MAX; i++) {
	if (!EF_IS_GAME_STATE(i))
	    continue;
	if (!ef_open(i, EFF_CREATE | EFF_NOTIME))
	    exit(1);
    }

    ef_read(EF_NATION, 0, &nat);
    strcpy(nat.nat_cnam, "POGO");
    strcpy(nat.nat_pnam, "peter");
    nat.nat_stat = STAT_GOD;
    nat.nat_btu = 255;
    nat.nat_money = 123456789;
    nat.nat_flags =
	NF_FLASH | NF_BEEP | NF_COASTWATCH | NF_SONAR | NF_TECHLISTS;
    ef_write(EF_NATION, 0, &nat);
    printf("All praise to %s!\n", nat.nat_cnam);
    if (mkdir(teldir, S_IRWXU | S_IRWXG) < 0 && errno != EEXIST) {
	fprintf(stderr, "Can't make telegram directory %s (%s)\n",
		teldir, strerror(errno));
	exit(1);
    }
    for (i = 0; i < MAXNOC; i++) {
	filename = mailbox(buf, i);
	close(creat(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));
    }
    close(creat(annfil, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP));

    for (i = 0; i < EF_MAX; i++) {
	if (!EF_IS_GAME_STATE(i))
	    continue;
	ef_close(i);
    }

    exit(0);
}
