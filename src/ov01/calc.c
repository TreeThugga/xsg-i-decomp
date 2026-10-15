/*
 * OV01 original TU 4: 0x00a10bb8..0x00a191c0 (116 functions)
 */
#include "common.h"
#include "main/xgl_2.h"
#include "ov01/battle_init.h"
#include "calc.h"

/* calcCfEncount's own callees; both are still asm in this TU. */
extern void calcPartyStat(int, int);
extern void menuTimeSet(int);

/*
 * calcStatGet (this TU, still asm) as every caller in this file invokes it:
 * a status/flag lookup keyed by category and bit.
 */
extern int calcStatGet(ObjectTask *unit, int category, int flag);

/*
 * The one queued-command word calcTurnStart (this TU) reads
 * through unitCmdPtrGet's return (`lw $3,0x0($2)`), and the two further
 * words calcTurnStart writes through unitCmdSet's second argument
 * (`sw $2,0x0($29)` / `sw $3,0x4($29)` / `sw $0,0x8($29)` at 0x00a12c00,
 * both still asm in src/ov01/unit_cmd.c).
 */
typedef struct CalcCmdEntry CalcCmdEntry;

struct CalcCmdEntry {
    int selector;      /* +0x00 */
    int argument;      /* +0x04 */
    int extra;         /* +0x08: calcTurnStart always sets it to 0 */
    unsigned char unmodeled_0c[8]; /* +0x0C: unwritten by the one evidenced
                                     * call; only its total size (the local's
                                     * stack reservation before unitCmdSet)
                                     * is evidenced here. */
};

extern int calcAp(ObjectTask *unit, int ap);
extern CalcCmdEntry *unitCmdPtrGet(ObjectTask *unit);
extern void unitCmdSet(ObjectTask *unit, CalcCmdEntry *cmd);

/* calcDeadUnitNum: calcSpecChk's own callee, still asm in this TU. */
extern int calcDeadUnitNum(ObjectTask *unit, int flag);

/* calcBattleChk: calcWctDec's own callee, still asm in this TU. */
extern int calcBattleChk(ObjectTask *unit);

/* calcTrg2PE (this TU, defined below): calcDeadUnitNum's own callee. */
extern int calcTrg2PE(ObjectTask *unit, int trgFlags);

/*
 * calcWct/calcStatTurn/rnd: calcTurnEnd's own callees, defined later in
 * this TU (calcWct and rnd below, calcStatTurn still asm).
 */
extern int calcWct(ObjectTask *unit);
extern void calcStatTurn(ObjectTask *unit);
extern int rnd(int max);

/*
 * dataWpnGet's return type, as far as calcAttWpnChk reads it: a byte at
 * +0x10 that it tests for zero/nonzero (data_unit_org_get.c, dataWpnGet's
 * own TU, still asm, owns the rest of the layout).
 */
typedef struct WpnData WpnData;

struct WpnData {
    unsigned char unmodeled_00[0x10]; /* +0x00 */
    u8 equipped;                      /* +0x10: calcAttWpnChk's own check */
    unsigned char unmodeled_11[0x18 - 0x11];
    short dblAct;                     /* +0x18: calcDblActChk requires nonzero */
    unsigned char unmodeled_1a[0x1C - 0x1A];
    short phyDefense;                 /* +0x1C: calcPhyDefGet's accumulated term */
};

extern WpnData *dataWpnGet(int weaponId);

/*
 * calcSpline/calcSplineSub (both still asm in this TU) advance this
 * descriptor every frame; calcSplineInit sets it up with the value range,
 * the duration in frames as a float and a fixed 0.5 midpoint, the frame
 * counter starting at zero.
 */
typedef struct CalcSpline {
    int start;
    int end;
    float duration;
    float midpoint;
    int frame;
} CalcSpline;

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcCopyParaMake);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcPara2Org);

/* dataUnitOrgGet (src/ov01/data_unit_org_get.c, still asm), this TU's own
 * private view (see CalcCharParaData, src/ov01/calc.h). */
extern CalcCharParaData *dataUnitOrgGet(int charaId);

void calcPara2OrgSub(ObjectTask *unit) {
    CalcUnitParam *param;
    CalcCharParaData *origin;

    param = calcUPGet(unit);
    origin = dataUnitOrgGet(param->charaId);
    origin->hp = param->currentHp;
    origin->ep = param->currentEp;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcDeadReset);

CalcUnitParam *calcUPGet(ObjectTask *unit)
{
    return ((CalcActorRecord *)unit->work)->up;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHpDmg);

/*
 * calcTotalParaMenuSub (this TU, still asm, 0x00a11128): calcTotalParaMenu's
 * tail-called worker, given the same three arguments plus the per-charaId
 * table below.
 */
extern CalcCharParaData *calcTotalParaMenuSub(int charaId, int *attack, int *defense, unsigned char *table);

/* calcCopyParaMake copies twelve 0x20-byte units into this scratch table;
 * calcTotalParaMenuSub then reads the CalcCharParaData and CalcUnitParam
 * views from the same storage. */
static unsigned char D_00A57CE8[0x180];

CalcCharParaData *calcTotalParaMenu(int charaId, int *attack, int *defense)
{
    return calcTotalParaMenuSub(charaId, attack, defense, D_00A57CE8);
}

