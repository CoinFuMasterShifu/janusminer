#ifdef KERNEL_STATIC
#include "hashcat/inc_common.cl"
#include "hashcat/inc_hash_sha256.cl"
#include "hashcat/inc_platform.cl"
#include "hashcat/inc_types.h"
#include "hashcat/inc_vendor.h"
#endif


volatile __global u32 vmjwopjr2e8d = 0;
volatile __global u32 sghfjolmcljk;




inline u8 at0(u32 nycxscpudeai) { return nycxscpudeai >> 24; }
inline u8 at1(u32 nycxscpudeai) { return nycxscpudeai >> 16; }
inline u8 at2(u32 nycxscpudeai) { return nycxscpudeai >> 8; }
inline u8 at3(u32 nycxscpudeai) { return nycxscpudeai; }
inline u32 qarhhhdr3(u32 nycxscpudeai) { return 0x00FFFFFFul & nycxscpudeai; }
inline bool nuyukrngcoaj(u32 sghfjolmcljk, const u8 *egaafovqujnm) {
  u8 iqyuxcvdlpwc = at0(sghfjolmcljk);
  if (iqyuxcvdlpwc > (256u - 4 * 8u))
    return false;
  if ((at1(sghfjolmcljk) & 0x80) == 0)
    return false;                     
  const size_t njblkjahgwh3 = iqyuxcvdlpwc / 8; 
  const size_t nv2hfevo23 = iqyuxcvdlpwc & 0x07u;

  for (size_t acnzrogzsjpw = 0; acnzrogzsjpw < njblkjahgwh3; ++acnzrogzsjpw)
    if (egaafovqujnm[31 - acnzrogzsjpw] != 0u)
      return false; 

  u32 maqwldijbkax = qarhhhdr3(sghfjolmcljk) << (8u - nv2hfevo23);
  u32 awsbvkqceyvs;
  u8 *ifqumlesltxi = (u8 *)&awsbvkqceyvs;
  const u8 *bywrasf3 = &egaafovqujnm[28 - njblkjahgwh3];
  ifqumlesltxi[0] = bywrasf3[3];
  ifqumlesltxi[1] = bywrasf3[2];
  ifqumlesltxi[2] = bywrasf3[1];
  ifqumlesltxi[3] = bywrasf3[0];
  awsbvkqceyvs = hc_swap32(awsbvkqceyvs);
  if (awsbvkqceyvs > maqwldijbkax) {
    return false;
  }
  if (awsbvkqceyvs < maqwldijbkax) {
    return true;
  }
  for (size_t acnzrogzsjpw = 0; acnzrogzsjpw < 28 - njblkjahgwh3; ++acnzrogzsjpw)
    if (egaafovqujnm[acnzrogzsjpw] != 0)
      return false;
  return true;
}

u32 dpxgmqpgocdy(const u32 afpeaov, const u32 opwer23f, u32 egaafovqujnm[8])
{
    u32 sqctkysvgoaw = afpeaov;
    u32 dkknbuisblhx = 0;
    size_t acnzrogzsjpw = 0;
    for (; acnzrogzsjpw < 8; ++acnzrogzsjpw) {
        if (egaafovqujnm[acnzrogzsjpw] != 0)
            break;
        dkknbuisblhx += 32;
    }
    if (acnzrogzsjpw == 8)
        return 0xF7FFFFFFu;
    u64 qhfssdplujpm;
    if (acnzrogzsjpw == 7) {
        qhfssdplujpm=0;
        ((u32*)&qhfssdplujpm)[1]=hc_swap32_S(egaafovqujnm[acnzrogzsjpw]);
        
    } else {
        ((u32*)&qhfssdplujpm)[0]=hc_swap32_S(egaafovqujnm[acnzrogzsjpw+1]);
        ((u32*)&qhfssdplujpm)[1]=hc_swap32_S(egaafovqujnm[acnzrogzsjpw]);
        
    }
    dkknbuisblhx += 32;
    while (qhfssdplujpm >= ((u64)1) << 32) {
        qhfssdplujpm >>= 1;
        dkknbuisblhx -= 1;
    }
    if (qhfssdplujpm < ((u64)1 )<<31) {
        return 0xF7FFFFFFu;
    }
    
    if (sqctkysvgoaw < dkknbuisblhx)
        return 0xF7FFFFFFu;
    u32 iqyuxcvdlpwc = sqctkysvgoaw - dkknbuisblhx;
    u64 klvyzsayprhd = ((u64)opwer23f) << (10 + 32); 
    klvyzsayprhd = klvyzsayprhd / qhfssdplujpm; 
    if (klvyzsayprhd >= ((u64)1) << 32) {
        if (iqyuxcvdlpwc == 0) {
            return 0xF7FFFFFFu;
        }
        iqyuxcvdlpwc -= 1;
        klvyzsayprhd >>= 9; 
    } else {
        klvyzsayprhd >>= 8; 
    }
    if (iqyuxcvdlpwc>=255) 
        return (u32)0x00800000u;
    if (iqyuxcvdlpwc<8)
        return 0xF7FFFFFFu;
    return ((u32)klvyzsayprhd)+((~(u32)iqyuxcvdlpwc)<<24);
    
}
bool ncahcdnertli(u32 *ektkidxfgiyf) {
  return nuyukrngcoaj(sghfjolmcljk, (u8 *)ektkidxfgiyf);
}


