/* NetHack 3.6	were.c	$NHDT-Date: 1550524568 2019/02/18 21:16:08 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.23 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

void
were_change(mon)
register struct monst *mon;
{
    if (!is_were(mon->data))
        return;

    if (is_human(mon->data) || is_demon(mon->data)) {
        /* Rat King isn't a real were-creature */
        if (mon->data == &mons[PM_RAT_KING])
            return;

        if (!Protection_from_shape_changers
            && !rn2(night() ? (flags.moonphase == FULL_MOON ? 3 : 30)
                            : (flags.moonphase == FULL_MOON ? 10 : 50))) {
            new_were(mon); /* change into animal form */
            if (!Deaf && !canseemon(mon)) {
                const char *howler, *howl = "";

                switch (monsndx(mon->data)) {
                case PM_WEREWOLF:
                    howler = "wolf";
                    howl = "howling";
                    break;
                case PM_WEREJACKAL:
                    howler = "jackal";
                    howl = "howling";
                    break;
                case PM_WEREDEMON:
                    howler = "hell hound";
                    howl = "howling";
                    break;
                case PM_HUMAN_WEREPANTHER:	
                    howler = "panther";
                    howl = "yowling";
                    break;
		        case PM_HUMAN_WERETIGER:	
                    howler = "tiger";
                    howl = "yowling";
                    break;
                case PM_WEREBEAR:
                    howler = "bear";
                    howl = "roaring";
                    break;
                default:
                    howler = (char *) 0;
                    break;
                }
                if (howler) {
                    if (Hallucination)
                        You_hear("the moon %s like a %s", howl, howler);
                    else
                        You_hear("a %s %s at the moon.", howler, howl);
                    wake_nearto(mon->mx, mon->my, 4 * 4);
                }
            }
        }
    } else if (!rn2(30) || Protection_from_shape_changers) {
        new_were(mon); /* change back into human form */
    }
    /* update innate intrinsics (mainly Drain_resistance) */
    set_uasmon(); /* new_were() doesn't do this */
}

int
counter_were(pm)
int pm;
{
    switch (pm) {
    case PM_WEREWOLF:
        return PM_HUMAN_WEREWOLF;
    case PM_HUMAN_WEREWOLF:
        return PM_WEREWOLF;
    case PM_WEREJACKAL:
        return PM_HUMAN_WEREJACKAL;
    case PM_HUMAN_WEREJACKAL:
        return PM_WEREJACKAL;
    case PM_WERERAT:
        return PM_HUMAN_WERERAT;
    case PM_HUMAN_WERERAT:
        return PM_WERERAT;
    case PM_WEREDEMON:
        return PM_DEMON_WEREDEMON;
    case PM_DEMON_WEREDEMON:
        return PM_WEREDEMON;
    case PM_WEREPANTHER:      
        return(PM_HUMAN_WEREPANTHER);            
    case PM_HUMAN_WEREPANTHER:
        return(PM_WEREPANTHER);
    case PM_WERETIGER:        
        return(PM_HUMAN_WERETIGER);
    case PM_HUMAN_WERETIGER:  
        return(PM_WERETIGER);
    case PM_WERESNAKE:        
        return(PM_HUMAN_WERESNAKE);
    case PM_HUMAN_WERESNAKE:  
        return(PM_WERESNAKE);
    case PM_WERESPIDER:       
        return(PM_HUMAN_WERESPIDER);
    case PM_HUMAN_WERESPIDER: 
        return(PM_WERESPIDER);
    case PM_WEREBEAR:
        return PM_HUMAN_WEREBEAR;
    case PM_HUMAN_WEREBEAR:
        return PM_WEREBEAR;
    default:
        return NON_PM;
    }
}

/* convert monsters similar to werecritters into appropriate werebeast */
int
were_beastie(pm)
int pm;
{
    switch (pm) {
    case PM_WERERAT:
    case PM_SEWER_RAT:
    case PM_GIANT_RAT:
    case PM_RABID_RAT:
    case PM_ENORMOUS_RAT:
        return PM_WERERAT;
    case PM_WEREJACKAL:
    case PM_JACKAL:
    case PM_FOX:
    case PM_COYOTE:
    case PM_RABID_DOG:
        return PM_WEREJACKAL;
    case PM_WEREWOLF:
    case PM_WOLF:
    case PM_WARG:
    case PM_WINTER_WOLF:
        return PM_WEREWOLF;
    case PM_WEREDEMON:
    case PM_HELL_HOUND:
        return PM_WEREDEMON;
    case PM_WEREBEAR:
    case PM_BLACK_BEAR:
    /*case PM_DROP_BEAR:
    case PM_GRIZZLY_BEAR:*/
        return PM_WEREBEAR;
    default:
        break;
    }
    return NON_PM;
}

void
new_were(mon)
register struct monst *mon;
{
    register int pm;

    if (mon->data == &mons[PM_RAT_KING])
        return;

    pm = counter_were(monsndx(mon->data));
    if (pm < LOW_PM) {
        impossible("unknown lycanthrope %s.", mon->data->mname);
        return;
    }

    if (canseemon(mon) && !Hallucination)
        pline("%s changes into a %s.", Monnam(mon),
              is_human(&mons[pm]) ? "human" :
              is_demon(&mons[pm]) ? "demon" :
              pm == PM_WEREDEMON  ? "hell hound" : mons[pm].mname + 4);

    set_mon_data(mon, &mons[pm]);
    if (mon->msleeping || !mon->mcanmove) {
        /* transformation wakens and/or revitalizes */
        mon->msleeping = 0;
        mon->mfrozen = 0; /* not asleep or paralyzed */
        if (!mon->mstone || mon->mstone > 2)
            mon->mcanmove = 1;
    }
    /* regenerate by 1/4 of the lost hit points */
    mon->mhp += (mon->mhpmax - mon->mhp) / 4;
    newsym(mon->mx, mon->my);
    mon_break_armor(mon, FALSE);
    possibly_unwield(mon, FALSE);
}