/*
 * calcTotalParaMenuSub/calcSkillEquChkAll's (this TU) own fake unit: a
 * stack-local ObjectTask/CalcActorRecord pair whose `work`/`up` chain
 * (calcUPGet's own traversal, 0x00a11010..0x00a1101c) is pointed at a
 * temporary CalcUnitParam-compatible record instead of a live battle unit,
 * so the two functions can reuse calcSkillInit/calcWpnAtkGet/calcPhyDefGet/
 * calcMagDefGet/calcSkillEquChk for a unit that has no ObjectTask.
 * calcTotalParaMenuSub allocates 0x80/0x110 bytes for the pair
 * (`addiu v0,sp,128`, sp+0x190 for the outer's own end), matching the
 * ObjectTask/CalcActorRecord prefixes it actually writes through plus the
 * task-specific payload xglTaskEntryNext callers leave unwritten here.
 */
typedef struct CalcScratchActor {
    CalcActorRecord actor;      /* +0x00 */
    unsigned char unmodeled_88[0x110 - 0x88];
} CalcScratchActor;

typedef struct CalcScratchUnit {
    ObjectTask task;            /* +0x00 */
    unsigned char unmodeled_14[0x80 - 0x14];
} CalcScratchUnit;

/* calcCopyParaMake/calcSkillInit/calcWpnAtkGet/calcPhyDefGet/calcMagDefGet:
 * calcTotalParaMenuSub's own callees, still asm in this TU except
 * calcMagDefGet (this TU, below); the latter four each read their argument
 * through calcUPGet, so calcTotalParaMenuSub hands them its own fake unit
 * above. */
extern void calcCopyParaMake(CalcCharParaData *origin, unsigned char *table);
extern void calcSkillInit(ObjectTask *unit);
extern int calcWpnAtkGet(ObjectTask *unit, int slot);
extern int calcPhyDefGet(ObjectTask *unit);
extern int calcMagDefGet(ObjectTask *unit);

CalcCharParaData *calcTotalParaMenuSub(int charaId, int *attack, int *defense, unsigned char *table)
{
    CalcScratchUnit fakeUnit;
    CalcScratchActor fakeActor;
    CalcCharParaData *scratch;
    CalcCharParaData *origin;
    int i;

    scratch = (CalcCharParaData *) table;
    fakeUnit.task.work = &fakeActor;
    calcCopyParaMake(dataUnitOrgGet(charaId), table);
    fakeActor.actor.up = (CalcUnitParam *) table;
    calcSkillInit(&fakeUnit.task);

    origin = dataUnitOrgGet(charaId);
    if (scratch->maxHp < origin->hp) {
        origin->hp = scratch->maxHp;
    }
    if (origin->ep > scratch->maxEp) {
        origin->ep = scratch->maxEp;
    }
    if (attack != 0) {
        for (i = 0; i < 3; i++) {
            attack[i] = calcWpnAtkGet(&fakeUnit.task, i);
        }
    }
    if (defense != 0) {
        defense[0] = calcPhyDefGet(&fakeUnit.task);
        defense[1] = calcMagDefGet(&fakeUnit.task);
    }
    return scratch;
}

/*
 * The record calcTakeEpGetMenu builds calcTotalParaMenu's recalculated
 * record into; nothing else in this TU fills the rest of it.
 */
typedef struct CalcTakeEpPara {
    unsigned char unmodeled_00[0x84];
    CalcCharParaData *totalPara; /* +0x84: calcTotalParaMenu's own return value */
    unsigned char unmodeled_88[0x110 - 0x88];
} CalcTakeEpPara;

/*
 * The context block calcTakeEpGetMenu builds for its only call to
 * calcTakeEpGet (this TU, still asm, 0x00a15670); nothing else in this TU
 * fills the rest of it.
 */
typedef struct CalcTakeEpArg {
    unsigned char unmodeled_00[0x10];
    CalcTakeEpPara *totalPara; /* +0x10: the record above */
    unsigned char unmodeled_14[0x80 - 0x14];
} CalcTakeEpArg;

extern int calcTakeEpGet(CalcTakeEpArg *arg, int tecId);

/* dataEthGet (src/ov01/data_unit_org_get.c, still asm), this TU's own
 * private view (see CalcEtherStatus, src/ov01/calc.h). */
extern CalcEtherStatus *dataEthGet(int etherId);

int calcTakeEpGetMenu(int charaId, int etherId) {
    CalcTakeEpArg arg;
    CalcTakeEpPara para;

    arg.totalPara = &para;
    para.totalPara = calcTotalParaMenu(charaId, 0, 0);
    return calcTakeEpGet(&arg, dataEthGet(etherId)->tecId);
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcUseEtherMenu);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcUseItemMenu);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcUseItem);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcSkillInit);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcSkillInitSub);

/* calcSkillSub/AccessoryData.skillId (both this TU, defined below): the
 * per-skill type/level mask test and the accessory's granted skill id. */
extern int calcSkillSub(ObjectTask *unit, int skillId, int typeMask, int levelMask);
extern AccessoryData *dataAccGet(int accessoryId);

int calcSkill(ObjectTask *unit, int kind, int mask)
{
    CalcUnitParam *up;
    int result;
    int i;
    int accId;
    int skillId;

    result = 0;
    for (i = 0; i < 3; i++) {
        up = calcUPGet(unit);
        accId = up->accessoryId[i];
        if (accId != 0) {
            skillId = dataAccGet(accId)->skillId;
            if (skillId != 0) {
                result += calcSkillSub(unit, skillId, kind, mask);
            }
        }
    }
    for (i = 0; i < 3; i++) {
        up = calcUPGet(unit);
        skillId = up->skillId[i];
        if (skillId != 0) {
            result += calcSkillSub(unit, skillId, kind, mask);
        }
    }
    return result;
}

