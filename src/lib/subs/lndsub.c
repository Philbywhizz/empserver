/*
 *  Empire - A multi-player, client/server Internet based war game.
 *  Copyright (C) 1986-2006, Dave Pare, Jeff Bailey, Thomas Ruschak,
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
 *  lndsub.c: Land unit subroutines
 * 
 *  Known contributors to this file:
 *     Ken Stevens, 1995
 *     Steve McClure, 1998-2000
 */

#include <config.h>

#include <math.h>
#include "misc.h"
#include "player.h"
#include "file.h"
#include "sect.h"
#include "path.h"
#include "news.h"
#include "nat.h"
#include "xy.h"
#include "land.h"
#include "ship.h"
#include "nsc.h"
#include "mission.h"
#include "plane.h"
#include "combat.h"
#include "damage.h"
#include "optlist.h"
#include "prototypes.h"

static void lnd_mess(char *, struct llist *);
static int lnd_hit_mine(struct lndstr *, struct lchrstr *);

double
attack_val(int combat_mode, struct lndstr *lp)
{
    int men;
    double value;
    struct lchrstr *lcp;

    if (lp->lnd_effic < LAND_MINEFF) {
	putland(lp->lnd_uid, lp);
	return 0;
    }

    lcp = &lchr[(int)lp->lnd_type];

/* Spies always count as 1 during assaults.  If they are the only ones
   in the assault, they get to sneak on anyway. */

    if (lcp->l_flags & L_SPY && combat_mode == A_ASSAULT)
	return 1;

    men = lp->lnd_item[I_MILIT];
    value = men * lp->lnd_att * lp->lnd_effic / 100.0;

    switch (combat_mode) {
    case A_ATTACK:
	return value;
    case A_ASSAULT:
	if (!(lcp->l_flags & L_MARINE))
	    return assault_penalty * value;
	break;
    case A_BOARD:
	if (!(lcp->l_flags & L_MARINE))
	    return assault_penalty * men;
    }

    return value;
}

double
defense_val(struct lndstr *lp)
{
    int men;
    double value;
    struct lchrstr *lcp;

    if (lp->lnd_effic < LAND_MINEFF) {
	putland(lp->lnd_uid, lp);
	return 0;
    }

    lcp = &lchr[(int)lp->lnd_type];

    men = lp->lnd_item[I_MILIT];

    if ((lp->lnd_ship >= 0 || lp->lnd_land >= 0) &&
	!(lcp->l_flags & L_MARINE))
	return men;

    value = men * lp->lnd_def * lp->lnd_effic / 100.0;
    value *= ((double)land_mob_max + lp->lnd_harden) / land_mob_max;

    /* If there are military on the unit, you get at least a 1
       man defensive unit, except for spies */
    if (value < 1.0 && men > 0 && !(lcp->l_flags & L_SPY))
	return 1;

    return value;
}

void
lnd_print(struct llist *llp, char *s)
{
    if (llp->land.lnd_own == player->cnum)
	pr("%s %s\n", prland(&llp->land), s);
    else
	wu(0, llp->land.lnd_own, "%s %s\n", prland(&llp->land), s);
}

void
lnd_delete(struct llist *llp, char *s)
{
    if (s)
	lnd_print(llp, s);
    putland(llp->land.lnd_uid, &llp->land);
    emp_remque((struct emp_qelem *)llp);
    free(llp);
}

