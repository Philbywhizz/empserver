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
 *  empmod.c: List all commands for Empire
 * 
 *  Known contributors to this file:
 *     Dave Pare, 1986
 *     Jeff Bailey
 *     Thomas Ruschak, 1992
 *     Ken Stevens, 1995
 *     Steve McClure, 1996-2000
 */

#include "misc.h"
#include "com.h"
#include "nat.h"
#include "file.h"

extern int add();
extern int anti();
extern int assa();
extern int atta();
extern int boar();
extern int bdes();
extern int bomb();
extern int buil();
extern int chan();
extern int coas();
extern int comm();
extern int coun();
extern int decl();
extern int deli();
extern int show();
extern int show_motd(void);
extern int desi();
extern int diss();
extern int drop();
extern int echo(void);
extern int enli();
extern int fire();
extern int flee();
extern int fly();
extern int force();
extern int info();
extern int apro();
extern int load();
extern int look();
extern int map();
extern int mine();
extern int move();
extern int nati();
extern int navi();
extern int head(), news();
extern int nuke();
extern int offs();
extern int para();
extern int path();
extern int payo();
extern int powe();
extern int rada();
extern int rea();
extern int real();
extern int reco();
extern int rout();
extern int sona();
extern int spy();
extern int tele();
extern int tend();
extern int torp();
extern int tran();
extern int trea();
extern int turn();
extern int vers();
/*
 * new commands
 */
extern int repo();
extern int laun();
extern int new();
extern int shoo();
extern int thre();
extern int dist();
extern int sct();
extern int plan();
extern int arm();
extern int hard();
extern int upgr();
extern int surv();
extern int capi();
extern int orig();
extern int conv();
extern int leve();
extern int cuto();
extern int prod();
extern int wai();
extern int carg();
extern int terr();
extern int sate();
extern int give();
extern int edit();
extern int wipe();
extern int dump();
extern int ldump();
extern int ndump();
extern int pdump();
extern int pboa();
extern int sdump();
extern int lost();
extern int explore();
extern int reso();
extern int scra();
extern int rela();
extern int brea();
extern int upda();
extern int hidd();
extern int orde();
extern int qorde();
extern int sorde();
extern int reje();
extern int acce();
extern int sabo();
extern int scut();
extern int grin();
extern int shar();
extern int sail();		/* Forsman's hacks */
extern int foll();
extern int mobq();
extern int name();
extern int range();
extern int zdon();
extern int fuel();
extern int multifire();
extern int retr();
extern int budg();
extern int wire();
extern int stop();
extern int start();

extern int land();
extern int supp();
extern int lboa();
extern int lcarg();
extern int lload();
extern int army();
extern int lrange();
extern int morale();
extern int lretr();
extern int landmine();
extern int fort();
extern int march();
extern int llook();
extern int mission();
extern int work();
extern int ltend();
extern int cede();
extern int best();
extern int newe();
extern int starve();

extern int setres();
extern int setsector();

extern int disa();
extern int enab();

/*
 * Undeclared functions
 */
extern int quit(void);
extern int cens();
extern int demo();
extern int shi();
extern int wing();
extern int execute(void);
extern int explain(void);
extern int set();
extern int flash(void);
extern int wall();
extern int shut();
extern int togg();
extern int stre();
extern int skyw();
extern int play();
extern int swaps();

extern int trad();
extern int mark();
extern int buy();
extern int mult();
extern int sell();
extern int rese();

extern int repa();
extern int fina();
extern int coll();
extern int cons();
extern int ledg();
extern int shark();
extern int offe();

extern int mobupdate();

extern int sinfra();
extern int improve();
extern int lstats();
extern int pstat();
extern int sstat();