/*
 * dataSklGet's return record (src/ov01/data_unit_org_get.c, still asm), as
 * far as calcSkillSub reads it: two halfword bit sets.
 */
typedef struct SkillData SkillData;

struct SkillData {
    u16 typeFlags;   /* +0x00: calcSkillSub's own mask test */
    u16 levelFlags;  /* +0x02: calcSkillSub returns bits 0-6 of this field */
};

extern SkillData *dataSklGet(int skillId);

int calcSkillSub(ObjectTask *unit, int skillId, int typeMask, int levelMask) {
    SkillData *skill;
    unsigned short levelFlags;
    int result;

    skill = dataSklGet(skillId);
    calcUPGet(unit);
    result = 0;
    if (skill->typeFlags & typeMask) {
        levelFlags = skill->levelFlags;
        result = (levelFlags & levelMask) != 0 ? (levelFlags & 0x7F) : 0;
    }
    return result;
}

int calcAccEquChk(ObjectTask *unit, int accId)
{
    CalcUnitParam *param;
    int i;

    for (i = 0; i < 3; i++)
    {
        param = calcUPGet(unit);
        if (accId == param->accessoryId[i])
        {
            return 1;
        }
    }
    return 0;
}

/* dataAccGet (src/ov01/data_unit_org_get.c, still asm), this TU's own
 * private view (see AccessoryData, src/ov01/calc.h). */
extern AccessoryData *dataAccGet(int accessoryId);

int calcSkillEquChk(ObjectTask *unit, int skillId)
{
    CalcUnitParam *param;
    int i;
    short accessoryId;

    for (i = 0; i < 3; i++)
    {
        param = calcUPGet(unit);
        accessoryId = param->accessoryId[i];
        if (accessoryId != 0)
        {
            if (skillId == dataAccGet(accessoryId)->skillId)
            {
                return 1;
            }
        }
    }

    for (i = 0; i < 3; i++)
    {
        param = calcUPGet(unit);
        if (skillId == param->skillId[i])
        {
            return 1;
        }
    }

    return 0;
}

/* calcSkillEquChk/unitTblGet: calcSkillEquChkAll's own callees, still asm
 * in this TU. calcSkillEquChk's second call below hands it the same fake
 * unit calcMagDefGet (this TU) takes; unitTblGet writes the active unit
 * table's base address through its second argument and returns its count. */
extern int calcSkillEquChk(ObjectTask *unit, int skillId);
extern int unitTblGet(int type, ObjectTask ***table);

int calcSkillEquChkAll(int skillId)
{
    CalcScratchUnit fakeUnit;
    CalcScratchActor fakeActor;
    ObjectTask **table;
    ObjectTask *unit;
    int count;
    int i;
    int found;

    i = 0;
    count = unitTblGet(0, &table);
    found = 0;
    if (count > 0) {
        for (; i < count; i++) {
            unit = table[i];
            if (unit != 0) {
                found = 1;
                if (calcSkillEquChk(unit, skillId) == 0) {
                    if (calcUPGet(table[i])->flags & 0x40) {
                        fakeUnit.task.work = &fakeActor;
                        fakeActor.actor.up = calcUPGet(table[i])->linkedUnit;
                        if (calcSkillEquChk(&fakeUnit.task, skillId) != 0) {
                            return 1;
                        }
                    }
                } else {
                    return found;
                }
            }
        }
        return 0;
    }
    return found;
}

int calcAp(ObjectTask *unit, int ap)
{
    int current = calcUPGet(unit)->dblActKind;

    if (ap < 0 && current < -ap) {
        return 0;
    }
    current += ap;
    if (current > 6) {
        current = 6;
    }
    calcUPGet(unit)->dblActKind = current;
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHp);

int calcEp(ObjectTask *unit, int delta)
{
    int ep;

    ep = (short) calcUPGet(unit)->currentEp;
    ep += delta;
    if (ep < 0)
    {
        ep = 0;
    }
    else if (calcUPGet(unit)->maxEp < ep)
    {
        ep = calcUPGet(unit)->maxEp;
    }
    calcUPGet(unit)->currentEp = ep;
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcBp);

/* calcBp (this TU, still asm): unit and the BP-decay timeline id. */
extern void calcBp(ObjectTask *unit, int tid);

/*
 * dataTecGet's return, as far as calcBp2Tid reads it: a byte at +0x11
 * selecting the BP timeline (data_unit_org_get.c, dataTecGet's own TU,
 * still asm, owns the rest of the layout).
 */
typedef struct TecKindView TecKindView;

struct TecKindView {
    unsigned char unmodeled_00[1];
    signed char epCost;     /* +0x01: calcTakeEpGet's base term, `lb $16,0x1($2)` */
    u16 flags;              /* +0x02: bit 0x800 rules out a counter */
    unsigned char unmodeled_04[0x11 - 0x04];
    /*
     * +0x11: calcBp2Tid's timeline selector. calcBp2Tid.s (0x00a1267c)
     * reads it with `lb $2,0x11($2)` (signed byte load), hence `signed char`.
     */
    signed char kind;
};

extern TecKindView *dataTecGet(int tecId);
extern int menuTimeGet(void);
extern const char D_00A443D8[]; /* "** BP up: EventTimer G (%d)\n" */

