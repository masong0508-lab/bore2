/*
 * Dreamcast stand-in for PSn00bSDK's inline_c.h: the GTE (the PS1's geometry coprocessor) worked out in C on the SH-4, with the
 * PS1's own fixed-point rules (RTPS/RTPT with sf=1, lm=0; the projection's divide saturates at 0x1FFFF; screen x/y clamp to
 * -1024..1023, z to 0..65535), so the game's projected corners, depth-table indices and back-face tests come out as on a PS1.
 * Only the macros BORE 2 uses are here.
 */
#pragma once
#include <stdint.h>
#include <string.h>

typedef struct {
    int16_t v[3][3];                 /* V0 V1 V2 */
    int32_t r[3][3];                 /* rotation (4.12) */
    int32_t t[3];                    /* translation */
    int32_t ofx, ofy, h;             /* screen offset (16.16), projection distance */
    int32_t zsf3, zsf4;
    int16_t sx[3], sy[3];            /* SXY0..2 fifo */
    uint16_t sz[4];                  /* SZ0..3 fifo */
    int32_t mac[4], ir[4], otz;
} DcGte;
extern DcGte dc_gte;
void dc_gte_rtps(int i);             /* one vertex V[i] through the pipeline (pushes the fifos) */

#define gte_ldv0(p)       do { const int16_t *s_ = (const int16_t *)(p); dc_gte.v[0][0] = s_[0]; dc_gte.v[0][1] = s_[1]; dc_gte.v[0][2] = s_[2]; } while (0)
#define gte_ldv1(p)       do { const int16_t *s_ = (const int16_t *)(p); dc_gte.v[1][0] = s_[0]; dc_gte.v[1][1] = s_[1]; dc_gte.v[1][2] = s_[2]; } while (0)
#define gte_ldv2(p)       do { const int16_t *s_ = (const int16_t *)(p); dc_gte.v[2][0] = s_[0]; dc_gte.v[2][1] = s_[1]; dc_gte.v[2][2] = s_[2]; } while (0)
#define gte_ldv3(a, b, c) do { gte_ldv0(a); gte_ldv1(b); gte_ldv2(c); } while (0)
#define gte_ldv3c(p)      do { const int16_t *s_ = (const int16_t *)(p); gte_ldv0(s_); gte_ldv1(s_ + 4); gte_ldv2(s_ + 8); } while (0)

#define gte_rtps()  dc_gte_rtps(0)
#define gte_rtpt()  do { dc_gte_rtps(0); dc_gte_rtps(1); dc_gte_rtps(2); } while (0)
#define gte_nclip() (dc_gte.mac[0] = (int32_t)dc_gte.sx[0] * (dc_gte.sy[1] - dc_gte.sy[2]) + (int32_t)dc_gte.sx[1] * (dc_gte.sy[2] - dc_gte.sy[0]) + (int32_t)dc_gte.sx[2] * (dc_gte.sy[0] - dc_gte.sy[1]))
#define gte_avsz3() (dc_gte.mac[0] = dc_gte.zsf3 * ((int32_t)dc_gte.sz[1] + dc_gte.sz[2] + dc_gte.sz[3]), dc_gte.otz = dc_gte.mac[0] >> 12 < 0 ? 0 : dc_gte.mac[0] >> 12 > 0xffff ? 0xffff : dc_gte.mac[0] >> 12)
#define gte_avsz4() (dc_gte.mac[0] = dc_gte.zsf4 * ((int32_t)dc_gte.sz[0] + dc_gte.sz[1] + dc_gte.sz[2] + dc_gte.sz[3]), dc_gte.otz = dc_gte.mac[0] >> 12 < 0 ? 0 : dc_gte.mac[0] >> 12 > 0xffff ? 0xffff : dc_gte.mac[0] >> 12)

static inline void dc_gte_put_xy(void *p, int i) { int16_t xy[2] = { dc_gte.sx[i], dc_gte.sy[i] }; memcpy(p, xy, 4); }
#define gte_stsxy(p)        dc_gte_put_xy((p), 2)
#define gte_stsxy0(p)       dc_gte_put_xy((p), 0)
#define gte_stsxy1(p)       dc_gte_put_xy((p), 1)
#define gte_stsxy2(p)       dc_gte_put_xy((p), 2)
#define gte_stsxy3(a, b, c) do { dc_gte_put_xy((a), 0); dc_gte_put_xy((b), 1); dc_gte_put_xy((c), 2); } while (0)
#define gte_stsz(p)         (*(int32_t *)(p) = dc_gte.sz[3])
#define gte_stsz3(a, b, c)  do { *(int32_t *)(a) = dc_gte.sz[1]; *(int32_t *)(b) = dc_gte.sz[2]; *(int32_t *)(c) = dc_gte.sz[3]; } while (0)
#define gte_stotz(p)        (*(int32_t *)(p) = dc_gte.otz)
#define gte_stopz(p)        (*(int32_t *)(p) = dc_gte.mac[0])
#define gte_stlvnl(p)       do { int32_t *o_ = (int32_t *)(p); o_[0] = dc_gte.mac[1]; o_[1] = dc_gte.mac[2]; o_[2] = dc_gte.mac[3]; } while (0)

#define gte_SetRotMatrix(M_)  do { const MATRIX *m_ = (M_); for (int i_ = 0; i_ < 3; i_++) for (int j_ = 0; j_ < 3; j_++) dc_gte.r[i_][j_] = m_->m[i_][j_]; } while (0)
#define gte_SetTransMatrix(M_) do { const MATRIX *m_ = (M_); dc_gte.t[0] = m_->t[0]; dc_gte.t[1] = m_->t[1]; dc_gte.t[2] = m_->t[2]; } while (0)
#define gte_SetGeomOffset(X_, Y_) (dc_gte.ofx = (int32_t)(X_) << 16, dc_gte.ofy = (int32_t)(Y_) << 16)
#define gte_SetGeomScreen(H_)  (dc_gte.h = (H_))
#define gte_SetBackColor(r, g, b) ((void)0)