int
lnd_take_casualty(int combat_mode, struct llist *llp, int cas)
			/* attacking or assaulting or paratrooping? */
			/* number of casualties to take */
{
    int eff_eq;
    int n;
    int biggest;
    int civs;
    int nowned;
    coord ret_x, ret_y;
    coord bx, by;
    struct sctstr sect;
    int ret_chance;
    char buf[1024];
    int taken;
    int nowhere_to_go = 0;
    struct sctstr rsect;
    double mobcost, bmcost;
    signed char orig;
    int mob;

    taken = llp->land.lnd_item[I_MILIT];
    /* Spies always die */
    if (llp->lcp->l_flags & L_SPY) {
	eff_eq = 100;
	llp->land.lnd_effic = 0;
    } else {
	eff_eq = ldround(cas * 100.0 / llp->lcp->l_mil, 1);
	llp->land.lnd_effic -= eff_eq;
	lnd_submil(&llp->land, cas);
    }

    if (llp->land.lnd_effic < LAND_MINEFF) {
	sprintf(buf, "dies %s %s!",
		combat_mode ? att_mode[combat_mode] : "defending",
		xyas(llp->land.lnd_x, llp->land.lnd_y, llp->land.lnd_own));
	lnd_delete(llp, buf);
	/* Since we killed the unit, we killed all the mil on it */
	return taken;
    } else {
	/* Ok, now, how many did we take off? (sould be the diff) */
	taken = taken - llp->land.lnd_item[I_MILIT];
    }

    if (llp->land.lnd_effic >= llp->land.lnd_retreat)
	return taken;

    /* we're being boarded */
    if (llp->land.lnd_ship >= 0 && combat_mode == A_DEFEND)
	return taken;

    /* we're being boarded */
    if (llp->land.lnd_land >= 0 && combat_mode == A_DEFEND)
	return taken;

    /* Have to make a retreat check */

    ret_chance = llp->land.lnd_retreat - llp->land.lnd_effic;
    if (roll(100) < ret_chance) {
	pr("\n");
	lnd_print(llp, "fails morale check!");
	llp->land.lnd_mission = 0;
	llp->land.lnd_harden = 0;
	if (llp->land.lnd_ship >= 0 || llp->land.lnd_land >= 0)
	    nowhere_to_go = 1;
	else if (combat_mode == A_DEFEND) {
	    /*
	     * defending unit.. find a place to send it
	     * strategy: look for the most-populated 
	     * adjacent sector that is owned by the unit
	     * player->owner. Charge mob..
	     */
	    biggest = -1;
	    nowned = 0;
	    for (n = 1; n <= 6; ++n) {
		ret_x = llp->land.lnd_x + diroff[n][0];
		ret_y = llp->land.lnd_y + diroff[n][1];
		getsect(ret_x, ret_y, &sect);
		if (sect.sct_own != llp->land.lnd_own)
		    continue;
		if (sect.sct_type == SCT_MOUNT)
		    continue;
		mobcost = lnd_mobcost(&llp->land, &rsect);
		if (mobcost < 0)
		    continue;
		++nowned;
		civs = sect.sct_item[I_CIVIL];
		if (civs > biggest) {
		    biggest = civs;
		    bx = sect.sct_x;
		    by = sect.sct_y;
		    bmcost = mobcost;
		}
	    }
	    if (!nowned)
		nowhere_to_go = 1;
	    else {
		/* retreat to bx,by */
		llp->land.lnd_x = bx;
		llp->land.lnd_y = by;
		/* FIXME landmines */
		getsect(bx, by, &rsect);
		mob = llp->land.lnd_mobil - (int)bmcost;
		if (mob < -127)
		    mob = -127;
		orig = llp->land.lnd_mobil;
		llp->land.lnd_mobil = (signed char)mob;
		if (llp->land.lnd_mobil > orig)
		    llp->land.lnd_mobil = -127;
		sprintf(buf, "retreats at %d%% efficiency to %s!",
			llp->land.lnd_effic,
			xyas(bx, by, llp->land.lnd_own));
		lnd_delete(llp, buf);
	    }
	} else {		/* attacking from a sector */
	    sprintf(buf, "leaves the battlefield at %d%% efficiency",
		    llp->land.lnd_effic);
	    if ((llp->land.lnd_mobil - (int)llp->mobil) < -127)
		llp->land.lnd_mobil = -127;
	    else
		llp->land.lnd_mobil -= (int)llp->mobil;
	    llp->mobil = 0.0;
	    lnd_delete(llp, buf);
	}
    }
    if (nowhere_to_go) {
	/* nowhere to go.. take more casualties */
	llp->land.lnd_effic -= 10;
	lnd_submil(&llp->land, llp->lcp->l_mil / 10);
	if (llp->land.lnd_effic < LAND_MINEFF)
	    lnd_delete(llp, "has nowhere to retreat, and dies!");
	else
	    lnd_print(llp,
		      "has nowhere to retreat and takes extra losses!");
    }

    return taken;
}

void
lnd_takemob(struct emp_qelem *list, double loss)
{
    struct emp_qelem *qp, *next;
    struct llist *llp;
    int new;
    int mcost = ldround(combat_mob * loss, 1);

    for (qp = list->q_forw; qp != list; qp = next) {
	next = qp->q_forw;
	llp = (struct llist *)qp;
/*
		if (chance(loss))
			use_supply(&llp->land);
		if (llp->land.lnd_mission == MI_RESERVE)
			new = llp->land.lnd_mobil - mcost/2;
		else
 */
	new = llp->land.lnd_mobil - mcost;
	if (new < -127)
	    new = -127;
	llp->land.lnd_mobil = (signed char)new;
    }
}

void
lnd_submil(struct lndstr *lp, int num)
{
    int new = lp->lnd_item[I_MILIT] - num;
    lp->lnd_item[I_MILIT] = new < 0 ? 0 : new;
}

int
lnd_spyval(struct lndstr *lp)
{
    if (lchr[(int)lp->lnd_type].l_flags & L_RECON)
	return lp->lnd_spy * (lp->lnd_effic / 100.0) + 2;
    else
	return lp->lnd_spy * (lp->lnd_effic / 100.0);
}

double
intelligence_report(int destination, struct lndstr *lp, int spy,
		    char *mess)
{
    struct lchrstr *lcp;
    char buf1[80], buf2[80], buf3[80];
    double estimate = 0.0;	/* estimated defense value */

    if (destination == 0)
	return 0;

    if (lp->lnd_own == 0)
	return 0;

    lcp = &lchr[(int)lp->lnd_type];

    memset(buf1, 0, sizeof(buf1));
    memset(buf2, 0, sizeof(buf2));
    memset(buf3, 0, sizeof(buf3));
    if (chance((spy + lp->lnd_vis) / 10.0)) {
	if (destination == player->cnum)
	    pr("%s %s", mess, prland(lp));
	else
	    sprintf(buf1, "%s %s", mess, prland(lp));

	estimate = lp->lnd_item[I_MILIT];

	if (chance((spy + lp->lnd_vis) / 20.0)) {
	    if (destination == player->cnum)
		pr(" (eff %d, mil %d",
		   roundintby(lp->lnd_effic, 5),
		   roundintby(lp->lnd_item[I_MILIT], 10));
	    else
		sprintf(buf2, " (eff %d, mil %d",
			roundintby(lp->lnd_effic, 5),
			roundintby(lp->lnd_item[I_MILIT], 10));
	    estimate = lp->lnd_item[I_MILIT] * lp->lnd_effic / 100.0;

	    if (chance((spy + lp->lnd_vis) / 20.0)) {
		int t;
		t = lp->lnd_tech - 20 + roll(40);
		t = MAX(t, 0);
		if (destination == player->cnum)
		    pr(", tech %d)\n", t);
		else
		    sprintf(buf3, ", tech %d)\n", t);
	    } else {
		if (destination == player->cnum)
		    pr(")\n");
		else
		    sprintf(buf3, ")\n");
	    }
	} else {
	    if (destination == player->cnum)
		pr("\n");
	    else
		sprintf(buf2, "\n");
	}
    }

    if (destination != player->cnum) {
	wu(0, destination, "%s%s%s", buf1, buf2, buf3);
    }

    if (lp->lnd_ship < 0 || lcp->l_flags & L_MARINE)
	estimate *= lp->lnd_def;

    return estimate;
}