int calcBp2Tid(ObjectTask *unit, int tecId)
{
    BattleActor *actor;
    int kind;
    int tid;

    actor = (BattleActor *) unit->work;
    if (actor->flags & 0x40) {
        return 0;
    }

    kind = dataTecGet(tecId)->kind;
    if (kind == 0) {
        tid = 4;
    } else if (kind == 1) {
        tid = 6;
    } else if (kind == 2) {
        tid = 10;
    } else {
        tid = 0;
    }

    if (menuTimeGet() == 2) {
        tid *= 3;
        printf(D_00A443D8, tid);
    }

    calcBp(unit, tid);
    return 1;
}

int calcBc(BattleUnit *unit, int delta)
{
    int bc;

    bc = calcUPGet((ObjectTask *) unit)->bcValue;
    bc += delta;
    if (bc < 0)
    {
        bc = 0;
    }
    else if (calcUPGet((ObjectTask *) unit)->bcLimit < bc)
    {
        bc = calcUPGet((ObjectTask *) unit)->bcLimit;
    }
    calcUPGet((ObjectTask *) unit)->bcValue = bc;
    return 1;
}

int calcCounter(ObjectTask *unit, int tecId)
{
    int rate;

    if (dataTecGet(tecId)->flags & 0x800) {
        return 0;
    }
    rate = calcUPGet(unit)->counterRate;
    rate += calcSkill(unit, 0x4000, 0x4000);
    if (rate > 100) {
        rate = 100;
    }
    return rnd(100) < rate;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcBoostChk);

extern int *actStockGet(void);
/* calcBc returns int, not void: its original asm sets $2 = 1
 * (`addiu $2,$0,0x1`, build/ov01/asm/nonmatchings/ov01/calc/calcBc.s) right
 * before the epilogue on every path. calcBoost (below, this TU) is the only
 * caller and discards the result. */
int calcBc(BattleUnit *unit, int delta);
void decoEffCall(BattleUnit *unit, int effectId);
int sndSysSePlay(int soundId);

int calcBoost(BattleUnit *unit, int mode) {
    CalcActorRecord *work;

    if (mode == 1) {
        calcBc(unit, -1);
    }
    work = (CalcActorRecord *) ((ObjectTask *) unit)->work;
    work->flags |= 0x400;
    ((CalcActStock *) actStockGet())->slot[mode] = (ObjectTask *) unit;
    decoEffCall(unit, 0x64);
    return sndSysSePlay(9);
}

int calcIdol2Chk(ObjectTask *unit)
{
    int hp = (short) calcUPGet(unit)->currentHp;

    if ((((BattleActor *) unit->work)->flags & 0x40) == 0) {
        if (hp < calcUPGet(unit)->maxHp / 5) {
            return 1;
        }
    }
    return 0;
}

int calcTuigekiChk(ObjectTask *unit)
{
    return calcStatGet(unit, 7, 0x80) != 0;
}

int calcSpecChk(ObjectTask *unit, int specId)
{
    int result;

    if (specId == 0x34 && calcDeadUnitNum(unit, 0x40) <= 0)
    {
        return 0;
    }

    if (specId != 0x5A || (result = calcStatGet(unit, 7, 2)) != 0)
    {
        if (specId != 0x59 || (result = calcStatGet(unit, 7, 1)) != 0)
        {
            result = 1;
        }
    }

    return result;
}

/*
 * dataItmGet's return, as far as calcItmEnableChk and calcDItemChk read it:
 * the item's target-selection flags at +0x08 (`lbu $5,0x8($2)`), handed on
 * to calcTgtUnitTbl, and a one-bit flag at the bottom of +0x09 that
 * calcDItemChk tests (`lbu $3,0x9($2)`, `andi 0x1`, `andi 0xFF`).
 */
typedef struct ItmTrgView {
    unsigned char unmodeled_00[0x08];
    u8 trgFlags;        /* +0x08 */
    u8 dItemEnable : 1; /* +0x09 bit 0 */
} ItmTrgView;

extern ItmTrgView *dataItmGet(int itmId);
extern int calcTgtUnitTbl(ObjectTask *unit, int trgFlags, ObjectTask **table);
extern int cfEventGet(void);
extern int dataItmBoxChk(int itmId);

int calcDItemChk(ObjectTask *unit, int itmId)
{
    if (calcUPGet(unit)->dblActKind >= 6
        && (dataItmGet(itmId)->trgFlags & 0x10) == 0
        && dataItmBoxChk(itmId) >= 2
        && dataItmGet(itmId)->dItemEnable) {
        return 1;
    }
    return 0;
}

void calcTurnStart(ObjectTask *unit)
{
    int ap = 4;
    int apForSelect;
    CalcActorRecord *work = (CalcActorRecord *) unit->work;

    if (calcStatGet(unit, 2, 0x10) != 0 || calcStatGet(unit, 6, 0x1000) != 0)
    {
        ap = 6;
    }

    if (calcStatGet(unit, 0, 0x40) != 0 || calcStatGet(unit, 4, 0x20) != 0)
    {
        ap /= 2;
    }

    apForSelect = ap;
    calcAp(unit, calcStatGet(unit, 7, 0x20) ? apForSelect + 1 : ap);

    if (((CalcActorRecord *) unit->work)->flags & 0x800)
    {
        CalcCmdEntry *cmd = unitCmdPtrGet(unit);

        if (cmd->selector == 0)
        {
            CalcCmdEntry newCmd;

            newCmd.selector = 0xD;
            newCmd.argument = 0x1C;
            newCmd.extra = 0;
            unitCmdSet(unit, &newCmd);
        }

        work->flags &= ~0x800;
    }
}