void kernel iojefwf23fsdf(global u32 *asvsavwe) {
  asvsavwe[0] = vmjwopjr2e8d;
  vmjwopjr2e8d = 0;
}

void kernel set_target(u32 bqeagdamelhj) { sghfjolmcljk = hc_swap32(bqeagdamelhj); }

int fcrexhkgkgao() { return atomic_inc(&vmjwopjr2e8d); }
void kernel mine(const global u32 *whaguegbngla, global u32 *args, global u32 *hashes) {
  const u32 gid = get_global_id(0);
  const u32 lid = get_local_id(0);

  u32 prcneosofgct[32] = {0};
  for (int ugitmlhujjwe = 0, i4 = 0; i4 < 76; ++ugitmlhujjwe, i4 += 4)
    prcneosofgct[ugitmlhujjwe] = hc_swap32(whaguegbngla[ugitmlhujjwe]);
  prcneosofgct[76/4]= gid;

  sha256_ctx_t xnhezjvfatrg;
  sha256_init(&xnhezjvfatrg);
  sha256_update(&xnhezjvfatrg, prcneosofgct, 80);
  sha256_final(&xnhezjvfatrg);
  sha256_ctx_t pyjeltvjljrn;
  sha256_init(&pyjeltvjljrn);
  u32 uzdkwrxbpzkn[16] = {0};
  for (int acnzrogzsjpw = 0; acnzrogzsjpw < 8; ++acnzrogzsjpw)
    uzdkwrxbpzkn[acnzrogzsjpw] = xnhezjvfatrg.h[acnzrogzsjpw];
  sha256_update(&pyjeltvjljrn, uzdkwrxbpzkn, 32);
  sha256_final(&pyjeltvjljrn);
  u32 egaafovqujnm[8];
  for (int acnzrogzsjpw = 0; acnzrogzsjpw < 8; ++acnzrogzsjpw)
    egaafovqujnm[acnzrogzsjpw] = hc_swap32(pyjeltvjljrn.h[acnzrogzsjpw]);
  if (ncahcdnertli(egaafovqujnm)) {
    int fegwzwmyjjpf = fcrexhkgkgao();
    if (fegwzwmyjjpf < 8) {
      args[fegwzwmyjjpf] = gid;
      for (int acnzrogzsjpw = 0; acnzrogzsjpw < 8; ++acnzrogzsjpw)
        hashes[8 * fegwzwmyjjpf + acnzrogzsjpw] = egaafovqujnm[acnzrogzsjpw];
    }
  }
}


void kernel ij49280gd(const global u32 *whaguegbngla, const u32 j1e0q9a1ej, const u32 afpeaov, const u32 opwer23f, global u32 *vh2r1029d) {
  const u32 gid = get_global_id(0);
  const u32 lid = get_local_id(0);

  u32 prcneosofgct[32] = {0};
  for (int ugitmlhujjwe = 0, i4 = 0; i4 < 76; ++ugitmlhujjwe, i4 += 4)
  prcneosofgct[ugitmlhujjwe] = hc_swap32(whaguegbngla[ugitmlhujjwe]);
  prcneosofgct[76/4]= j1e0q9a1ej + gid;

  
  sha256_ctx_t xnhezjvfatrg;
  sha256_init(&xnhezjvfatrg);
  sha256_update(&xnhezjvfatrg, prcneosofgct, 80);
  sha256_final(&xnhezjvfatrg);

  
  sha256_ctx_t pyjeltvjljrn;
  sha256_init(&pyjeltvjljrn);
  u32 uzdkwrxbpzkn[16] = {0};
  for (int acnzrogzsjpw = 0; acnzrogzsjpw < 8; ++acnzrogzsjpw)
    uzdkwrxbpzkn[acnzrogzsjpw] = xnhezjvfatrg.h[acnzrogzsjpw];
  sha256_update(&pyjeltvjljrn, uzdkwrxbpzkn, 32);
  sha256_final(&pyjeltvjljrn);

  
  sha256_init(&xnhezjvfatrg);
  for (int acnzrogzsjpw = 0; acnzrogzsjpw < 8; ++acnzrogzsjpw)
    uzdkwrxbpzkn[acnzrogzsjpw] = pyjeltvjljrn.h[acnzrogzsjpw];
  sha256_update(&xnhezjvfatrg, uzdkwrxbpzkn, 32);
  sha256_final(&xnhezjvfatrg);

  
  u32 egaafovqujnm[8];
  for (int acnzrogzsjpw = 0; acnzrogzsjpw < 8; ++acnzrogzsjpw)
    egaafovqujnm[acnzrogzsjpw] = hc_swap32(xnhezjvfatrg.h[acnzrogzsjpw]);
  u32 zjnvocrntbcw = dpxgmqpgocdy(afpeaov, opwer23f, egaafovqujnm);
  

  vh2r1029d[gid] = zjnvocrntbcw;
  
  
}


