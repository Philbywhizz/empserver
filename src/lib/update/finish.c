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
 */

#include <config.h>

#include <stdlib.h>
#include <sys/resource.h>
#include "distribute.h"
#include "path.h"
#include "update.h"

/* Used for building up distribution info */
struct distinfo {
    double imcost;		/* import cost */
    double excost;		/* export cost */
};

/* This is our global buffer of distribution pointers.  Note that
 * We only malloc this once, and never again (until reboot time
 * of course :) ) We do clear it each and every time. */
static struct distinfo *g_distptrs;

static void assemble_dist_paths(struct distinfo *distptrs);
static char *ReversePath(char *path);

void
finish_sects(int etu)
{
    struct sctstr *sp;
    struct natstr *np;
    int n;
    struct rusage rus1, rus2;
    struct distinfo *infptr;

    if (g_distptrs == NULL) {
	logerror("First update since reboot, allocating buffer\n");
	/* Allocate the information buffer */
	g_distptrs = malloc(WORLD_SZ() * sizeof(*g_distptrs));
	if (g_distptrs == NULL) {
	    logerror("malloc failed in finish_sects.\n");
	    return;
	}
    }

    /* Wipe it clean */
    memset(g_distptrs, 0, WORLD_SZ() * sizeof(*g_distptrs));

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
    assemble_dist_paths(g_distptrs);

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
	/* Get the pointer */
	infptr = &g_distptrs[sp->sct_uid];
	dodistribute(sp, EXPORT, infptr->imcost, infptr->excost);
    }
    logerror("done exporting\n");

    logerror("importing...");
    for (n = 0; NULL != (sp = getsectid(n)); n++) {
	/* Get the pointer (we do it first so we can free if needed) */
	infptr = &g_distptrs[sp->sct_uid];
	if (sp->sct_type == SCT_WATER || sp->sct_own == 0)
	    continue;
	np = getnatp(sp->sct_own);
	if (np->nat_money < 0)
	    continue;
	dodistribute(sp, IMPORT, infptr->imcost, infptr->excost);
	sp->sct_off = 0;
    }
    logerror("done importing\n");

}

static void
assemble_dist_paths(struct distinfo *distptrs)
{
    char *path, *p;
    double d;
    struct sctstr *sp;
    struct sctstr *dist;
    struct distinfo *infptr;
    int n;
    char buf[512];

    for (n = 0; NULL != (sp = getsectid(n)); n++) {
	if ((sp->sct_dist_x == sp->sct_x) && (sp->sct_dist_y == sp->sct_y))
	    continue;
	/* Set the pointer */
	infptr = &distptrs[sp->sct_uid];
	/* now, get the dist sector */
	dist = getsectp(sp->sct_dist_x, sp->sct_dist_y);
	if (dist == NULL) {
	    logerror("Bad dist sect %d,%d for %d,%d !\n",
		     sp->sct_dist_x, sp->sct_dist_y,
		     sp->sct_x, sp->sct_y);
	    continue;
	}
	/* Now, get the best distribution path over roads */
	/* Note we go from the dist center to the sector.  This gives
	   us the import path for that sector. */
	path = BestDistPath(buf, dist, sp, &d);

	/* Now, we have a path */
	if (!path)
	    infptr->imcost = infptr->excost = -1.0;
	else {
	    /* Save the import cost */
	    infptr->imcost = d;
	    /* Now, reverse the path */
	    p = ReversePath(path);
	    /* And walk the path back to the dist center to get the export
	       cost */
	    infptr->excost = pathcost(sp, p, MOB_MOVE);
	}
    }
}

static char *
ReversePath(char *path)
{
    char *patharray = "aucdefjhigklmyopqrstbvwxnz";
    static char new_path[512];
    int ind;

    if (path == NULL)
	return NULL;

    ind = strlen(path);
    if (ind == 0)
	return NULL;

    if (path[ind - 1] == 'h')
	ind--;

    new_path[ind--] = '\0';
    new_path[ind] = '\0';

    while (ind >= 0) {
	new_path[ind--] = patharray[*(path++) - 'a'];
    }

    return new_path;
}