void calcTurnEnd(ObjectTask *unit)
{
    CalcUnitParam *up;
    short wct;

    ((CalcActorRecord *) unit->work)->flags &= ~0x6418;

    up = calcUPGet(unit);
    calcUPGet(unit)->waitCount = wct = calcWct(unit);
    up->waitCountBase = wct;

    calcUPGet(unit)->turnBias = 0;

    calcStatTurn(unit);
}

int calcWct(ObjectTask *unit)
{
    int base;
    int accum = 0;

    base = (calcUPGet(unit)->turnBias - calcUPGet(unit)->agility) * 9 + 0xB4;

    if (calcStatGet(unit, 0, 0x1000) != 0 || calcStatGet(unit, 4, 0x800) != 0)
    {
        accum = base / 2;
    }

    if (calcStatGet(unit, 2, 0x100) != 0 || calcStatGet(unit, 6, 0x4000) != 0)
    {
        accum -= base / 4;
    }

    if (calcStatGet(unit, 2, 0x80) != 0 || calcStatGet(unit, 6, 0x2000) != 0)
    {
        accum -= base / 2;
    }

    return base + accum;
}

int calcWctDec(ObjectTask *unit)
{
    int wait = calcUPGet(unit)->waitCount;

    if (calcBattleChk(unit) != 0)
    {
        int decremented = wait - ((rnd(100) < 30) ? 1 : 0);

        wait = decremented - 1;
        if (wait < 0)
        {
            wait = 0;
        }
    }

    calcUPGet(unit)->waitCount = wait;
    return wait;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHitMotChk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHitDmg);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHitRate);

int calcHitParaGet(ObjectTask *unit) {
    int hitRate;

    hitRate = (signed char) calcUPGet(unit)->hitStat;
    if (calcStatGet(unit, 0, 0x400) != 0 || calcStatGet(unit, 4, 0x200) != 0) {
        hitRate /= 2;
    } else if (calcStatGet(unit, 2, 0x40) != 0) {
        hitRate += hitRate / 4;
    }
    return hitRate;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHit);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcWpnAtkGet);

int calcPhyDefGet(ObjectTask *unit)
{
    int def;
    int i;
    short accessoryId;
    short weaponId;

    def = 0;
    for (i = 0; i < 3; i++)
    {
        accessoryId = calcUPGet(unit)->accessoryId[i];
        if (accessoryId != 0)
        {
            def += dataAccGet(accessoryId)->phyDefense;
        }
    }

    for (i = 0; i < 3; i++)
    {
        weaponId = calcUPGet(unit)->weaponId[i];
        if (weaponId != 0)
        {
            def += dataWpnGet(weaponId)->phyDefense;
        }
    }

    return def;
}

/* dataAccGet (src/ov01/data_unit_org_get.c, still asm), this TU's own
 * private view (see AccessoryData, src/ov01/calc.h). */
extern AccessoryData *dataAccGet(int accessoryId);

int calcMagDefGet(ObjectTask *unit)
{
    int magicDef;
    int i;
    short accessoryId;

    magicDef = 0;
    for (i = 0; i < 3; i++) {
        accessoryId = calcUPGet(unit)->accessoryId[i];
        if (accessoryId != 0) {
            magicDef += dataAccGet(accessoryId)->magicDefense;
        }
    }
    return magicDef;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcPowLevGet);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcPlusAglGet);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcDmg);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", futtobiChk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStat);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStatAtk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStatClr);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcCrit);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcSpec);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcLinePos);

int calcRevHit(int hitMode)
{
    int mode = hitMode & 0xFF;
    int side = (hitMode >> 8) & 0xFF;

    switch (mode)
    {
        case 1:
            mode = 2;
            break;

        case 2:
            mode = 1;
            break;

        case 3:
            mode = 4;
            break;

        case 4:
            mode = 3;
            break;
    }

    return mode | (side << 8);
}

int calcAtrChg(ObjectTask *unit, int attribute) {
    int result;

    result = attribute;
    if (calcStatGet(unit, 3, 0x4000) != 0) {
        result = (result & ~0x1F) | 0x10;
    } else if (calcStatGet(unit, 3, 0x2000) != 0) {
        result = (result & ~0x1F) | 8;
    } else if (calcStatGet(unit, 3, 0x1000) != 0) {
        result = (result & ~0x1F) | 4;
    }
    return result;
}

/* calcSlotAChk (below, this TU): unit and the technique id. */
extern int calcSlotAChk(ObjectTask *unit, int techId);

/* tecId is `int`, not `short`: with `short` the entry block re-sign-extends
 * it (`sll $5,$5,16`/`sra $18,$5,16` replacing the original's plain
 * `daddu $18,$5,$0`), fndiff on form calcTakeEpGet (2026-10-14). */
int calcTakeEpGet(CalcTakeEpArg *arg, int tecId)
{
    ObjectTask *unit = (ObjectTask *) arg;
    int ep;

    ep = dataTecGet(tecId)->epCost;
    if (calcStatGet(unit, 1, 0x10)) {
        ep *= 2;
    }
    if (calcStatGet(unit, 3, 0x200)) {
        ep /= 2;
    }
    if (calcStatGet(unit, 3, 0x40)) {
        ep *= 2;
    }
    if (calcStatGet(unit, 7, 0x100) && calcSlotAChk(unit, tecId)) {
        ep = 0;
    }

    return (ep < 100) ? ep : 99;
}

