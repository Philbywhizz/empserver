/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2000, Dave Pare, Jeff Bailey, Thomas Ruschak,
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
 *  See the "LEGAL", "LICENSE", "CREDITS" and "README" files for all the
 *  related information and legal notices. It is expected that any future
 *  projects/authors will amend these files as needed.
 *
 *  ---
 *
 *  nxtsct.c: Select/get the next sector from a range of sectors
 * 
 *  Known contributors to this file:
 *     Dave Pare, 1989
 */

#include "misc.h"
#include "var.h"
#include "xy.h"
#include "sect.h"
#include "nsc.h"
#include "file.h"
#include "prototypes.h"
#include "optlist.h"

/*
 * get the next sector in the range
 * that matches the conditions.
 */
int
nxtsct(register struct nstr_sect *np, struct sctstr *sp)
{
    while (1) {
	np->dx++;
	np->x++;
	if (np->x >= WORLD_X)
	    np->x = 0;
	if (np->dx >= np->range.width) {
	    np->dx = 0;
	    np->x = np->range.lx;
	    np->dy++;
	    if (np->dy >= np->range.height)
		return 0;
	    np->y++;
	    if (np->y >= WORLD_Y)
		np->y = 0;
	}
	if ((np->y + np->x) & 01)
	    continue;
	if (np->type == NS_DIST) {
	    np->curdist = mapdist(np->x, np->y, np->cx, np->cy);
	    if (np->curdist > np->dist)
		continue;
	}
	np->id = sctoff(np->x, np->y);
	if (!np->read(EF_SECTOR, np->id, sp))
	    continue;
	if (np->ncond == 0)
	    return 1;
	if (nstr_exec(np->cond, np->ncond, sp))
	    return 1;
    }
    /*NOTREACHED*/
}