/* Used by the spy command to count land units in a sector.  If used
   for anything else, you may want to reconsider, because this doesn't
   always count spies. :) */
int
count_sect_units(struct sctstr *sp)
{
    int count = 0;
    struct nstr_item ni;
    struct lndstr land;

    snxtitem_all(&ni, EF_LAND);
    while (nxtitem(&ni, &land)) {
	if (!land.lnd_own)
	    continue;
	if (land.lnd_x != sp->sct_x || land.lnd_y != sp->sct_y)
	    continue;
	/* Don't always see spies */
	if (lchr[(int)land.lnd_type].l_flags & L_SPY) {
	    if (!(chance(LND_SPY_DETECT_CHANCE(land.lnd_effic))))
		continue;
	}
	/* Got here, report it */
	++count;
    }

    return count;
}

void
count_units(struct shpstr *sp)
{
    struct nstr_item ni;
    struct lndstr land;
    int nland = 0;

    if (sp->shp_effic < SHIP_MINEFF)
	return;

    snxtitem_xy(&ni, EF_LAND, sp->shp_x, sp->shp_y);
    while (nxtitem(&ni, &land)) {
	if (land.lnd_own == 0)
	    continue;
	if (land.lnd_ship == sp->shp_uid)
	    nland++;
    }

    if (sp->shp_nland != nland) {
	sp->shp_nland = nland;
	putship(sp->shp_uid, sp);
    }
}

void
lnd_count_units(struct lndstr *lp)
{
    struct nstr_item ni;
    struct lndstr land;
    int nland = 0;

    if (lp->lnd_effic < LAND_MINEFF)
	return;

    snxtitem_xy(&ni, EF_LAND, lp->lnd_x, lp->lnd_y);
    while (nxtitem(&ni, &land)) {
	if (land.lnd_own == 0)
	    continue;
	if (land.lnd_land == lp->lnd_uid)
	    nland++;
    }

    if (lp->lnd_nland != nland) {
	lp->lnd_nland = nland;
	putland(lp->lnd_uid, lp);
    }
}

void
lnd_sel(struct nstr_item *ni, struct emp_qelem *list)


/*	int	wantflags;
	int	nowantflags;
*/
{
    struct lndstr land;
    struct lchrstr *lcp;
    struct llist *llp;

    emp_initque(list);
    while (nxtitem(ni, &land)) {
	if (!player->owner)
	    continue;
	if (opt_MARKET) {
	    if (ontradingblock(EF_LAND, &land)) {
		pr("unit #%d inelligible - it's for sale.\n",
		   land.lnd_uid);
		continue;
	    }
	}
	lcp = &lchr[(int)land.lnd_type];
/*		if (wantflags && (lcp->m_flags & wantflags) != wantflags)
			continue;
		if (nowantflags && lcp->m_flags & nowantflags)
			continue;
*/
	/* This abuse is better fixed by building a unit with the normal negative
	   mobility that everything else is built with */
/* Just so that the player can't build a bunch of land units, and them
   march them a few minutes later... */
/*
		if (opt_MOB_ACCESS) {
		  if (land.lnd_effic < 11 &&
		    land.lnd_mobil < etu_per_update) {
		    pr("Land unit #%d needs at least %d mob to march.\n",
			land.lnd_uid, etu_per_update);
		    continue;
		  }
		}
*/
	land.lnd_mission = 0;
	land.lnd_rflags = 0;
	land.lnd_harden = 0;
	memset(land.lnd_rpath, 0, sizeof(land.lnd_rpath));
	putland(land.lnd_uid, &land);
	llp = malloc(sizeof(struct llist));
	llp->lcp = lcp;
	llp->land = land;
	llp->mobil = land.lnd_mobil;
	emp_insque(&llp->queue, list);
    }
}