/* were-creature (even you) summons a horde */
int
were_summon(ptr, yours, visible, genbuf)
struct permonst *ptr;
boolean yours;
int *visible; /* number of visible helpers created */
char *genbuf;
{
    int i, typ, pm = monsndx(ptr);
    struct monst *mtmp;
    int total = 0;

    *visible = 0;
    if (Protection_from_shape_changers && !yours)
        return 0;
    for (i = rnd(5); i > 0; i--) {
        switch (pm) {
            case PM_WERERAT:
            case PM_HUMAN_WERERAT:
                typ = rn2(3) ? PM_SEWER_RAT : rn2(3) ? PM_GIANT_RAT : PM_RABID_RAT;
                if (genbuf) Strcpy(genbuf, "rat");
                break;
            case PM_RAT_KING:
                typ = rn2(3) ? PM_SEWER_RAT
                             : rn2(3) ? PM_GIANT_RAT
                                      : rn2(2) ? PM_RABID_RAT : PM_ENORMOUS_RAT;
                if (genbuf)
                    Strcpy(genbuf, "rat");
                break;
            case PM_WEREJACKAL:
            case PM_HUMAN_WEREJACKAL:
                typ = PM_JACKAL;
                if (genbuf)
                    Strcpy(genbuf, "jackal");
                break;
            case PM_WEREWOLF:
            case PM_HUMAN_WEREWOLF:
            case PM_NOSFERATU:
                typ = rn2(5) ? PM_WOLF : rn2(2) ? PM_WINTER_WOLF : PM_RABID_WOLF;
                if (genbuf)
                    Strcpy(genbuf, "wolf");
                break;
            case PM_WEREDEMON:
            case PM_DEMON_WEREDEMON:
                typ = PM_HELL_HOUND;
                if (genbuf)
                    Strcpy(genbuf, "hell hound");
                break;
            case PM_WEREPANTHER:
            case PM_HUMAN_WEREPANTHER:
                typ = rn2(5) ? PM_JAGUAR : PM_PANTHER;
                if (genbuf) Strcpy(genbuf, "large cat");
                break;
            case PM_WERETIGER:
            case PM_HUMAN_WERETIGER:
                typ = rn2(5) ? PM_JAGUAR : PM_TIGER;
                if (genbuf) Strcpy(genbuf, "large cat");
                break;
            case PM_WERESNAKE:
            case PM_HUMAN_WERESNAKE:
                typ = rn2(5) ? PM_SNAKE : (rn2(3) ? PM_PIT_VIPER : PM_COBRA);
                if (genbuf) Strcpy(genbuf, "snake");
                break;
            case PM_WERESPIDER:
            case PM_HUMAN_WERESPIDER:
                typ = rn2(5) ? PM_CAVE_SPIDER : (rn2(3) ? PM_JUMPING_SPIDER : PM_GIANT_SPIDER);
                if (genbuf) Strcpy(genbuf, "spider");
                break;
            case PM_HUMAN_WEREBEAR:
            case PM_WEREBEAR:
                typ = rn2(5) ? PM_BABY_OWLBEAR :
                      (rn2(3) ? PM_BLACK_BEAR : PM_GRIZZLY_BEAR);
                if (genbuf)
                    Strcpy(genbuf, "bear");
                break;
            default:
                continue;
        }
        mtmp = makemon(&mons[typ], u.ux, u.uy, NO_MM_FLAGS);
        if (mtmp) {
            total++;
            if (canseemon(mtmp))
                *visible += 1;
        }
        if (yours && mtmp)
            (void) tamedog(mtmp, (struct obj *) 0);
    }
    return total;
}

void
you_were()
{
    char qbuf[QBUFSZ];
    boolean controllable_poly = Polymorph_control && !(Stunned || Afraid || Unaware);

    if (Unchanging || u.umonnum == u.ulycn)
        return;
    if (controllable_poly) {
        /* `+4' => skip "were" prefix to get name of beast */
        Sprintf(qbuf, "Do you want to change into %s?",
                an(mons[u.ulycn].mname + 4));
        if (!paranoid_query(ParanoidWerechange, qbuf))
            return;
    }
    (void) polymon(u.ulycn);
}

void
you_unwere(purify)
boolean purify;
{
    boolean controllable_poly = Polymorph_control && !(Stunned || Afraid || Unaware);

    if (purify) {
        You_feel("purified.");
        set_ulycn(NON_PM); /* cure lycanthropy */
    }
    if (!Unchanging && is_were(youmonst.data)
        && (!controllable_poly
            || !paranoid_query(ParanoidWerechange, "Remain in beast form?")))
        rehumanize();
    else if (is_were(youmonst.data) && !u.mtimedone)
        u.mtimedone = rn1(200, 200); /* 40% of initial were change */
}

/* lycanthropy is being caught or cured, but no shape change is involved */
void
set_ulycn(which)
int which;
{
    u.ulycn = which;
    /* add or remove lycanthrope's innate intrinsics (Drain_resistance) */
    set_uasmon();
}

/*were.c*/