int calcSlotAChk(ObjectTask *unit, int techId)
{
    CalcUnitParam *param;
    int i;

    for (i = 0; i < 2; i++)
    {
        param = calcUPGet(unit);
        if (param->slotTechniqueId[i] == techId)
        {
            return 1;
        }
    }
    return 0;
}

int calcAttWpnChk(int weaponId)
{
    return dataWpnGet(weaponId)->equipped != 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHitItem);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcDmgItem);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", tgtFindFirst);

/*
 * The battle command tgtFindFirst (this TU, still asm) passes on to
 * tgtFindChk, as far as either reads it: tgtFindFirst resolves +0x00
 * through dataTecGet and +0x08 through dataItmGet, takes the target from
 * +0x04, and branches on mode 3 (an item, as in sndConvSpid's mode 3).
 */
typedef struct TgtCmd {
    int tecId;          /* +0x00 */
    ObjectTask *target; /* +0x04 */
    int itmId;          /* +0x08 */
    int mode;           /* +0x0C: 3 = item */
} TgtCmd;

/*
 * calcAtkChk (this TU, still asm): unit, target and technique id. Its
 * return type is narrower than int: tgtFindChk calls it with jal and returns
 * through its own epilogue, which a same-width int return would turn into a
 * sibling call (`j calcAtkChk`). Every narrow width gives the same bytes here.
 */
extern short calcAtkChk(ObjectTask *unit, ObjectTask *target, int tecId);

int tgtFindChk(ObjectTask *unit, ObjectTask *target, TgtCmd *cmd)
{
    if (cmd->mode != 3) {
        return calcAtkChk(unit, target, cmd->tecId);
    }
    return 1;
}

/*
 * tgtFindFirst's own candidate table (this TU, still asm): tgtNum entries
 * of tgtTbl are filled in target-search order, tgtIdx walks them out one at
 * a time. The gap between tgtTbl and tgtIdx (0x00a57e68..0x00a57e88, 0x20
 * bytes) is this array's only size evidence.
 */
static ObjectTask *tgtTbl[8];
static int tgtIdx;
static int tgtNum;

ObjectTask *tgtFindNext(void)
{
    if (tgtIdx >= tgtNum) {
        return 0;
    }
    return tgtTbl[tgtIdx++];
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcMoveChk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcAtkChk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcA2Chk);

/* dataPosLineGet (src/ov01/data_unit_org_get.c, this TU's own private view
 * of its PosTblEntry.line member): the position's battle line index. */
extern short dataPosLineGet(int posId);

int calcLineChk(ObjectTask *unit)
{
    ObjectTask **table;
    short myLine;
    short otherLine;
    int side;
    int count;
    int i;

    myLine = dataPosLineGet(calcUPGet(unit)->posId);
    if (myLine == 0)
    {
        return 1;
    }

    if (((CalcActorRecord *) unit->work)->flags & 0x40)
    {
        side = 1;
    }
    else
    {
        side = 0;
    }
    i = 0;
    count = unitTblGet(side, &table);
    if (count > 0)
    {
        for (; i < count; i++)
        {
            if (table[i] != 0)
            {
                if ((((CalcActorRecord *) table[i]->work)->flags & 0x80) == 0)
                {
                    otherLine = dataPosLineGet(calcUPGet(table[i])->posId);
                    if (otherLine != 0 && otherLine < myLine)
                    {
                        return 0;
                    }
                }
            }
        }
    }
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcBackLineChk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcWpnEnableChk);

int calcWpnUse(ObjectTask *unit, int slot)
{
    CalcUnitParam *up;

    if (slot != -1) {
        if (slot >= 4) {
            return 1;
        }
        up = calcUPGet(unit);
        if (calcAttWpnChk(up->weaponId[slot]) != 0) {
            if (up->wpnSlotValue[slot] != 0xFF) {
                up->wpnSlotValue[slot]--;
            }
            if (up->wpnSlotValue[slot] < 0) {
                up->wpnSlotValue[slot] = 0;
            }
        }
    }
    return 1;
}

int calcWpnEquipOrg(int charaId, int slot, int weaponId, int hand) {
    CalcCharParaData *origin;

    if (slot >= 3 || charaId >= 0x21) {
        return 0;
    }
    origin = dataUnitOrgGet(charaId);
    origin->weaponId[slot] = (short) weaponId;
    origin->hand[slot] = (signed char) hand;
    return 1;
}

int calcAccEquipOrg(int charaId, int slot, int accessoryId) {
    if (slot >= 3 || charaId >= 0x21) {
        return 0;
    }
    dataUnitOrgGet(charaId)->accessoryId[slot] = (short) accessoryId;
    return 1;
}

int calcAttEquipOrg(int charaId, int slot, int attachmentId) {
    if (slot >= 3 || charaId >= 0x21) {
        return 0;
    }
    dataUnitOrgGet(charaId)->attachmentId[slot] = (short) attachmentId;
    return 1;
}

int calcSpecEquipOrg(int charaId, int slot, int specialId) {
    if (slot >= 6 || charaId >= 0x21) {
        return 0;
    }
    dataUnitOrgGet(charaId)->specialId[slot] = (short) specialId;
    return 1;
}

int calcEtherEquipOrg(int charaId, int slot, int etherId) {
    if (slot >= 0xC || charaId >= 0x21) {
        return 0;
    }
    dataUnitOrgGet(charaId)->etherId[slot] = (short) etherId;
    return 1;
}