/* This function assumes that the list was created by lnd_sel */
void
lnd_mar(struct emp_qelem *list, double *minmobp, double *maxmobp,
	int *togetherp, natid actor)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;
    struct sctstr sect;
    struct lndstr land;
    coord allx;
    coord ally;
    int first = 1;
    char mess[128];
    int rel;

    *minmobp = 9876.0;
    *maxmobp = -9876.0;
    *togetherp = 1;
    for (qp = list->q_back; qp != list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	getland(llp->land.lnd_uid, &land);
	if (land.lnd_own != actor) {
	    mpr(actor, "%s was disbanded at %s\n",
		prland(&land), xyas(land.lnd_x, land.lnd_y, land.lnd_own));
	    emp_remque((struct emp_qelem *)llp);
	    free(llp);
	    continue;
	}
	if (land.lnd_ship >= 0) {
	    lnd_mess("is on a ship", llp);
	    continue;
	}
	if (land.lnd_land >= 0) {
	    lnd_mess("is on a unit", llp);
	    continue;
	}
	if (!getsect(land.lnd_x, land.lnd_y, &sect)) {
	    lnd_mess("was sucked into the sky by a strange looking spaceland", llp);	/* heh -KHS */
	    continue;
	}
	if (!(lchr[(int)llp->land.lnd_type].l_flags & L_SPY) &&
	    !(lchr[(int)llp->land.lnd_type].l_flags & L_TRAIN) &&
	    llp->land.lnd_item[I_MILIT] == 0) {
	    lnd_mess("has no mil on it to guide it", llp);
	    continue;
	}
	rel = getrel(getnatp(sect.sct_own), player->cnum);
	if (sect.sct_own != land.lnd_own && rel != ALLIED &&
	    !(lchr[(int)llp->land.lnd_type].l_flags & L_SPY) &&
	    sect.sct_own) {
	    sprintf(mess, "has been kidnapped by %s", cname(sect.sct_own));
	    lnd_mess(mess, llp);
	    continue;
	}
	if (first) {
	    allx = land.lnd_x;
	    ally = land.lnd_y;
	    first = 0;
	}
	if (land.lnd_x != allx || land.lnd_y != ally)
	    *togetherp = 0;
	if (land.lnd_mobil + 1 < (int)llp->mobil) {
	    llp->mobil = land.lnd_mobil;
	}
	if (llp->mobil < *minmobp)
	    *minmobp = llp->mobil;
	if (llp->mobil > *maxmobp)
	    *maxmobp = llp->mobil;
	llp->land = land;
    }
}

void
lnd_put(struct emp_qelem *list, natid actor)
{
    struct emp_qelem *qp;
    struct emp_qelem *newqp;
    struct llist *llp;

    qp = list->q_back;
    while (qp != list) {
	llp = (struct llist *)qp;
	if (actor) {
	    mpr(actor, "%s stopped at %s\n", prland(&llp->land),
		xyas(llp->land.lnd_x, llp->land.lnd_y, llp->land.lnd_own));
	    if (llp->mobil < -127)
		llp->mobil = -127;
	    llp->land.lnd_mobil = llp->mobil;
	}
	putland(llp->land.lnd_uid, &llp->land);
	newqp = qp->q_back;
	emp_remque(qp);
	free(qp);
	qp = newqp;
    }
}

void
lnd_sweep(struct emp_qelem *land_list, int verbose, int takemob,
	  natid actor)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;
    struct sctstr sect;
    int mines, m, max, sshells, lshells;

    for (qp = land_list->q_back; qp != land_list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	if (!(llp->lcp->l_flags & L_ENGINEER)) {
	    if (verbose)
		mpr(actor, "%s is not an engineer!\n", prland(&llp->land));
	    continue;
	}
	if (takemob && llp->mobil < 0.0) {
	    if (verbose)
		lnd_mess("is out of mobility", llp);
	    continue;
	}
	getsect(llp->land.lnd_x, llp->land.lnd_y, &sect);
	if (sect.sct_oldown == llp->land.lnd_own) {
	    if (verbose)
		mpr(actor,
		    "%s is in a sector completely owned by you.  Don't bother digging up mines there!\n",
		    prland(&llp->land));
	    continue;
	}
	if (sect.sct_type == SCT_BSPAN) {
	    if (verbose)
		mpr(actor, "%s is on a bridge.  No mines there!\n",
		    prland(&llp->land));
	    continue;
	}
	if (takemob) {
	    llp->mobil -= lnd_pathcost(&llp->land, 0.2);
	    llp->land.lnd_mobil = (int)llp->mobil;
	    llp->land.lnd_harden = 0;
	}
	putland(llp->land.lnd_uid, &llp->land);
	if (!(mines = sect.sct_mines))
	    continue;
	max = llp->lcp->l_item[I_SHELL];
	lshells = llp->land.lnd_item[I_SHELL];
	sshells = sect.sct_item[I_SHELL];
	for (m = 0; mines > 0 && m < max * 2; m++) {
	    if (chance(0.5 * llp->lcp->l_att)) {
		mpr(actor, "Sweep...\n");
		mines--;
		if (lshells < max)
		    ++lshells;
		else if (sshells < ITEM_MAX)
		    ++sshells;
	    }
	}
	sect.sct_mines = mines;
	llp->land.lnd_item[I_SHELL] = lshells;
	sect.sct_item[I_SHELL] = sshells;
	putland(llp->land.lnd_uid, &llp->land);
	putsect(&sect);
    }
}

static int
contains_engineer(struct emp_qelem *list)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;

    for (qp = list->q_back; qp != list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	if (llp->lcp->l_flags & L_ENGINEER)
	    return 1;
    }
    return 0;
}

int
lnd_check_mines(struct emp_qelem *land_list)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;
    struct sctstr sect;
    int stopping = 0;
    int with_eng = contains_engineer(land_list);

    for (qp = land_list->q_back; qp != land_list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	getsect(llp->land.lnd_x, llp->land.lnd_y, &sect);
	if (sect.sct_oldown == llp->land.lnd_own)
	    continue;
	if (sect.sct_type == SCT_BSPAN)
	    continue;
	if (!sect.sct_mines)
	    continue;
	if (chance(DMINE_LHITCHANCE(sect.sct_mines) / (1 + 2 * with_eng))) {
	    lnd_hit_mine(&llp->land, llp->lcp);
	    sect.sct_mines--;
	    putsect(&sect);
	    putland(llp->land.lnd_uid, &llp->land);
	    if (!llp->land.lnd_own) {
		stopping = 1;
		emp_remque(qp);
		free(qp);
	    }
	}
    }
    return stopping;
}

