/*
 * Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/).
 * All Rights Reserved.
 *
 * This software is licensed as OpenSource, under the Apache License, Version
 * 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

#include <stdarg.h>

#include "ac.h"

double
FixToDbl(Fixed f)
{
    float r;
    acfixtopflt(f, &r);
    return r;
}

void
ReportAddFlex(void)
{
    if (gHasFlex)
        return;
    gHasFlex = true;
    LogMsg(INFO, OK, "added flex operators to this glyph.");
}

void
ReportLinearCurve(PathElt* e, Fixed x0, Fixed y0, Fixed x1, Fixed y1)
{
    if (gAutoLinearCurveFix) {
        e->type = LINETO;
        e->x = e->x3;
        e->y = e->y3;
        LogMsg(INFO, OK, "Curve from %g %g to %g %g was changed to a line.",
               FixToDbl(x0), FixToDbl(-y0), FixToDbl(x1), FixToDbl(-y1));
    } else {
        LogMsg(INFO, OK,
               "Curve from %g %g to %g %g should be changed to a line.",
               FixToDbl(x0), FixToDbl(-y0), FixToDbl(x1), FixToDbl(-y1));
    }
}

static void
ReportNonHVError(Fixed x0, Fixed y0, Fixed x1, Fixed y1, char* s)
{
    Fixed dx, dy;
    y0 = -y0;
    y1 = -y1;
    dx = x0 - x1;
    dy = y0 - y1;
    if (abs(dx) > FixInt(10) || abs(dy) > FixInt(10) ||
        FTrunc(dx * dx) + FTrunc(dy * dy) > FixInt(100)) {
        LogMsg(INFO, OK, "The line from %g %g to %g %g is not exactly %s.",
               FixToDbl(x0), FixToDbl(y0), FixToDbl(x1), FixToDbl(y1), s);
    }
}

void
ReportNonHError(Fixed x0, Fixed y0, Fixed x1, Fixed y1)
{
    ReportNonHVError(x0, y0, x1, y1, "horizontal");
}

void
ReportNonVError(Fixed x0, Fixed y0, Fixed x1, Fixed y1)
{
    ReportNonHVError(x0, y0, x1, y1, "vertical");
}

void
ExpectedMoveTo(PathElt* e)
{
    char* s;
    switch (e->type) {
        case LINETO:
            s = (char*)"lineto";
            break;
        case CURVETO:
            s = (char*)"curveto";
            break;
        case CLOSEPATH:
            s = (char*)"closepath";
            break;
        default:
            LogMsg(LOGERROR, NONFATALERROR, "Malformed path list.");
            return;
    }
    LogMsg(LOGERROR, NONFATALERROR,
           "Glyph path has a %s where a moveto was expected. "
           "The data is probably truncated.",
           s);
}

void
ReportMissingClosePath(void)
{
    LogMsg(LOGERROR, NONFATALERROR,
           "Missing closepath. The data is probably truncated.");
}

void
ReportTryFlexNearMiss(Fixed x0, Fixed y0, Fixed x2, Fixed y2)
{
    LogMsg(WARNING, OK, "Curves from %g %g to %g %g near miss for adding flex.",
           FixToDbl(x0), FixToDbl(-y0), FixToDbl(x2), FixToDbl(-y2));
}

void
ReportTryFlexError(bool CPflg, Fixed x, Fixed y)
{
    LogMsg(LOGERROR, OK,
           CPflg
             ? "Please move closepath from %g %g so can add flex."
             : "Please remove zero length element at %g %g so can add flex.",
           FixToDbl(x), FixToDbl(-y));
}

void
ReportSplit(PathElt* e)
{
    Fixed x0, y0, x1, y1;
    GetEndPoints(e, &x0, &y0, &x1, &y1);
    LogMsg(INFO, OK,
           "the element that goes from %g %g to %g %g has been split.",
           FixToDbl(x0), FixToDbl(-y0), FixToDbl(x1), FixToDbl(-y1));
}

void
AskForSplit(PathElt* e)
{
    Fixed x0, y0, x1, y1;
    if (e->type == MOVETO)
        e = GetClosedBy(e);
    GetEndPoints(e, &x0, &y0, &x1, &y1);
    LogMsg(LOGERROR, OK,
           "Please split the element that goes from %g %g to %g %g.",
           FixToDbl(x0), FixToDbl(-y0), FixToDbl(x1), FixToDbl(-y1));
}

void
ReportPossibleLoop(PathElt* e)
{
    Fixed x0, y0, x1, y1;
    if (e->type == MOVETO)
        e = GetClosedBy(e);
    GetEndPoints(e, &x0, &y0, &x1, &y1);
    LogMsg(LOGERROR, OK,
           "Possible loop in element that goes from %g %g to %g %g."
           " Please check.",
           FixToDbl(x0), FixToDbl(-y0), FixToDbl(x1), FixToDbl(-y1));
}

void
ReportConflictCheck(PathElt* e, PathElt* conflict, PathElt* cp)
{
    Fixed ex, ey, cx, cy, cpx, cpy;
    GetEndPoint(e, &ex, &ey);
    GetEndPoint(conflict, &cx, &cy);
    GetEndPoint(cp, &cpx, &cpy);
    LogMsg(INFO, OK, "Check e %g %g conflict %g %g cp %g %g.", FixToDbl(ex),
           FixToDbl(-ey), FixToDbl(cx), FixToDbl(-cy), FixToDbl(cpx),
           FixToDbl(-cpy));
}

void
ReportConflictCnt(PathElt* e, int32_t cnt)
{
    Fixed ex, ey;
    GetEndPoint(e, &ex, &ey);
    LogMsg(INFO, OK, "%g %g conflict count = %d", FixToDbl(ex), FixToDbl(-ey),
           cnt);
}

void
ReportRemFlare(PathElt* e, PathElt* e2, bool hFlg, int32_t i)
{
    Fixed ex1, ey1, ex2, ey2;
    GetEndPoint(e, &ex1, &ey1);
    GetEndPoint(e2, &ex2, &ey2);
    LogMsg(INFO, OK, "Removed %s flare at %g %g by %g %g : %d.",
           hFlg ? "horizontal" : "vertical", FixToDbl(ex1), FixToDbl(-ey1),
           FixToDbl(ex2), FixToDbl(-ey2), i);
}

void
ReportRemConflict(PathElt* e)
{
    Fixed ex, ey;
    GetEndPoint(e, &ex, &ey);
    LogMsg(INFO, OK, "Removed conflicting hints at %g %g.", FixToDbl(ex),
           FixToDbl(-ey));
}

void
ReportRotateSubpath(PathElt* e)
{
    Fixed ex, ey;
    GetEndPoint(e, &ex, &ey);
    LogMsg(INFO, OK, "changed closepath to %g %g.", FixToDbl(ex),
           FixToDbl(-ey));
}

void
ReportRemShortHints(Fixed ex, Fixed ey)
{
    LogMsg(INFO, OK, "Removed hints from short element at %g %g.", FixToDbl(ex),
           FixToDbl(-ey));
}

static void
PrintDebugVal(Fixed v)
{
    if (v >= FixInt(100000))
        LogMsg(LOGDEBUG, OK, "%d", FTrunc(v));
    else
        LogMsg(LOGDEBUG, OK, "%g", FixToDbl(v));
}

static void
ShwHV(HintVal* val)
{
    Fixed bot, top;
    bot = -val->vLoc1;
    top = -val->vLoc2;
    LogMsg(LOGDEBUG, OK, "b %g t %g v ", FixToDbl(bot), FixToDbl(top));
    PrintDebugVal(val->vVal);
    LogMsg(LOGDEBUG, OK, " s %g", FixToDbl(val->vSpc));
    if (val->vGhst)
        LogMsg(LOGDEBUG, OK, " G");
}

void
ShowHVal(HintVal* val)
{
    Fixed l, r;
    HintSeg* seg;
    ShwHV(val);
    seg = val->vSeg1;
    if (seg == NULL)
        return;
    l = seg->sMin;
    r = seg->sMax;
    LogMsg(LOGDEBUG, OK, " l1 %g r1 %g ", FixToDbl(l), FixToDbl(r));
    seg = val->vSeg2;
    l = seg->sMin;
    r = seg->sMax;
    LogMsg(LOGDEBUG, OK, " l2 %g r2 %g", FixToDbl(l), FixToDbl(r));
}

void
ShowHVals(HintVal* lst)
{
    while (lst != NULL) {
        ShowHVal(lst);
        lst = lst->vNxt;
    }
}

void
ReportAddHVal(HintVal* val)
{
    ShowHVal(val);
}

static void
ShwVV(HintVal* val)
{
    Fixed lft, rht;
    lft = val->vLoc1;
    rht = val->vLoc2;
    LogMsg(LOGDEBUG, OK, "l %g r %g v ", FixToDbl(lft), FixToDbl(rht));
    PrintDebugVal(val->vVal);
    LogMsg(LOGDEBUG, OK, " s %g", FixToDbl(val->vSpc));
}

void
ShowVVal(HintVal* val)
{
    Fixed b, t;
    HintSeg* seg;
    ShwVV(val);
    seg = val->vSeg1;
    if (seg == NULL)
        return;
    b = -seg->sMin;
    t = -seg->sMax;
    LogMsg(LOGDEBUG, OK, " b1 %g t1 %g ", FixToDbl(b), FixToDbl(t));
    seg = val->vSeg2;
    b = -seg->sMin;
    t = -seg->sMax;
    LogMsg(LOGDEBUG, OK, " b2 %g t2 %g", FixToDbl(b), FixToDbl(t));
}

void
ShowVVals(HintVal* lst)
{
    while (lst != NULL) {
        ShowVVal(lst);
        lst = lst->vNxt;
    }
}

void
ReportAddVVal(HintVal* val)
{
    ShowVVal(val);
}

void
ReportFndBstVal(HintSeg* seg, HintVal* val, bool hFlg)
{
    if (hFlg) {
        LogMsg(LOGDEBUG, OK, "FndBstVal: sLoc %g sLft %g sRght %g ",
               FixToDbl(-seg->sLoc), FixToDbl(seg->sMin), FixToDbl(seg->sMax));
        if (val)
            ShwHV(val);
        else
            LogMsg(LOGDEBUG, OK, "NULL");
    } else {
        LogMsg(LOGDEBUG, OK, "FndBstVal: sLoc %g sBot %g sTop %g ",
               FixToDbl(seg->sLoc), FixToDbl(-seg->sMin), FixToDbl(-seg->sMax));
        if (val)
            ShwVV(val);
        else
            LogMsg(LOGDEBUG, OK, "NULL");
    }
}

void
ReportCarry(Fixed l0, Fixed l1, Fixed loc, HintVal* hints, bool vert)
{
    if (vert) {
        ShowVVal(hints);
    } else {
        ShowHVal(hints);
        loc = -loc;
        l0 = -l0;
        l1 = -l1;
    }
    LogMsg(LOGDEBUG, OK, " carry to %g in [%g..%g]", FixToDbl(loc),
           FixToDbl(l0), FixToDbl(l1));
}

void
ReportBestCP(PathElt* e, PathElt* cp)
{
    Fixed ex, ey, px, py;
    GetEndPoint(e, &ex, &ey);
    if (cp != NULL) {
        GetEndPoint(cp, &px, &py);
        LogMsg(INFO, OK, "%g %g best cp at %g %g", FixToDbl(ex), FixToDbl(-ey),
               FixToDbl(px), FixToDbl(-py));
    } else {
        LogMsg(INFO, OK, "%g %g no best cp", FixToDbl(ex), FixToDbl(-ey));
    }
}

void
LogHintInfo(HintPoint* pl)
{
    char c = pl->c;
    if (c == 'y' || c == 'm') { /* vertical lines */
        Fixed lft = pl->x0;
        Fixed rht = pl->x1;
        LogMsg(LOGDEBUG, OK, "%4g  %-30s%5g%5g", FixToDbl(rht - lft),
               gGlyphName, FixToDbl(lft), FixToDbl(rht));
    } else {
        Fixed bot = pl->y0;
        Fixed top = pl->y1;
        Fixed wdth = top - bot;
        if (wdth == -FixInt(21) || wdth == -FixInt(20))
            return; /* ghost pair */
        LogMsg(LOGDEBUG, OK, "%4g  %-30s%5g%5g", FixToDbl(wdth), gGlyphName,
               FixToDbl(bot), FixToDbl(top));
    }
}