int calcSkillEquipOrg(int charaId, int slot, int skillId) {
    if (slot >= 3 || charaId >= 0x21) {
        return 0;
    }
    dataUnitOrgGet(charaId)->skillId[slot] = (short) skillId;
    return 1;
}

int calcAgwsEquipOrg(int charaId, int agwsId) {
    if ((unsigned int) (agwsId - 0x11) >= 0x10 || charaId >= 0x11) {
        return 0;
    }
    dataUnitOrgGet(charaId)->agwsId = (short) agwsId;
    return 1;
}

int calcEngineEquipOrg(int charaId, int engineId)
{
    CalcCharParaData *origin = dataUnitOrgGet(charaId);

    if ((origin->flags & 0x40) == 0) {
        return 0;
    }
    origin->engineId = (short) engineId;
    return 1;
}

int calcFrameEquipOrg(int charaId, int frameId)
{
    CalcCharParaData *origin = dataUnitOrgGet(charaId);

    if ((origin->flags & 0x40) == 0) {
        return 0;
    }
    origin->frameId = (short) frameId;
    return 1;
}

/*
 * The CalcUnitParam.dblActKind value calcDblActChk requires (`addiu $4,$0,6`
 * against `lb $2,0x3A($17)`); nothing else in this TU names it.
 */
#define CALC_DBL_ACT_KIND 6

/* calcSameWpnChk (this TU, still asm): a weapon slot of unit, or -1. */
extern int calcSameWpnChk(ObjectTask *unit);

int calcDblActChk(ObjectTask *unit)
{
    CalcUnitParam *up;
    int slot;

    up = calcUPGet(unit);
    slot = calcSameWpnChk(unit);
    if (slot != -1 && up->dblActKind == CALC_DBL_ACT_KIND
        && dataWpnGet(up->weaponId[slot])->dblAct != 0) {
        if (calcAttWpnChk(up->weaponId[slot]) == 0 || up->wpnSlotValue[slot] > 0) {
            return 1;
        }
        return 0;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcSameWpnChk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcEthEnableChk);

int calcItmEnableChk(ObjectTask *unit, int itmId)
{
    if (calcTgtUnitTbl(unit, dataItmGet(itmId)->trgFlags, 0) == 0) {
        return 0;
    }
    if (itmId == 0x10 && (cfEventGet() & 8) != 0) {
        return 0;
    }
    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcTgtUnitTbl);

int calcDeadUnitNum(ObjectTask *unit, int flag)
{
    ObjectTask **table;
    int count;
    int i;
    int result;

    count = unitTblGet(calcTrg2PE(unit, flag), &table);
    result = 0;
    if (count > 0)
    {
        for (i = 0; i < count; i++)
        {
            if (table[i] != 0)
            {
                if ((((CalcActorRecord *)table[i]->work)->flags & 0x80) != 0)
                {
                    result++;
                }
            }
        }
    }
    return result;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcBattleChk);

/*
 * Answers a target-selection flag word (cmdAtktbl, calcTgtUnitTbl and
 * calcDeadUnitNum supply it) against the unit's own side bit, bit 0x40 of
 * CalcActorRecord's flags: a selection of 0x40, and any word carrying 0x08,
 * asks whether the unit is on that side; a selection of 0x80 asks whether it
 * is not; every other selection answers 2.
 */
int calcTrg2PE(ObjectTask *unit, int trgFlags)
{
    if ((trgFlags & 0xC0) == 0x40 || (trgFlags & 8) != 0) {
        if ((((CalcActorRecord *)unit->work)->flags & 0x40) != 0) {
            return 1;
        }
        return 0;
    }
    if ((trgFlags & 0xC0) == 0x80) {
        if ((((CalcActorRecord *)unit->work)->flags & 0x40) == 0) {
            return 1;
        }
        return 0;
    }
    return 2;
}

/* calcStatIdx2Bit/calcStatReset: calcStatTurn's own callees, defined later
 * in this TU (calcStatIdx2Bit below) or still asm (calcStatReset). */
extern int calcStatIdx2Bit(int statIdx);
extern void calcStatReset(ObjectTask *unit, int category, int mask, int useAltMask);

void calcStatTurn(ObjectTask *unit)
{
    CalcUnitParam *param;
    int category;
    int statIdx;
    unsigned char remaining;

    param = calcUPGet(unit);
    for (category = 0; category < 8; category++) {
        for (statIdx = 0; statIdx < 16; statIdx++) {
            if (param->statActiveMask[category] & calcStatIdx2Bit(statIdx)) {
                remaining = param->statTurnCount[category][statIdx];
                if (remaining != 0xFF) {
                    remaining--;
                    if (remaining == 0) {
                        calcStatReset(unit, category, calcStatIdx2Bit(statIdx), 0);
                    }
                    param->statTurnCount[category][statIdx] = remaining;
                }
            }
        }
    }
}

int calcStatProtectGet(ObjectTask *unit, int group, int mask)
{
    return calcUPGet(unit)->statProtect[group] & mask;
}

void calcStatProtectSet(ObjectTask *unit, int group, int mask)
{
    calcUPGet(unit)->statProtect[group] |= mask;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStatGet);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStatGetNorm);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStatSetChk);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStatSet);

/* calcStatBit2Idx (this TU, defined below): shared with calcStatTurn. */
extern int calcStatBit2Idx(int statBit);