void
lnd_list(struct emp_qelem *land_list)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;
    struct lndstr *lnd;

    pr("lnd#     land type       x,y    a  eff  sh gun xl  mu tech retr fuel\n");

    for (qp = land_list->q_back; qp != land_list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	lnd = &llp->land;
	pr("%4d ", lnd->lnd_uid);
	pr("%-16.16s ", llp->lcp->l_name);
	prxy("%4d,%-4d ", lnd->lnd_x, lnd->lnd_y, llp->land.lnd_own);
	pr("%1.1s", &lnd->lnd_army);
	pr("%4d%%", lnd->lnd_effic);
	pr("%4d", lnd->lnd_item[I_SHELL]);
	pr("%4d", lnd->lnd_item[I_GUN]);
	count_land_planes(lnd);
	pr("%3d", lnd->lnd_nxlight);
	pr("%4d", lnd->lnd_mobil);
	pr("%4d", lnd->lnd_tech);
	pr("%4d%%", lnd->lnd_retreat);
	pr("%5d\n", lnd->lnd_fuel);
    }
}

static void
lnd_mess(char *str, struct llist *llp)
{
    mpr(llp->land.lnd_own, "%s %s & stays in %s\n",
	prland(&llp->land),
	str, xyas(llp->land.lnd_x, llp->land.lnd_y, llp->land.lnd_own));
    if (llp->mobil < -127)
	llp->mobil = -127;
    llp->land.lnd_mobil = llp->mobil;
    putland(llp->land.lnd_uid, &llp->land);
    emp_remque((struct emp_qelem *)llp);
    free(llp);
}

static int
lnd_count(struct emp_qelem *list)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;
    int count = 0;

    for (qp = list->q_back; qp != list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	++count;
    }
    return count;
}

static int
lnd_damage(struct emp_qelem *list, int totdam)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;
    int dam;
    int count;

    if (!totdam || !(count = lnd_count(list)))
	return 0;
    dam = ldround((double)totdam / count, 1);
    for (qp = list->q_back; qp != list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	/* have to get it again because of collateral damage */
	getland(llp->land.lnd_uid, &llp->land);
	landdamage(&llp->land, dam);
	putland(llp->land.lnd_uid, &llp->land);
	if (!llp->land.lnd_own) {
	    emp_remque(qp);
	    free(qp);
	}
    }
    return dam;
}

static int
lnd_easiest_target(struct emp_qelem *list)
{
    struct emp_qelem *qp;
    struct emp_qelem *next;
    struct llist *llp;
    int hard;
    int easiest = 9876;		/* things start great for victim */
    int count = 0;

    for (qp = list->q_back; qp != list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	hard = lnd_hardtarget(&llp->land);
	if (hard < easiest)
	    easiest = hard;	/* things get worse for victim */
	++count;
    }
    return easiest - count;
}

static int
lnd_missile_interdiction(struct emp_qelem *list, coord newx, coord newy,
			 natid victim)
{
    int dam;
    struct emp_qelem msl_list, *qp, *newqp;

    msl_sel(&msl_list, newx, newy, victim, P_T, P_MAR, MI_INTERDICT);

    dam = msl_launch_mindam(&msl_list, newx, newy,
			    lnd_easiest_target(list), EF_LAND,
			    lnd_count(list) * 20, "troops", victim,
			    MI_INTERDICT);
    if (dam) {
	mpr(victim, "missile interdiction mission does %d damage!\n", dam);
	collateral_damage(newx, newy, dam, 0);
    }
    qp = msl_list.q_forw;
    while (qp != msl_list.q_forw) {
	newqp = qp->q_forw;
	emp_remque(qp);
	free(qp);
	qp = newqp;
    }
    return dam;
}

#if 0
/* Steve M. - commented out for now until abuse is decided upon */
/* risner: allow forts to interdict land units. */
static int
lnd_fort_interdiction(struct emp_qelem *list,
		      coord newx, coord newy, natid victim)
{
    struct nstr_sect ns;
    struct sctstr fsect;
    int trange, range;
    double guneff;
    int shell, gun;
    int dam;
    int totdam = 0;
    int i;

    snxtsct_dist(&ns, newx, newy, fort_max_interdiction_range);
    while (nxtsct(&ns, &fsect)) {
	if (fsect.sct_own == 0)
	    continue;
	if (fsect.sct_own == victim)
	    continue;
	if (getrel(getnatp(fsect.sct_own), victim) >= NEUTRAL)
	    continue;
	gun = fsect.sct_item[I_GUN];
	if (gun < 1)
	    continue;
	range = roundrange(fortrange(&fsect));
	trange = mapdist(newx, newy, fsect.sct_x, fsect.sct_y);
	if (trange > range)
	    continue;
	if (fsect.sct_item[I_MILIT] < 5)
	    continue;
	shell = fsect.sct_item[I_SHELL];
	if (shell < 1)
	    shell += supply_commod(fsect.sct_own, fsect.sct_x, fsect.sct_y,
				   I_SHELL, 1);
	if (shell < 1)
	    continue;
	shell--;
	fsect.sct_item[I_SHELL] = shell;
	putsect(&fsect);
	if (gun > 7)
	    gun = 7;
	guneff = landgun((int)fsect.sct_effic, gun);
	dam = (int)guneff;
	totdam += dam;
	mpr(victim, "Incoming fire does %d damage!\n", dam);
	wu(0, fsect.sct_own,
	   "%s fires at %s land units in %s for %d!\n",
	   xyas(fsect.sct_x, fsect.sct_y,
		fsect.sct_own),
	   cname(victim), xyas(newx, newy, fsect.sct_own), dam);
	nreport(fsect.sct_own, N_SCT_SHELL, victim, 1);
    }
    if (totdam > 0)
	return lnd_damage(list, totdam);
    return 0;
}
#endif