static void
LstHVal(HintVal* val)
{
    LogMsg(LOGDEBUG, OK, "\t");
    ShowHVal(val);
    LogMsg(LOGDEBUG, OK, " ");
}

static void
LstVVal(HintVal* val)
{
    LogMsg(LOGDEBUG, OK, "\t");
    ShowVVal(val);
    LogMsg(LOGDEBUG, OK, " ");
}

void
ListHintInfo(void)
{ /* debugging routine */
    PathElt* e;
    SegLnkLst *hLst, *vLst;
    HintSeg* seg;
    Fixed x, y;
    e = gPathStart;
    while (e != NULL) {
        hLst = e->Hs;
        vLst = e->Vs;
        if ((hLst != NULL) || (vLst != NULL)) {
            GetEndPoint(e, &x, &y);
            y = -y;
            LogMsg(LOGDEBUG, OK, "x %g y %g ", FixToDbl(x), FixToDbl(y));
            while (hLst != NULL) {
                seg = hLst->lnk->seg;
                LstHVal(seg->sLnk);
                hLst = hLst->next;
            }
            while (vLst != NULL) {
                seg = vLst->lnk->seg;
                LstVVal(seg->sLnk);
                vLst = vLst->next;
            }
        }
        e = e->next;
    }
}

void
ReportStemNearMiss(bool vert, Fixed w, Fixed minW, Fixed b, Fixed t, bool curve)
{
    LogMsg(INFO, OK, "%s %s stem near miss: %g instead of %g at %g to %g.",
           vert ? "Vertical" : "Horizontal", curve ? "curve" : "linear",
           FixToDbl(w), FixToDbl(minW), FixToDbl(NUMMIN(b, t)),
           FixToDbl(NUMMAX(b, t)));
}

