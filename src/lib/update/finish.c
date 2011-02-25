/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2010, Dave Pare, Jeff Bailey, Thomas Ruschak,
 *                           Ken Stevens, Steve McClure
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  ---
 *
 *  See files README, COPYING and CREDITS in the root of the source
 *  tree for related information and legal notices.  It is expected
 *  that future projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  finish.c: Finish the update
 *
 *  Known contributors to this file:
 *     Dave Pare, 1986
 *     Thomas Ruschak, 1993
 *     Steve McClure, 1998
 *     Markus Armbruster, 2004-2011
 */

#include <config.h>

#include <stdlib.h>
#include <sys/resource.h>
#include "distribute.h"
#include "path.h"
#include "update.h"

static void assemble_dist_paths(double *);

void
finish_sects(int etu)
{
    static double *import_cost;
    struct sctstr *sp;
    struct natstr *np;
    int n;
    struct rusage rus1, rus2;

    if (import_cost == NULL) {
	logerror("First update since reboot, allocating buffer\n");
	import_cost = malloc(WORLD_SZ() * sizeof(*import_cost));
	if (import_cost == NULL) {
	    logerror("malloc failed in finish_sects.\n");
	    return;
	}
    }

    logerror("delivering...\n");
    /* Do deliveries */
    for (n = 0; NULL != (sp = getsectid(n)); n++) {
	if (sp->sct_type == SCT_WATER)
	    continue;
	if (sp->sct_own == 0)
	    continue;
	np = getnatp(sp->sct_own);
	if (np->nat_money < 0)
	    continue;
	dodeliver(sp);
    }
    logerror("done delivering\n");

    logerror("assembling paths...\n");
    getrusage(RUSAGE_SELF, &rus1);

    /* First, enable the best_path cacheing */
    bp_enable_cachepath();

    /* Now assemble the paths */
    assemble_dist_paths(import_cost);

    /* Now disable the best_path cacheing */
    bp_disable_cachepath();

    /* Now, clear the best_path cache that may have been created */
    bp_clear_cachepath();

    getrusage(RUSAGE_SELF, &rus2);
    logerror("done assembling paths %g user %g system",
	     rus2.ru_utime.tv_sec + rus2.ru_utime.tv_usec / 1e6
	     - (rus1.ru_utime.tv_sec + rus1.ru_utime.tv_usec / 1e6),
	     rus2.ru_stime.tv_sec + rus2.ru_stime.tv_usec / 1e6
	     - (rus1.ru_stime.tv_sec + rus1.ru_stime.tv_usec / 1e6));

    logerror("exporting...");
    for (n = 0; NULL != (sp = getsectid(n)); n++) {
	if (sp->sct_type == SCT_WATER || sp->sct_own == 0)
	    continue;
	np = getnatp(sp->sct_own);
	if (np->nat_money < 0)
	    continue;
	dodistribute(sp, EXPORT, import_cost[n]);
    }
    logerror("done exporting\n");

    logerror("importing...");
    for (n = 0; NULL != (sp = getsectid(n)); n++) {
	if (sp->sct_type == SCT_WATER || sp->sct_own == 0)
	    continue;
	np = getnatp(sp->sct_own);
	if (np->nat_money < 0)
	    continue;
	dodistribute(sp, IMPORT, import_cost[n]);
	sp->sct_off = 0;
    }
    logerror("done importing\n");

}

static void
assemble_dist_paths(double *import_cost)
{
    char *path;
    double d;
    struct sctstr *sp;
    struct sctstr *dist;
    int n;
    char buf[512];

    for (n = 0; NULL != (sp = getsectid(n)); n++) {
	import_cost[n] = -1;
	if ((sp->sct_dist_x == sp->sct_x) && (sp->sct_dist_y == sp->sct_y))
	    continue;
	dist = getsectp(sp->sct_dist_x, sp->sct_dist_y);
	if (CANT_HAPPEN(!dist))
	    continue;
	if (sp->sct_own != dist->sct_own)
	    continue;
	/* Now, get the best distribution path over roads */
	/* Note we go from the dist center to the sector.  This gives
	   us the import path for that sector. */
	path = BestDistPath(buf, dist, sp, &d);
	if (path)
	    import_cost[n] = d;
    }
}