void calcStatReset(ObjectTask *unit, int category, int mask, int useAltMask)
{
    CalcUnitParam *up;
    int idx;

    up = calcUPGet(unit);
    if (useAltMask == 0) {
        up->statActiveMask[category] &= ~mask;
        idx = calcStatBit2Idx(mask);
        up->statTurnCount[category][idx] = 0;
    } else {
        up->statAltMask[category] &= ~mask;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcStatDefTurnGet);

/*
 * The inverse of calcStatIdx2Bit below: the index of statBit's lowest set bit
 * among the 16 status bits calcStatSet, calcStatReset and calcStatDefTurnGet
 * index with it, or 16 when statBit holds none of them.
 */
int calcStatBit2Idx(int statBit)
{
    int statIdx;
    int mask;

    mask = 1;
    for (statIdx = 0; statIdx < 16; statIdx++) {
        if (statBit & mask) {
            break;
        }
        mask <<= 1;
    }
    return statIdx;
}

int calcStatIdx2Bit(int statIdx)
{
    return 1 << statIdx;
}

void calcStatReset(ObjectTask *unit, int category, int mask, int useAltMask);

void calcStatResetCha(ObjectTask *unit) {
    calcStatReset(unit, 0, 0x7FFF, 0);
    calcStatReset(unit, 1, 0x7FFF, 0);
    calcStatReset(unit, 2, 0x7FFF, 0);
    calcStatReset(unit, 3, 0x7FFF, 0);
}

void calcStatResetAll(ObjectTask *unit) {
    calcStatReset(unit, 0, 0x7FFF, 0);
    calcStatReset(unit, 1, 0x7FFF, 0);
    calcStatReset(unit, 2, 0x7FFF, 0);
    calcStatReset(unit, 3, 0x7FFF, 0);
    calcStatReset(unit, 4, 0x7FFF, 0);
    calcStatReset(unit, 5, 0x7FFF, 0);
    calcStatReset(unit, 6, 0x7FFF, 0);
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcResult);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcResultSub);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcLvUp);

/*
 * entryPhase40 (src/ov01/entry_first_init.c) calls this as a bare statement
 * and never reads a result, but the original ends with a real jal to
 * calcPartyStat plus its own separate epilogue rather than a tail jump into
 * it, which this int return type (left unset) reproduces.
 */
int calcCfEncount(int flags)
{
    if (flags & 1)
    {
        calcPartyStat(0, 0);
        menuTimeSet(1);
    }
    if (flags & 2)
    {
        calcPartyStat(0, 0);
        calcPartyStat(0, 1);
    }
}

void calcPartyStat(int side, int mode)
{
    ObjectTask **table;
    ObjectTask *unit;
    int count;
    int i;

    count = unitTblGet(side, &table);
    for (i = 0; i < count; i++) {
        unit = table[i];
        if (unit != 0) {
            switch (mode) {
            case 0:
                calcUPGet(unit)->waitCount = 0;
                break;
            case 1:
                calcUPGet(unit)->bcValue++;
                break;
            }
        }
    }
}

int calcEtherDisableGet(ObjectTask *unit, int bit)
{
    u16 mask;

    mask = 1 << bit;
    return calcUPGet(unit)->etherDisableFlags & mask;
}

void calcEtherDisableSet(ObjectTask *unit, int bit)
{
    CalcUnitParam *param;
    u16 mask;

    mask = 1 << bit;
    param = calcUPGet(unit);
    param->etherDisableFlags |= mask;
}

int rnd(int max)
{
    return xglSRand() % (max + 1);
}

void calcSplineInit(CalcSpline *spline, int start, int end, int duration)
{
    spline->start = start;
    spline->end = end;
    spline->duration = (float) duration;
    spline->midpoint = 0.5f;
    spline->frame = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcSpline);

/*
 * calcLinerSub's own point table, as far as it reads it: a pointer to an
 * array of Vector4 entries (calcSpline's own callee, still asm in this TU).
 * Vector4 is the shared four-float record from include/shared.h, used the
 * same way (an indexed array, only x/y/z read) by src/ov01/unit_cmd.c's
 * cmdTarget fields.
 *
 * The original (asm/nonmatchings/ov01/calc/calcLinerSub.s, 0x00a18a98):
 * `sll $6,$6,4` scales idx by 16 before `addu $6,$6,$2` adds it to the base,
 * proving the 16-byte stride per entry; the two `ldl/ldr` pairs at
 * `0x7($6)`/`0x0($6)` and `0xf($6)`/`0x8($6)` copy points[idx] and
 * points[idx+1] (0x10 bytes each) whole, and the `lwc1`/`sub.s`/`mul.s`/
 * `add.s` sequence on $f0-$f5 at offsets 0x0/0x4/0x8 of each copy is the
 * x/y/z lerp this function writes through `out`.
 */
typedef struct CalcLinerTbl {
    Vector4 *points;
} CalcLinerTbl;

void calcLinerSub(CalcLinerTbl *tbl, Vector4 *out, int idx, float ratio)
{
    Vector4 a, b;

    a = tbl->points[idx];
    b = tbl->points[idx + 1];
    out->x = a.x + (b.x - a.x) * ratio;
    out->y = a.y + (b.y - a.y) * ratio;
    out->z = a.z + (b.z - a.z) * ratio;
}

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcBezierSub);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcSplineSub);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcHermiteSub);

INCLUDE_ASM("asm/nonmatchings/ov01/calc", calcNormal);