int
lnd_interdict(struct emp_qelem *list, coord newx, coord newy, natid victim)
{
    int stopping = 0;

#if 0
    if (!opt_NO_FORT_FIRE)
/* Steve M. - commented out for now until abuse is decided upon */
	stopping |= lnd_fort_interdiction(list, newx, newy, victim);
#endif

    stopping |=
	lnd_damage(list,
		   unit_interdict(newx, newy, victim, "land units",
				  lnd_easiest_target(list), MI_INTERDICT));

    stopping |=
	lnd_damage(list,
		   lnd_missile_interdiction(list, newx, newy, victim));
    return stopping;
}

/* high value of hardtarget is harder to hit */
int
lnd_hardtarget(struct lndstr *lp)
{
    struct sctstr sect;

    getsect(lp->lnd_x, lp->lnd_y, &sect);
    return (int)((lp->lnd_effic / 100.0) *
		 (10 + dchr[sect.sct_type].d_dstr * 2 + lp->lnd_spd / 2.0
		  - lp->lnd_vis));
}

static int
lnd_hit_mine(struct lndstr *lp, struct lchrstr *lcp)
{
    int m;

    mpr(lp->lnd_own, "Blammo! Landmines detected in %s! ",
	xyas(lp->lnd_x, lp->lnd_y, lp->lnd_own));

    nreport(lp->lnd_own, N_LHIT_MINE, 0, 1);

    m = MINE_LDAMAGE();
    if (lcp->l_flags & L_ENGINEER)
	m /= 2;

    landdamage(lp, m);
    return m;
}

double
lnd_pathcost(struct lndstr *lp, double pathcost)
{
    double effspd;

    effspd = lp->lnd_spd;
    if (lchr[(int)lp->lnd_type].l_flags & L_SUPPLY)
	effspd *= lp->lnd_effic * 0.01;

    /*
     * The return value must be PATHCOST times a factor that depends
     * only on the land unit.  Anything else breaks path finding.  In
     * particular, you can't add or enforce a minimum cost here.  Do
     * it in sector_mcost().
     */
    return pathcost * 5.0 * speed_factor(effspd, lp->lnd_tech);
}

int
lnd_mobtype(struct lndstr *lp)
{
    return (lchr[(int)lp->lnd_type].l_flags & L_TRAIN)
	? MOB_RAIL : MOB_MARCH;
}

double
lnd_mobcost(struct lndstr *lp, struct sctstr *sp)
{
    return lnd_pathcost(lp, sector_mcost(sp, lnd_mobtype(lp)));
}