void
ReportHintConflict(Fixed x0, Fixed y0, Fixed x1, Fixed y1, char ch)
{
    char s[2];
    s[0] = ch;
    s[1] = 0;
    LogMsg(LOGERROR, OK, "  Conflicts with current hints: %g %g %g %g %s.",
           FixToDbl(x0), FixToDbl(y0), FixToDbl(x1), FixToDbl(y1), s);
}

void
ReportDuplicates(Fixed x, Fixed y)
{
    LogMsg(LOGERROR, OK, "Check for duplicate subpath at %g %g.", FixToDbl(x),
           FixToDbl(y));
}

void
ReportBBoxBogus(Fixed llx, Fixed lly, Fixed urx, Fixed ury)
{
    LogMsg(LOGERROR, OK, "Glyph bounding box looks bogus: %g %g %g %g.",
           FixToDbl(llx), FixToDbl(lly), FixToDbl(urx), FixToDbl(ury));
}

void
ReportMergeHVal(Fixed b0, Fixed t0, Fixed b1, Fixed t1, Fixed v0, Fixed s0,
                Fixed v1, Fixed s1)
{
    LogMsg(LOGDEBUG, OK,

           "Replace H hints pair at %g %g by %g %g\n\told value ",
           FixToDbl(-b0), FixToDbl(-t0), FixToDbl(-b1), FixToDbl(-t1));
    PrintDebugVal(v0);
    LogMsg(LOGDEBUG, OK, " %g new value ", FixToDbl(s0));
    PrintDebugVal(v1);
    LogMsg(LOGDEBUG, OK, " %g", FixToDbl(s1));
}

void
ReportMergeVVal(Fixed l0, Fixed r0, Fixed l1, Fixed r1, Fixed v0, Fixed s0,
                Fixed v1, Fixed s1)
{
    LogMsg(LOGDEBUG, OK, "Replace V hints pair at %g %g by %g %g\n\told value ",
           FixToDbl(l0), FixToDbl(r0), FixToDbl(l1), FixToDbl(r1));
    PrintDebugVal(v0);
    LogMsg(LOGDEBUG, OK, " %g new value ", FixToDbl(s0));
    PrintDebugVal(v1);
    LogMsg(LOGDEBUG, OK, " %g", FixToDbl(s1));
}

void
ReportPruneHVal(HintVal* val, HintVal* v, int32_t i)
{
    LogMsg(LOGDEBUG, OK, "PruneHVal: %d\n\t", i);
    ShowHVal(val);
    LogMsg(LOGDEBUG, OK, "\n\t");
    ShowHVal(v);
}

void
ReportPruneVVal(HintVal* val, HintVal* v, int32_t i)
{
    LogMsg(LOGDEBUG, OK, "PruneVVal: %d\n\t", i);
    ShowVVal(val);
    LogMsg(LOGDEBUG, OK, "\n\t");
    ShowVVal(v);
}
