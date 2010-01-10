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
 *  reco.c: Fly a recon mission
 *
 *  Known contributors to this file:
 *     Dave Pare, 1986
 *     Markus Armbruster, 2004-2009
 */

#include <config.h>

#include "commands.h"
#include "path.h"
#include "plane.h"
#include "ship.h"

int
reco(void)
{
    int mission_flags = player->argp[0][0] == 's' ? PM_R | PM_S : PM_R;
    coord tx, ty;
    coord ax, ay;
    int ap_to_target;
    char flightpath[MAX_PATH_LEN];
    int cno;
    struct nstr_item ni_bomb;
    struct nstr_item ni_esc;
    struct sctstr target;
    struct emp_qelem bomb_list;
    struct emp_qelem esc_list;
    int wantflags;
    struct sctstr ap_sect;
    char buf[1024];

    wantflags = 0;
    if (get_planes(&ni_bomb, &ni_esc, player->argp[1], player->argp[2]) < 0)
	return RET_SYN;
    if (!get_assembly_point(player->argp[3], &ap_sect, buf))
	return RET_SYN;
    ax = ap_sect.sct_x;
    ay = ap_sect.sct_y;
    if (!getpath(flightpath, player->argp[4], ax, ay, 0, 0, P_FLYING)
	|| *flightpath == 0)
	return RET_SYN;
    tx = ax;
    ty = ay;
    (void)pathtoxy(flightpath, &tx, &ty, fcost);
    pr("target is %s\n", xyas(tx, ty, player->cnum));
    getsect(tx, ty, &target);
    cno = -1;
    if (pln_onewaymission(&target, &cno, &wantflags) < 0)
	return RET_SYN;
    ap_to_target = strlen(flightpath);
    if (flightpath[ap_to_target - 1] == 'h')
	ap_to_target--;
    pr("range to target is %d\n", ap_to_target);
    /*
     * select planes within range
     */
    pln_sel(&ni_bomb, &bomb_list, &ap_sect, ap_to_target, 1,
	    wantflags | (mission_flags & PM_S ? P_SWEEP : 0),
	    P_M | P_O);
    pln_sel(&ni_esc, &esc_list, &ap_sect, ap_to_target, 1,
	    wantflags | P_ESC | P_F, P_M | P_O);
    if (cno >= 0 && !pln_oneway_to_carrier_ok(&bomb_list, &esc_list, cno)) {
	pr("Not enough room on ship #%d!\n", cno);
	return RET_FAIL;
    }
    /*
     * now arm and equip the bombers, transports, whatever.
     */
    pln_arm(&bomb_list, ap_to_target, 'r', NULL);
    if (QEMPTY(&bomb_list)) {
	pr("No planes could be equipped for the mission.\n");
	return RET_FAIL;
    }
    pln_arm(&esc_list, ap_to_target, 'e', NULL);
    ac_encounter(&bomb_list, &esc_list, ax, ay, flightpath, mission_flags);
    if (QEMPTY(&bomb_list)) {
	pr("No planes got through fighter defenses\n");
    } else {
	getsect(tx, ty, &target);
	pln_newlanding(&bomb_list, tx, ty, cno);
	pln_newlanding(&esc_list, tx, ty, cno);
    }
    pln_put(&bomb_list);
    pln_put(&esc_list);
    return RET_OK;
}