int
lnd_mar_one_sector(struct emp_qelem *list, int dir, natid actor,
		   int together)
{
    struct sctstr sect, osect;
    struct emp_qelem *qp;
    struct emp_qelem *qp2;
    struct emp_qelem *next;
    struct llist *llp;
    struct emp_qelem cur, done;
    coord dx;
    coord dy;
    coord newx;
    coord newy;
    int stopping = 0;
    int visible;
    int stop;
    char dp[80];
    int rel;
    int oldown;

    if (dir <= DIR_STOP || dir >= DIR_VIEW) {
	lnd_put(list, actor);
	return 1;
    }
    dx = diroff[dir][0];
    dy = diroff[dir][1];
    for (qp = list->q_back; qp != list; qp = next) {
	next = qp->q_back;
	llp = (struct llist *)qp;
	getsect(llp->land.lnd_x, llp->land.lnd_y, &osect);
	oldown = osect.sct_own;
	newx = xnorm(llp->land.lnd_x + dx);
	newy = ynorm(llp->land.lnd_y + dy);
	getsect(newx, newy, &sect);
	rel = getrel(getnatp(sect.sct_own), player->cnum);
	if ((sect.sct_own != actor && rel != ALLIED &&
	     !(lchr[(int)llp->land.lnd_type].l_flags & L_SPY) &&
	     sect.sct_own) || (sect.sct_type == SCT_WATER ||
			       sect.sct_type == SCT_SANCT ||
			       sect.sct_type == SCT_WASTE)) {
	    if (together) {
		pr("can't go to %s\n", xyas(newx, newy, actor));
		return 1;
	    } else {
		sprintf(dp, "can't go to %s", xyas(newx, newy, actor));
		lnd_mess(dp, llp);
		continue;
	    }
	}
	if ((!intrchr[INT_RAIL].in_enable || sect.sct_rail == 0) &&
	    lchr[(int)llp->land.lnd_type].l_flags & L_TRAIN) {
	    if (together) {
		pr("no rail system in %s\n", xyas(newx, newy, actor));
		return 1;
	    } else {
		sprintf(dp, "has no rail system in %s",
			xyas(newx, newy, actor));
		lnd_mess(dp, llp);
		continue;
	    }
	}
	/* Note we check would_abandon first because we don't want
	   to always have to do these checks */
	if (would_abandon(&osect, I_CIVIL, 0, &llp->land)) {
	    stop = 0;
	    if (!want_to_abandon(&osect, I_CIVIL, 0, &llp->land)) {
		stop = 1;
	    }
	    /* now check stuff */
	    if (!check_sect_ok(&sect))
		return 1;
	    if (!check_sect_ok(&osect))
		return 1;
	    for (qp2 = list->q_back; qp2 != list; qp2 = qp2->q_back) {
		if (!check_land_ok(&((struct llist *)qp2)->land))
		    return 1;
	    }
	    if (stop) {
		lnd_mess("stops", llp);
		continue;
	    }
	}
	if (llp->mobil <= 0.0) {
	    lnd_mess("is out of mobility", llp);
	    continue;
	}
	llp->land.lnd_x = newx;
	llp->land.lnd_y = newy;
	llp->mobil -= lnd_mobcost(&llp->land, &sect);
	llp->land.lnd_mobil = (int)llp->mobil;
	llp->land.lnd_harden = 0;
	putland(llp->land.lnd_uid, &llp->land);
	putsect(&osect);
	getsect(osect.sct_x, osect.sct_y, &osect);
	if (osect.sct_own != oldown && oldown == player->cnum) {
	    /* It was your sector, now it's not.  Simple :) */
	    pr("You no longer own %s\n",
	       xyas(osect.sct_x, osect.sct_y, player->cnum));
	}
	if (rel != ALLIED && sect.sct_own != actor && sect.sct_own) {	/* must be a spy */
	    /* Always a 10% chance of getting caught. */
	    if (chance(LND_SPY_DETECT_CHANCE(llp->land.lnd_effic))) {
		if (rel == NEUTRAL || rel == FRIENDLY) {
		    wu(0, sect.sct_own,
		       "%s unit spotted in %s\n", cname(player->cnum),
		       xyas(sect.sct_x, sect.sct_y, sect.sct_own));
		    setrel(sect.sct_own, llp->land.lnd_own, HOSTILE);
		} else if (rel == HOSTILE || rel == AT_WAR ||
			   rel == SITZKRIEG || rel == MOBILIZATION) {
		    wu(0, sect.sct_own,
		       "%s spy shot in %s\n", cname(player->cnum),
		       xyas(sect.sct_x, sect.sct_y, sect.sct_own));
		    pr("%s was shot and killed.\n", prland(&llp->land));
		    llp->land.lnd_effic = 0;
		    putland(llp->land.lnd_uid, &llp->land);
		    lnd_delete(llp, 0);
		}
	    }
	}
    }
    if (QEMPTY(list))
	return stopping;
    lnd_sweep(list, 0, 1, actor);
    stopping |= lnd_check_mines(list);
    if (QEMPTY(list))
	return stopping;

    /* interdict land units sector by sector */
    emp_initque(&cur);
    emp_initque(&done);
    while (!QEMPTY(list)) {
	llp = (struct llist *)list->q_back;
	newx = llp->land.lnd_x;
	newy = llp->land.lnd_y;
	/* move units in NEWX,NEWY to cur */
	visible = 0;
	for (qp = list->q_back; qp != list; qp = next) {
	    next = qp->q_back;
	    llp = (struct llist *)qp;
	    if (llp->land.lnd_x == newx && llp->land.lnd_y == newy) {
		emp_remque(qp);
		emp_insque(qp, &cur);
		if (!(lchr[(int)llp->land.lnd_type].l_flags & L_SPY))
		    visible = 1;
	    }
	}
	/* interdict them */
	if (visible)
	    stopping |= lnd_interdict(&cur, newx, newy, actor);
	/* move survivors to done */
	for (qp = cur.q_back; qp != &cur; qp = next) {
	    next = qp->q_back;
	    llp = (struct llist *)qp;
	    emp_remque(qp);
	    emp_insque(qp, &done);
	}
    }
    /* assign surviving land units back to list */
    emp_insque(list, &done);
    emp_remque(&done);

    return stopping;
}

/*
 * find all artillery units belonging
 * to the attacker or defender that can fire.
 * Each arty unit adds +1%/damage point
 */
int
lnd_support(natid victim, natid attacker, coord x, coord y, int defending)
{
    struct nstr_item ni;
    struct lndstr land;
    int rel, rel2;
    double dam = 0.0;
    int dist;
    int shell;
    int gun;
    int range;

    snxtitem_all(&ni, EF_LAND);
    while (nxtitem(&ni, &land)) {
	if (land.lnd_frg == 0)
	    continue;
	if ((land.lnd_x == x) && (land.lnd_y == y))
	    continue;
	if (land.lnd_ship >= 0)
	    continue;
	if (land.lnd_land >= 0)
	    continue;
	if (land.lnd_effic < LAND_MINFIREEFF)
	    continue;
	/* Do we have mil? */
	if (land.lnd_item[I_MILIT] <= 0)
	    continue;
	rel = getrel(getnatp(land.lnd_own), attacker);
	rel2 = getrel(getnatp(land.lnd_own), victim);
	if ((land.lnd_own != attacker) &&
	    ((rel != ALLIED) || (rel2 != AT_WAR)))
	    continue;

	/* do we have supplies? */
	if (!has_supply(&land))
	    continue;

	/* are we in range? */
	dist = mapdist(land.lnd_x, land.lnd_y, x, y);

	range = roundrange(effrange(land.lnd_frg, land.lnd_tech));
	if (dist > range)
	    continue;

	shell = land.lnd_item[I_SHELL];
	gun = land.lnd_item[I_GUN];

	if (shell == 0 || gun == 0)
	    continue;

	use_supply(&land);
	if (defending)
	    nreport(land.lnd_own, N_FIRE_BACK, victim, 1);
	else
	    nreport(land.lnd_own, N_FIRE_L_ATTACK, victim, 1);
	if (roll(100) < land.lnd_acc) {
	    dam += landunitgun(land.lnd_effic, land.lnd_dam, gun,
			       land.lnd_ammo, shell) / 2;
	} else {
	    dam += landunitgun(land.lnd_effic, land.lnd_dam, gun,
			       land.lnd_ammo, shell);
	}
	if (land.lnd_own != attacker)
	    wu(0, land.lnd_own,
	       "%s supported %s at %s\n",
	       prland(&land), cname(attacker), xyas(x, y, land.lnd_own));
    }
    return (int)dam;
}