struct cmndstr player_coms[] = {
/*  command form                       cost	addr    permit */
    {"accept  [as-COUNTRY]", 0, acce, C_MOD, NORM},
    {"add <NAT> <NAME> <REP> <STATUS> <c|w|i>", 0, add, C_MOD, GOD},
    {"announce {message}", 0, tele, C_MOD, VIS},
    {"anti <SECT>", 3, anti, C_MOD, NORM + MONEY + CAP},
    {"apropos <topic>", 0, apro, 0, VIS},
    {"arm <PLANE> <NUKETYPE> <airburst?>", 3, arm, C_MOD,
     NORM + MONEY + CAP},
    {"army <ARMY> <UNITS>", 0, army, C_MOD, NORM},
    {"assault <to-SECT> <from-SHIP> [<forts?> <ships?> <arty?> <planes?>]",
     3, assa, C_MOD, NORM + MONEY + CAP},
    {"attack <SECT> [<forts?> <ships?> <arty?> <planes?>]", 3, atta, C_MOD,
     NORM + MONEY + CAP},
    {"bdes <SECTS> <type>", 0, bdes, C_MOD, NORM},
    {"bestpath <SECTS> <SECTS>", 0, best, 0, NORM},
    {"bestdistpath <SECTS> <SECTS>", 0, best, 0, NORM},
    {"bmap <SECTS|UNIT> [s|l|p|r|t*]", 0, map, C_MOD, NORM},
    {"board <to-SHIP> [<from-SHIP>|<from-sector>]", 3, boar, C_MOD,
     NORM + MONEY + CAP},
    {"bomb <bomber-PLANES> <fighter-PLANES> <pin|strat> <ap-SECT> <PATH|DESTINATION>", 2, bomb, C_MOD, NORM + MONEY + CAP},
    {"break {sanctuary}", 1, brea, C_MOD, SANCT},
    {"budget [<sector type|P|S|L|A|M|N|C> <PRIORITY|~>]", 1, budg, 0,
     NORM},
    {"build <s|l|b|p|n|t> <SECTS> <type|dir> [<number> <tech> <sure?>]", 1,
     buil, C_MOD, NORM + MONEY + CAP},
    {"buy <COMM> <LOT#> <BID/UNIT> <DEST>", 1, buy, C_MOD,
     CAP + NORM + MONEY},
    {"bye {log-off}", 0, quit, 0, VIS},
    {"capital <SECT>", 0, capi, C_MOD, NORM},
    {"cargo <SHIPS>", 0, carg, 0, NORM},
#ifdef CEDE_IS_ALLOWED
    {"cede <SECTS|SHIPS> <COUNTRY> [se|sh]", 639, cede, C_MOD, NORM},
#endif /* CEDE_IS_ALLOWED */
    {"census <SECTS>", 0, cens, 0, VIS},
    {"change <country|representative> <NAME>", 0, chan, C_MOD, VIS},
    {"coastwatch <SECTS>", 1, coas, C_MOD, NORM + CAP},
    {"collect <LOAN> <SECT>", 2, coll, C_MOD, NORM + MONEY + CAP},
    {"commodity <SECTS>", 0, comm, 0, VIS},
    {"consider <loan|treaty> <LOAN/TREATY> <acc|decl|post>", 1, cons,
     C_MOD, NORM + CAP},
    {"convert <SECTS> <NUM> [sure?]", 2, conv, C_MOD, NORM + MONEY + CAP},
    {"country <NATS>", 0, coun, 0, NORM},
    {"cutoff <SECTS>", 0, cuto, C_MOD, NORM},
    {"declare <all|friend|neut|hos|war> <NATS>", 2, decl, C_MOD,
     NORM + CAP},
    {"deliver <COMM> <SECTS> [q|<THRESH>] [<DIR>]", 1, deli, C_MOD, NORM},
    {"demobilize <SECTS> <NUM> <active-reserve?> [sure?]", 1, demo, C_MOD,
     NORM + CAP},
    {"designate <SECTS> <type> [sure?]", 1, desi, C_MOD, NORM},
    {"disable", 0, disa, C_MOD, GOD},
    {"disarm <PLANE>", 2, arm, C_MOD, NORM + MONEY + CAP},
    {"dissolve <really dissove?>", 0, diss, C_MOD, GOD},
    {"distribute <SECTS> <DISTSECT|.|h>", 1, dist, C_MOD, NORM},
    {"drop <cargo-PLANES> <fighter-PLANES> <ap-SECT> <PATH|DESTINATION> <COMM>", 1, drop, C_MOD, NORM + MONEY + CAP},
    {"dump <SECTS> [<fields>]", 0, dump, 0, NORM},
    {"echo [<string>]", 0, echo, 0, NORM},
    {"edit <country|land|unit|ship|plane|nuke> [<KEY> <VALUE>]...", 0,
     edit, C_MOD, GOD},
    {"enable", 0, enab, C_MOD, GOD},
    {"enlist <SECTS> <NUM>", 2, enli, C_MOD, NORM + MONEY + CAP},
    {"execute <INPUT FILE>", 0, execute, 0, VIS},
    {"explore <c|m> <SECT> <NUM> <PATH|DESTINATION>", 1, explore, C_MOD,
     NORM + MONEY + CAP},
    {"financial", 0, fina, 0, NORM},
    {"fire <land, ship or sect> <from-SECTS | SHIPS | UNITS> [TO-SECT | SHIP]", 1, multifire, C_MOD, NORM + MONEY + CAP},
    {"flash <COUNTRY> [<message>]", 0, flash, 0, VIS},
    {"fleetadd <FLEET> <SHIPS>", 0, flee, C_MOD, NORM},
    {"fly <cargo-PLANES> <fighter-PLANES> <ap-SECT> <PATH|DESTINATION> <COM>", 2, fly, C_MOD, NORM + MONEY + CAP},
    {"follow <leader> <SHIPS>", 1, foll, C_MOD, NORM + CAP},
    {"force <delay time>", 0, force, C_MOD, GOD},
    {"fortify <UNITS> <MOB>", 1, fort, C_MOD, NORM},
    {"fuel <s|l> <SHIP/FLEET | UNIT/ARMY> <AMOUNT> [<OILER>]", 1, fuel,
     C_MOD, NORM},
    {"give <COMM> <SECT> <NUM>", 0, give, C_MOD, GOD},
    {"grind <SECT> <NUM>", 1, grin, C_MOD, NORM + MONEY + CAP},
    {"harden <PLANES> <NUM>", 2, hard, C_MOD, NORM + MONEY + CAP},
    {"help <topic>", 0, info, 0, VIS},
    {"headlines [days]", 0, head, C_MOD, VIS},
    {"hidden <SECTS>", 0, hidd, 0, GOD},
    {"improve <road|rail|defense> <SECTS> <NUM>", 1, improve, C_MOD,
     NORM + MONEY + CAP},
    {"info <topic>", 0, info, 0, VIS},
    {"land <UNITS>", 0, land, C_MOD, NORM},
    {"launch <PLANES>", 3, laun, C_MOD, NORM + MONEY + CAP},
    {"lboard <to-LAND> [<from-sector>]", 3, lboa, C_MOD,
     NORM + MONEY + CAP},
    {"lcargo <UNITS>", 0, lcarg, 0, NORM},
    {"ledger <LOANS>", 0, ledg, 0, NORM},
    {"level <SECTS>", 0, leve, 0, NORM},
    {"list of commands", 0, explain, 0, VIS},
    {"lbmap <SECTS|UNIT> [s|l|p|*]", 0, map, C_MOD, NORM},
    {"ldump <UNITS> [<fields>]", 0, ldump, 0, NORM},
    {"lload <COMM|\"land\"|\"plane\"> <UNITS> <NUM|UNITS|PLANES>", 1,
     lload, C_MOD, NORM + CAP},
    {"llookout <UNITS>", 1, llook, 0, NORM + CAP},
    {"lmap <SECTS|UNIT> [s|l|p|*]", 0, map, C_MOD, NORM},
    {"lmine <UNITS>", 2, landmine, C_MOD, NORM + MONEY + CAP},
    {"load <COMM|\"land\"|\"plane\"> <SHIPS> <NUM|UNITS|PLANES>", 1, load,
     C_MOD, NORM + CAP},
    {"lookout <SHIPS>", 1, look, 0, NORM + CAP},
    {"lost", 0, lost, 0, NORM},
    {"lradar <UNITS | SECTS>", 1, rada, 0, NORM + CAP},
    {"lrange <UNITS> <range>", 1, lrange, C_MOD, NORM + CAP},
    {"lretreat <UNITS|ARMY> <PATH> [i|h|b|c]", 1, lretr, C_MOD,
     NORM + CAP},
    {"lstats <UNITS>", 0, lstats, C_MOD, NORM},
    {"ltend <COMM> <tender-SHIPS> <NUM> <UNITS>", 1, ltend, C_MOD,
     NORM + CAP},
    {"lunload <COMM|\"land\"|\"plane\"> <UNITS> <NUM|UNITS|PLANES>", 1,
     lload, C_MOD, NORM + CAP},
    {"market [COMM]", 0, mark, 0, VIS},
    {"map <SECTS|SHIP>", 0, map, C_MOD, VIS},
    {"march <UNITS> <PATH|DESTINATION>", 1, march, C_MOD, NORM + CAP},
    {"mine <SHIPS>", 2, mine, C_MOD, NORM + MONEY + CAP},
    {"mission <type> <PLANES|SHIPS|UNITS> <mission type> <op sect> [<radius>]", 2, mission, C_MOD, NORM + CAP},
    {"mobupdate <MINUTES|check>", 0, mobupdate, C_MOD, GOD},
    {"morale <UNITS> <retreat%>", 1, morale, C_MOD, NORM + CAP},
    {"motd", 0, show_motd, C_MOD, VIS},
    {"move <c|m|s|g|p|i|o|b|f|r|u> <SECT> <NUM> <PATH|DESTINATION>", 1,
     move, C_MOD, NORM + CAP},
    {"mquota <SHIPS> <value>", 2, mobq, C_MOD, NORM + CAP},
    {"mult [COUNTRY]", 1, mult, C_MOD, GOD},
    {"name <SHIPS> <NAME>", 0, name, C_MOD, NORM},
    {"nation", 0, nati, 0, VIS},
    {"navigate <SHIPS> <PATH|DESTINATION>", 1, navi, C_MOD, NORM + CAP},
    {"ndump <SECTS>", 0, ndump, 0, NORM},
    {"newcap <NAT> [<SECTOR>]", 0, new, C_MOD, GOD},
    {"neweff <SECTS>", 0, newe, C_MOD, NORM},
    {"newspaper [days]", 0, news, 0, VIS},
    {"nmap <SECTS|SHIP>", 0, map, C_MOD, VIS},
    {"nuke <SECTS>", 0, nuke, 0, NORM},
    {"offer <loan|treaty> <NAT> [<NUM> <DAYS> <IRATE>]", 1, offe, C_MOD,
     NORM + MONEY + CAP},
    {"offset <SECT|NAT>", 0, offs, C_MOD, GOD},
    {"order <SHIPS> <c|s|r|d|l> ", 1, orde, C_MOD, NORM + CAP},
    {"origin <SECT>", 1, orig, C_MOD, NORM},
    {"paradrop <cargo-PLANES> <fighter-PLANES> <ap-SECT> <PATH|DESTINATION>", 3, para, C_MOD, NORM + MONEY + CAP},
    {"path <SECT>", 0, path, 0, NORM},
    {"payoff <SHIPS>", 0, payo, C_MOD, NORM},
    {"pbmap <SECTS|UNIT> [s|l|p|*]", 0, map, C_MOD, NORM},
    {"pboard <PLANES>", 3, pboa, C_MOD, NORM + MONEY + CAP},
    {"pdump <PLANES> [<fields>]", 0, pdump, 0, NORM},
    {"plane <SECTS>", 0, plan, 0, NORM},
    {"players", 0, play, 0, VIS},
    {"pmap <SECTS|UNIT> [s|l|p|*]", 0, map, C_MOD, NORM},
    {"power [[\"new\" [<NUMBER OF COUNTRIES>]] | [\"country\" <COUNTRY NUMBER>]]", 0, powe, C_MOD, VIS},
    {"pray", 0, tele, C_MOD, NORM},
    {"production <SECTS>", 0, prod, 0, NORM},
    {"pstat <PLANES>", 0, pstat, 0, NORM},
    {"qorder <SHIPS>", 0, qorde, C_MOD, NORM + CAP},
    {"quit", 0, quit, 0, VIS},
    {"radar <SHIPS | SECTS>", 1, rada, 0, NORM + CAP},
    {"range <PLANES> <range>", 1, range, C_MOD, NORM + CAP},
    {"read", 0, rea, C_MOD, VIS},
    {"realm <number> [<SECTS>]", 0, real, C_MOD, NORM},
    {"recon <spy-PLANES> <fighter-PLANES> <ap-SECT> <PATH|DESTINATION>", 3,
     reco, C_MOD, NORM + MONEY + CAP},
    {"retreat <SHIPS|FLEET> <PATH> [i|t|s|h|b|d|u|c]", 1, retr, C_MOD,
     NORM + CAP},
    {"relations  [as-COUNTRY]", 0, rela, 0, VIS},
    {"reject <reject|accept> <announcements|mail|treaties|loans> <NAT>", 2,
     reje, C_MOD, NORM},
    {"repay <LOAN> <NUM>", 1, repa, C_MOD, NORM + MONEY + CAP},
    {"report <NATS>", 0, repo, 0, VIS},
    {"reset <lot #> <0|price>", 1, rese, C_MOD, NORM + CAP},
    {"resource <SECTS>", 0, reso, 0, NORM},
    {"route <COMM> <SECTS>", 1, rout, 0, NORM},
    {"sabotage <UNITS>", 1, sabo, C_MOD, NORM + MONEY + CAP},
    {"sail <SHIPS> <PATH>", 1, sail, C_MOD, NORM + CAP},
    {"satellite <PLANE> [<se|sh|l> [?cond&cond&...]]", 1, sate, 0,
     NORM + MONEY + CAP},
    {"sbmap <SECTS|UNIT> [s|l|p|*]", 0, map, C_MOD, NORM},
    {"scrap <s|p|l> <SHIPS|PLANES|UNITS>", 2, scra, C_MOD,
     NORM + MONEY + CAP},
    {"scuttle <s|p|l> <SHIPS|PLANES|UNITS>", 5, scut, C_MOD, NORM + CAP},
    {"sdump <SHIPS> [<fields>]", 0, sdump, 0, NORM},
    {"sect <SECTS>", 0, sct, 0, NORM},
    {"sell <COMM> <SECTS> <NUM> <NUM>", 1, sell, C_MOD, NORM + CAP},
    {"set <TYPE> <LIST> <PRICE>", 1, set, C_MOD, NORM + CAP},
    {"setsector <TYPE> <SECTS> <NUM>", 0, setsector, C_MOD, GOD},
    {"setresource <TYPE> <SECTS> <NUM>", 0, setres, C_MOD, GOD},
    {"shark <LOAN>", 25, shark, C_MOD, NORM + MONEY + CAP},
    {"sharebmap <CNUM/CNAME> <SECTS> [<des>]", 0, shar, C_MOD, NORM},
    {"ship <SHIPS>", 0, shi, 0, NORM},
    {"shoot <c|u> <SECTS> <NUMBER>", 3, shoo, C_MOD, NORM + MONEY + CAP},
    {"show <TYPE> <\"build\"|\"stats\"|\"cap\"> [<tech>]", 0, show, 0,
     VIS},
    {"shutdown [<minutes>] <disable update?>", 0, shut, 0, GOD},
    {"sinfrastructure <SECTS>", 0, sinfra, 0, VIS},
    {"skywatch <SECTS>", 1, skyw, C_MOD, NORM + CAP},
    {"smap <SECTS|UNIT> [s|l|p|*]", 0, map, C_MOD, NORM},
    {"sonar <SHIPS> <brief?>", 1, sona, C_MOD, NORM + CAP},
    {"sorder <SHIPS>", 0, sorde, C_MOD, NORM + CAP},
    {"spy <SECTS>", 1, spy, C_MOD, NORM + CAP},
    {"sstat <SHIPS>", 0, sstat, 0, NORM},
    {"start <SECTS>", 1, start, C_MOD, NORM + CAP},
    {"starvation [<SECTS>|l <UNITS>|s <SHIPS>]", 0, starve, 0, NORM},
    {"stop <SECTS>", 1, stop, C_MOD, NORM + CAP},
    {"strength <SECTS>", 1, stre, C_MOD, NORM},
    {"supply <LAND UNITS>", 1, supp, C_MOD, NORM + CAP},
    {"survey <COMM|VAR> <SECTS>", 0, surv, 0, NORM + CAP},
    {"swapsector <SECT> <SECT>", 0, swaps, C_MOD, GOD},
    {"sweep <sweep-PLANES> <fighter-PLANES> <ap-SECT> <PATH|DESTINATION>",
     3, reco, C_MOD, NORM + MONEY + CAP},
    {"telegram <NAT>", 0, tele, C_MOD, NORM},
    {"tend <COMM|\"land\"> <tender-SHIPS> <NUM|UNIT> <dest-SHIPS>", 1,
     tend, C_MOD, NORM + CAP},
    {"territory <SECTS> <NUMBER> [<FIELD>]", 0, terr, C_MOD, NORM},
    {"test <c|m|s|g|p|i|o|b|f|r|u> <SECT> <NUM> <PATH|DESTINATION>", 1,
     move, 0, NORM + CAP},
    {"threshold <COMM> <SECTS> <THRESH>", 1, thre, C_MOD, NORM},
    {"toggle [inform|flash|beep|coastwatch|sonar|techlists] [on|off]", 0,
     togg, C_MOD, VIS},
    {"torpedo <submarine-SHIPS> <target-SHIP>", 3, torp, C_MOD,
     NORM + MONEY + CAP},
    {"trade", 1, trad, C_MOD, NORM + CAP + MONEY},
    {"transport <\"nuke\"|\"plane\"> <SECT|PLANES> [<NUKETYPE> <number>] <PATH|DESTINATION>", 1, tran, C_MOD, NORM + CAP},
    {"treaty <TREATIES>", 0, trea, 0, NORM + MONEY + CAP},
    {"turn <\"on\"|\"off\"|\"mess\">", 0, turn, C_MOD, GOD},
    {"unload <COMM|\"land\"|\"plane\"> <SHIPS> <NUM|UNIT|PLANE>", 1, load,
     C_MOD, NORM + CAP},
    {"unsail <SHIPS>", 1, sail, C_MOD, NORM + CAP},
    {"update", 0, upda, 0, VIS},
    {"upgrade <SHIPS|LAND UNITS|PLANES>", 1, upgr, C_MOD,
     NORM + MONEY + CAP},
    {"version", 0, vers, 0, VIS},
    {"wait", 0, wai, C_MOD, GOD},
    {"wall [<message>]", 0, wall, C_MOD, NORM},
    {"wingadd <WING> <PLANES>", 1, wing, C_MOD, NORM},
    {"wipe <SECTS>", 1, wipe, C_MOD, NORM},
    {"wire [yes|no|<days>]", 0, rea, C_MOD, VIS},
    {"work <LAND UNITS> <AMOUNT>", 1, work, C_MOD, NORM + MONEY + CAP},
    {"zdone <y|n|c>", 0, zdon, C_MOD, NORM},
    {0, 0, 0, 0, 0}
};