char *
lnd_path(int together, struct lndstr *lp, char *buf)
{
    coord destx;
    coord desty;
    struct sctstr d_sect, sect;
    char *cp;
    double dummy;
    int mtype;

    if (!sarg_xy(buf, &destx, &desty))
	return 0;
    if (!together) {
	pr("Cannot go to a destination sector if not all starting in the same sector\n");
	return 0;
    }
    if (!getsect(destx, desty, &d_sect)) {
	pr("%d,%d is not a sector\n", destx, desty);
	return 0;
    }
    getsect(lp->lnd_x, lp->lnd_y, &sect);
    mtype = lnd_mobtype(lp);
    cp = BestLandPath(buf, &sect, &d_sect, &dummy, mtype);
    if (!cp) {
	pr("No owned %s from %s to %s!\n",
	   mtype == MOB_RAIL ? "railway" : "path",
	   xyas(lp->lnd_x, lp->lnd_y, player->cnum),
	   xyas(d_sect.sct_x, d_sect.sct_y, player->cnum));
	return 0;
    }
    pr("Using path '%s'\n", cp);
    return cp;
}

int
lnd_can_attack(struct lndstr *lp)
{
    struct lchrstr *lcp = &lchr[(int)lp->lnd_type];

    if (lcp->l_flags & L_SUPPLY)
	return 0;

    return 1;
}

/*
 * Increase fortification value of LP.
 * Fortification costs mobility.  Use up to MOB mobility.
 * Return actual fortification increase.
 */
int
lnd_fortify(struct lndstr *lp, int mob)
{
    int hard_amt;
    double mob_used, mult;

    if (lp->lnd_ship >= 0 || lp->lnd_land >= 0)
	return 0;

    mob_used = MIN(lp->lnd_mobil, mob);
    mult = has_helpful_engineer(lp->lnd_x, lp->lnd_y, lp->lnd_own)
	? 1.5 : 1.0;

    hard_amt = (int)(mob_used * mult);
    if (lp->lnd_harden + hard_amt > land_mob_max) {
	hard_amt = land_mob_max - lp->lnd_harden;
	mob_used = ceil(hard_amt / mult);
    }

    lp->lnd_mobil -= (int)mob_used;
    lp->lnd_harden += hard_amt;
    lp->lnd_harden = MIN(lp->lnd_harden, land_mob_max);

    return hard_amt;
}

/*
 * Set LP's tech to TLEV along with everything else that depends on it.
 */
void
lnd_set_tech(struct lndstr *lp, int tlev)
{
    struct lchrstr *lcp = lchr + lp->lnd_type;
    int tech_diff = tlev - lcp->l_tech;

    if (CANT_HAPPEN(tech_diff < 0)) {
      tlev -= tech_diff;
      tech_diff = 0;
    }

    lp->lnd_tech = tlev;
    lp->lnd_att = (float)LND_ATTDEF(lcp->l_att, tech_diff);
    lp->lnd_def = (float)LND_ATTDEF(lcp->l_def, tech_diff);
    lp->lnd_vul = (int)LND_VUL(lcp->l_vul, tech_diff);
    lp->lnd_spd = (int)LND_SPD(lcp->l_spd, tech_diff);
    lp->lnd_vis = (int)LND_VIS(lcp->l_vis, tech_diff);
    lp->lnd_spy = (int)LND_SPY(lcp->l_spy, tech_diff);
    lp->lnd_rad = (int)LND_RAD(lcp->l_rad, tech_diff);
    lp->lnd_frg = (int)LND_FRG(lcp->l_frg, tech_diff);
    lp->lnd_acc = (int)LND_ACC(lcp->l_acc, tech_diff);
    lp->lnd_dam = (int)LND_DAM(lcp->l_dam, tech_diff);
    lp->lnd_ammo = (int)LND_AMM(lcp->l_ammo, lcp->l_dam, tech_diff);
    lp->lnd_aaf = (int)LND_AAF(lcp->l_aaf, tech_diff);
    lp->lnd_fuelc = (int)LND_FC(lcp->l_fuelc, tech_diff);
    lp->lnd_fuelu = (int)LND_FU(lcp->l_fuelu, tech_diff);
    lp->lnd_maxlight = (int)LND_XPL(lcp->l_nxlight, tech_diff);
    lp->lnd_maxland = (int)LND_MXL(lcp->l_nland, tech_diff);
}
