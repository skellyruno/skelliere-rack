#include "core/VoiceMatch.h"
#include <algorithm>
#include <complex>
#include <cmath>
#include <cstring>
#ifdef skelly_LAB190IQ
 #include <cstdio>
 #include <cstdlib>
#endif
#include <iterator>

namespace skelly::vmatch
{
namespace
{
constexpr double pi = 3.141592653589793;
constexpr double refRate = 48000.0; // the rate the calibration was measured at

inline double dbToGain (double db) noexcept { return std::pow (10.0, db / 20.0); }
inline double gainToDb (double g) noexcept { return 20.0 * std::log10 (std::max (g, 1e-12)); }
inline double coefMs (double ms, double sr) noexcept { return ms <= 0.0 ? 1.0 : 1.0 - std::exp (-1000.0 / (ms * sr)); }
inline double roundTo (double v, double step) noexcept { return std::round (v / step) * step; }

// piecewise-linear lookup in a sorted table
double interp (const float* xs, const float* ys, std::size_t N, double x) noexcept
{
    if (x <= xs[0])
        return ys[0];
    if (x >= xs[N - 1])
        return ys[N - 1];
    const auto* it = std::upper_bound (xs, xs + N, (float) x);
    const auto i = (std::size_t) (it - xs) - 1;
    const auto t = (x - xs[i]) / (xs[i + 1] - xs[i]);
    return ys[i] + t * (ys[i + 1] - ys[i]);
}

// bilinear transform of an analog biquad (n2 s^2 + n1 s + n0) / (s^2 + d1 s + d0) with s = K (1 - z^-1) / (1 + z^-1)
void bilinear (const double n[3], const double d[2], double K, Biquad& b) noexcept
{
    const double K2 = K * K;
    const double b0 = n[0] * K2 + n[1] * K + n[2], b1 = 2.0 * (n[2] - n[0] * K2), b2 = n[0] * K2 - n[1] * K + n[2];
    const double a0 = K2 + d[0] * K + d[1], a1 = 2.0 * (d[1] - K2), a2 = K2 - d[0] * K + d[1];
    b.set (b0, b1, b2, a0, a1, a2);
}

double biquadMagnitude (const Biquad& b, double w) noexcept
{
    const double c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
    const double nr = b.b0 + b.b1 * c1 + b.b2 * c2, ni = -(b.b1 * s1 + b.b2 * s2);
    const double dr = 1.0 + b.a1 * c1 + b.a2 * c2, di = -(b.a1 * s1 + b.a2 * s2);
    return std::sqrt ((nr * nr + ni * ni) / std::max (dr * dr + di * di, 1e-300));
}
} // namespace

namespace echannel
{
namespace
{
#include "core/VoiceMatchTables.inc"

constexpr double gainNodes[21] = { -15, -13.5, -12, -10.5, -9, -7.5, -6, -4.5, -3, -1.5, 0, 1.5, 3, 4.5, 6, 7.5, 9, 10.5, 12, 13.5, 15 };

// Catmull-Rom over the gain nodes of one segment ([-15,-7.5], [-7.5,0], [0,7.5], [7.5,15]: the band's law changes
// slope at the segment ends, measured), `at(i)` = the feature vector of gain node i
template <typename At>
void gainInterp (double g, At at, double out[5]) noexcept
{
    g = std::clamp (g, -15.0, 15.0);
    const int seg = g < -7.5 ? 0 : (g < 0.0 ? 1 : (g < 7.5 ? 2 : 3));
    const int lo = seg * 5, hi = lo + 5; // node indexes of the segment ends
    int i = std::clamp ((int) std::floor ((g + 15.0) / 1.5), lo, hi - 1);
    const double t = (g - gainNodes[i]) / 1.5;
    double p0[5], p1[5], p2[5], p3[5];
    at (i, p1);
    at (i + 1, p2);
    if (i > lo) at (i - 1, p0); else for (int k = 0; k < 5; ++k) p0[k] = 2 * p1[k] - p2[k];
    if (i + 2 <= hi) at (i + 2, p3); else for (int k = 0; k < 5; ++k) p3[k] = 2 * p2[k] - p1[k];
    const double t2 = t * t, t3 = t2 * t;
    for (int k = 0; k < 5; ++k)
    {
        const double m1 = 0.5 * (p2[k] - p0[k]), m2 = 0.5 * (p3[k] - p1[k]);
        out[k] = (2 * t3 - 3 * t2 + 1) * p1[k] + (t3 - 2 * t2 + t) * m1 + (-2 * t3 + 3 * t2) * p2[k] + (t3 - t2) * m2;
    }
}

struct Axis { int i; double t; };
Axis axis (const float* nodes, std::size_t N, double x) noexcept
{
    int i = 0;
    while (i < (int) N - 2 && x > nodes[i + 1])
        ++i;
    const double t = std::clamp ((x - nodes[i]) / (nodes[i + 1] - nodes[i]), 0.0, 1.0);
    return { i, t };
}

// the analog-domain biquad (normalised features x reference frequency) of one band, and its reference frequency
bool bandAnalog (int band, const Settings& s, double n[3], double d[2], double& wref) noexcept
{
    double g = 0.0, f = 0.0, q = 0.0;
    bool bell = false;
    switch (band)
    {
        case 0: g = s.lfGain; f = roundTo (std::clamp (s.lfHz, 30.0, 450.0), 1.0); bell = s.lfBell; break;
        case 1: g = s.lmfGain; f = roundTo (std::clamp (s.lmfKHz, 0.2, 2.5), 0.01) * 1000.0; q = s.lmfQ; break;
        case 2: g = s.hmfGain; f = roundTo (std::clamp (s.hmfKHz, 0.6, 7.0), 0.01) * 1000.0; q = s.hmfQ; break;
        default: g = s.hfGain; f = roundTo (std::clamp (s.hfKHz, 1.5, 16.0), 0.01) * 1000.0; bell = s.hfBell; break;
    }
    g = roundTo (g, 0.1);
    if (std::abs (g) < 0.05)
        return false;
    double u[5];
    if (band == 0 || band == 3)
    {
        const bool lo = band == 0;
        const float* feat = lo ? lfFeat : hfFeat;
        const double lw = lo ? interp (lfLawText, lfLawLogW, std::size (lfLawText), f) : interp (hfLawText, hfLawLogW, std::size (hfLawText), f);
        wref = std::exp (lw);
        const auto ax = lo ? axis (lfNodeLogW, std::size (lfNodeLogW), lw) : axis (hfNodeLogW, std::size (hfNodeLogW), lw);
        const int nf = (int) (lo ? std::size (lfNodeLogW) : std::size (hfNodeLogW)), type = bell ? 1 : 0;
        gainInterp (g, [&] (int gi, double* o) {
            const float* a = feat + ((type * 21 + gi) * nf + ax.i) * 5;
            for (int k = 0; k < 5; ++k)
                o[k] = (1 - ax.t) * a[k] + ax.t * a[5 + k];
        }, u);
    }
    else
    {
        const bool lo = band == 1;
        const float* feat = lo ? lmfFeat : hmfFeat;
        const double lw = lo ? interp (lmfLawText, lmfLawLogW, std::size (lmfLawText), f) : interp (hmfLawText, hmfLawLogW, std::size (hmfLawText), f);
        wref = std::exp (lw);
        const auto ax = lo ? axis (lmfNodeLogW, std::size (lmfNodeLogW), lw) : axis (hmfNodeLogW, std::size (hmfNodeLogW), lw);
        const auto aq = lo ? axis (lmfQv, std::size (lmfQv), dialFromQ (roundTo (q, 0.01))) : axis (hmfQv, std::size (hmfQv), dialFromQ (roundTo (q, 0.01)));
        const int nf = (int) (lo ? std::size (lmfNodeLogW) : std::size (hmfNodeLogW)), nq = (int) std::size (lmfQv);
        gainInterp (g, [&] (int gi, double* o) {
            for (int k = 0; k < 5; ++k)
                o[k] = 0.0;
            for (int df = 0; df < 2; ++df)
                for (int dq = 0; dq < 2; ++dq)
                {
                    const double w = (df ? ax.t : 1 - ax.t) * (dq ? aq.t : 1 - aq.t);
                    const float* a = feat + ((gi * nf + ax.i + df) * nq + aq.i + dq) * 5;
                    for (int k = 0; k < 5; ++k)
                        o[k] += w * a[k];
                }
        }, u);
    }
    n[0] = u[0];
    n[1] = u[1] * wref;
    n[2] = u[2] * wref * wref;
    d[0] = u[3] * wref;
    d[1] = u[4] * wref * wref;
    return true;
}

// At other sample rates the 48 kHz analog-domain response is kept at the band's own frequency: the prototype is
// scaled so that bilinear(K = 2 sr) maps that frequency where bilinear(K = 96000) mapped it at 48 kHz [model]
double rateScale (double wref, double sr) noexcept
{
    if (std::abs (sr - refRate) < 1.0)
        return 1.0;
    const double f0 = refRate / pi * std::atan (wref / (2.0 * refRate)); // the band's frequency in Hz (48 kHz design)
    const double fs = std::min (f0, 0.45 * sr);
    return (2.0 * sr * std::tan (pi * fs / sr)) / (2.0 * refRate * std::tan (pi * fs / refRate)) * (fs / f0);
}

} // namespace
double qFromDial (double v) noexcept
{
    v = std::clamp (v, 0.0, 1.0) * 20.0;
    const int i = std::min ((int) v, 19);
    return qText[i] + (v - i) * (qText[i + 1] - qText[i]);
}

double dialFromQ (double q) noexcept
{
    q = std::clamp (q, (double) qText[20], (double) qText[0]);
    for (int i = 0; i < 20; ++i)
        if (q <= qText[i] && q >= qText[i + 1])
            return (i + (qText[i] - q) / (qText[i] - qText[i + 1])) / 20.0;
    return 1.0;
}

// ---- round 190mt: the G-Channel EQ law (Settings::gLaw; REA_rodada190mt §1) ----
// [EXEC] Identified black-box on our own demo-mode impulse responses of the reference (every band alone, two takes per
// probe: shelves at 15 / 14 frequencies × every dB, bells at 11 / 12 frequencies × 20 gains × 10 Qs, the ÷3 / ×3 switches
// on a coarser grid): each band is ONE second-order section (residual −100 … −150 dB) whose poles and zeros move with
// the gain — the bells are peaking sections whose pole / zero Qs follow the gain (proportional Q) and whose centre
// moves with the gain and Q; the LF shelf is (s² + βs + Wz²) / (s² + βs + Wp²) with a shared β of a few kHz (a first-
// order slope below it), the HF shelf its mirror. skelly keeps each probe's analog prototype (fitted, inverse bilinear at
// 48 kHz) and interpolates ln of its coefficients over frequency (log), gain and Q; other rates keep the band's own
// frequency (rateScale). The numbers are our fits, not vendor data.
namespace gch
{
#include "core/SslGTables.inc"
struct Ax { int i; double t; };
Ax axisOf (const double* xs, int n, double x, bool logx) noexcept
{
    x = std::clamp (x, xs[0], xs[n - 1]);
    int i = 0;
    while (i < n - 2 && x > xs[i + 1])
        ++i;
    const double t = logx ? std::log (x / xs[i]) / std::log (xs[i + 1] / xs[i]) : (x - xs[i]) / (xs[i + 1] - xs[i]);
    return { i, std::clamp (t, 0.0, 1.0) };
}
// a table [frequency][gain][Q][5] (nq = 1: shelves) at (af, ag, aq): multilinear in ln of the coefficients
void lookup (const float* tab, int ng, int nq, Ax af, Ax ag, Ax aq, double u[5]) noexcept
{
    for (int k = 0; k < 5; ++k)
        u[k] = 0.0;
    for (int df = 0; df < 2; ++df)
        for (int dg = 0; dg < 2; ++dg)
            for (int dq = 0; dq < (nq > 1 ? 2 : 1); ++dq)
            {
                const double w = (df ? af.t : 1.0 - af.t) * (dg ? ag.t : 1.0 - ag.t) * (nq > 1 ? (dq ? aq.t : 1.0 - aq.t) : 1.0);
                const float* c = tab + (((af.i + df) * ng + ag.i + dg) * nq + (nq > 1 ? aq.i + dq : 0)) * 5;
                for (int k = 0; k < 5; ++k)
                    u[k] += w * c[k];
            }
    for (int k = 0; k < 5; ++k)
        u[k] = std::exp (u[k]);
}
bool band (int b, const Settings& s, double sampleRate, Biquad& out) noexcept
{
    constexpr int nShelfDb = (int) std::size (gShelfDb), nBellDb = (int) std::size (gBellDb), nBellQ = (int) std::size (gBellQ);
    constexpr int nSubDb = (int) std::size (gSubDb), nSubQ = (int) std::size (gSubQ);
    double u[5];
    const Ax none { 0, 0.0 };
    if (b == 0 || b == 3)
    {
        const double g = std::clamp (b == 0 ? s.lfGain : s.hfGain, -17.0, 17.0);
        if (std::abs (g) < 1e-6)
            return false;
        const auto af = b == 0 ? axisOf (gLfHz, (int) std::size (gLfHz), s.lfHz, true) : axisOf (gHfKHz, (int) std::size (gHfKHz), s.hfKHz, true);
        lookup (b == 0 ? gLf : gHf, nShelfDb, 1, af, axisOf (gShelfDb, nShelfDb, g, false), none, u);
    }
    else
    {
        const bool lo = b == 1, sub = lo ? s.lmfDiv3 : s.hmfMult3;
        const double g = std::clamp (lo ? s.lmfGain : s.hmfGain, -20.0, 20.0), q = lo ? s.lmfQ : s.hmfQ;
        if (std::abs (g) < 1e-6)
            return false;
        const auto af = lo ? axisOf (gLmfKHz, (int) std::size (gLmfKHz), s.lmfKHz, true) : axisOf (gHmfKHz, (int) std::size (gHmfKHz), s.hmfKHz, true);
        if (sub)
            lookup (lo ? gLmfDiv3 : gHmfX3, nSubDb, nSubQ, af, axisOf (gSubDb, nSubDb, g, false), axisOf (gSubQ, nSubQ, q, false), u);
        else
            lookup (lo ? gLmf : gHmf, nBellDb, nBellQ, af, axisOf (gBellDb, nBellDb, g, false), axisOf (gBellQ, nBellQ, q, false), u);
    }
    constexpr double U = 2.0 * pi * 1000.0;
    double n[3] = { u[0], u[1] * U, u[2] * U * U }, d[2] = { u[3] * U, u[4] * U * U };
    const double a = rateScale (std::sqrt (d[1]), sampleRate);
    n[1] *= a; n[2] *= a * a; d[0] *= a; d[1] *= a * a;
    bilinear (n, d, 2.0 * sampleRate, out);
    return true;
}
} // namespace gch

bool bandBiquad (int band, const Settings& s, double sampleRate, Biquad& out)
{
    if (s.gLaw == 1) // round 190mt: the G-Channel EQ law
        return gch::band (band, s, sampleRate, out);
    double n[3], d[2], wref = 0.0;
    if (! bandAnalog (band, s, n, d, wref))
        return false;
    const double a = rateScale (wref, sampleRate);
    n[1] *= a; n[2] *= a * a; d[0] *= a; d[1] *= a * a;
    bilinear (n, d, 2.0 * sampleRate, out);
    return true;
}

bool highPass (const Settings& s, double sampleRate, Biquad& first, Biquad& second)
{
    if (s.hpHz < 16.5)
        return false;
    // 3rd-order Butterworth in the analog domain (a real pole and a Q = 1 pair at the same frequency), measured
    double w = interp (hpText, hpW, std::size (hpText), roundTo (std::clamp (s.hpHz, 17.0, 350.0), 1.0));
    w *= rateScale (w, sampleRate);
    const double K = 2.0 * sampleRate;
    // first order s / (s + w)
    first.set (K, -K, 0.0, K + w, w - K, 0.0);
    const double n[3] = { 1.0, 0.0, 0.0 }, dd[2] = { w, w * w };
    bilinear (n, dd, K, second);
    return true;
}

bool lowPass (const Settings& s, double sampleRate, Biquad& out)
{
    if (s.lpKHz <= 0.0)
        return false;
    const double f = roundTo (std::clamp (s.lpKHz, 3.0, 19.5), 0.1) * 1000.0;
    // the table holds (n2, n1, n0, d1, d0) per displayed frequency
    constexpr auto count = std::size (lpText);
    std::size_t i = 0;
    while (i < count - 2 && f > lpText[i + 1])
        ++i;
    const double t = std::clamp ((f - lpText[i]) / (lpText[i + 1] - lpText[i]), 0.0, 1.0);
    double c[5];
    for (int k = 0; k < 5; ++k)
        c[k] = (1 - t) * lpCoef[i * 5 + k] + t * lpCoef[(i + 1) * 5 + k];
    double n[3] = { c[0], c[1], c[2] }, d[2] = { c[3], c[4] };
    const double a = rateScale (std::sqrt (std::max (d[1], 1.0)), sampleRate);
    n[1] *= a; n[2] *= a * a; d[0] *= a; d[1] *= a * a;
    bilinear (n, d, 2.0 * sampleRate, out);
    return true;
}

double responseDb (const Settings& s, double sampleRate, double hz)
{
    const double w = 2.0 * pi * hz / sampleRate;
    double m = 1.0;
    Biquad a, b;
    if (highPass (s, sampleRate, a, b))
        m *= biquadMagnitude (a, w) * biquadMagnitude (b, w);
    if (lowPass (s, sampleRate, a))
        m *= biquadMagnitude (a, w);
    if (s.eqOn)
        for (int band = 0; band < 4; ++band)
            if (bandBiquad (band, s, sampleRate, a))
                m *= biquadMagnitude (a, w);
    return gainToDb (m);
}

// ---- dynamics ----
namespace
{
#include "core/VoiceMatchDynamics.inc"

double softplus (double o, double k) noexcept { return o > 30.0 * k ? o : k * std::log1p (std::exp (o / k)); }

// ---- round 148: the "4K" console's dynamics, in the same feedback model ([EXEC] SSL 4K E trial; REA_rodada148 §2)
#include "core/Console4KDynamics.inc"

double rowInterp (const double* xs, int n, const double* rows, int stride, int k, double x) noexcept
{
    if (x <= xs[0]) return rows[k];
    for (int i = 0; i < n - 1; ++i)
        if (x <= xs[i + 1])
        {
            const double t = (x - xs[i]) / (xs[i + 1] - xs[i]);
            return rows[i * stride + k] + t * (rows[(i + 1) * stride + k] - rows[i * stride + k]);
        }
    return rows[(n - 1) * stride + k];
}

CompLaw k4CompLaw (const Settings& s) noexcept
{
    CompLaw c {};
    const double r = std::max (1.0, s.compRatio);
    const double u = r > 1e6 ? 1.0 : 1.0 - 1.0 / r;
    const double thr = std::clamp (s.compThreshold, -20.0, 10.0);
    constexpr int nT = (int) std::size (k4Thr), nU = (int) std::size (k4U);
    int ti = 0;
    while (ti < nT - 2 && thr < k4Thr[ti + 1]) ++ti;
    const double tt = std::clamp ((k4Thr[ti] - thr) / (k4Thr[ti] - k4Thr[ti + 1]), 0.0, 1.0);
    const double uu = std::max (u, k4U[0]);
    int ui = 0;
    while (ui < nU - 2 && uu > k4U[ui + 1]) ++ui;
    const double tu = std::clamp ((uu - k4U[ui]) / (k4U[ui + 1] - k4U[ui]), 0.0, 1.0);
    auto bil = [&] (const double* g, int stride, int k) {
        auto at = [&] (int a, int b) { return g[(a * nU + b) * stride + k]; };
        return (1 - tt) * ((1 - tu) * at (ti, ui) + tu * at (ti, ui + 1)) + tt * ((1 - tu) * at (ti + 1, ui) + tu * at (ti + 1, ui + 1));
    };
    // below the lowest measured ratio (1.8) the slope and make-up fade to 0 at 1:1
    const double fade = u < k4U[0] ? u / k4U[0] : 1.0;
    c.thresholdOffset = bil (k4Static, 3, 0);
    c.knee = std::max (0.2, bil (k4Static, 3, 1));
    // round tp172c4: the slope blends geometrically (log slope), not linearly: the infinity nodes' slopes are ~100 x the
    // 9.3:1 ones, so a plain blend made a 10:1 dial act like ~100:1 (the 4K E trial's S4: 6 dB too deep)
    {
        auto w = [&] (int a, int b) { return std::log (std::max (1e-6, k4Static[(a * nU + b) * 3 + 2])); };
        c.slope = std::exp ((1 - tt) * ((1 - tu) * w (ti, ui) + tu * w (ti, ui + 1)) + tt * ((1 - tu) * w (ti + 1, ui) + tu * w (ti + 1, ui + 1))) * fade;
    }
    // the automatic make-up (measured at -60 dBFS) and its offset: [EXEC] +-6 dB of offset moves the make-up by
    // 1.15 / 1.37 / 1.88 dB at 2.9:1 / 5:1 / inf:1 (never below 0)
    double mk = bil (k4Makeup, 1, 0) * fade;
    const double per = u <= 0.655 ? 0.192 * u / 0.655 : (u <= 0.8 ? 0.192 + (u - 0.655) / 0.145 * 0.036 : 0.228 + (u - 0.8) / 0.2 * 0.085);
    mk = std::max (0.0, mk + per * std::clamp (s.makeupOffsetDb, -12.0, 12.0));
    c.makeup = s.autoMakeup ? mk : 0.0;
    // attack: fast = instant (the fit's floor, 0.05 ms); otherwise the console's own time, longer for deeper settings
    c.attackFastMs = 0.05;
    c.attackSlowMs = std::exp (k4AttackLaw[0] + k4AttackLaw[1] * u + k4AttackLaw[2] * thr);
    const double rel = std::clamp (s.compRelease, 0.1, 4.0);
    const double* knobs = s.compFast ? k4RelKnobFast : k4RelKnobSlow;
    const double* rows = s.compFast ? k4RelFast : k4RelSlow;
    const int nk = s.compFast ? (int) std::size (k4RelKnobFast) : (int) std::size (k4RelKnobSlow);
    c.release1Fraction = rowInterp (knobs, nk, rows, 3, 0, rel);
    c.release1Ms = rowInterp (knobs, nk, rows, 3, 1, rel);
    c.releaseMs = rowInterp (knobs, nk, rows, 3, 2, rel);
    c.detectorAttackMs = 0.021;
    c.detectorReleaseMs = 0.064;
    return c;
}

// [EXEC] the 4K's expander / gate on 1 kHz stairs and bursts: threshold about 23.3 dB under the console's scale, the
// expander 1.13 dB of reduction per dB below it, the range dial's depth 5 / 10 / 20 / 40 -> 7.8 / 17.1 / 26.7 / 47.3 dB,
// opening within half a millisecond, closing over 117 ms x (release / 0.1 s)^0.58 (gate) and 21 ms x (release / 0.1 s)^0.6
// (expander); the gate's hysteresis was not measured (round 123's 3.9 dB kept) [model]
GateLaw k4GateLaw (const Settings& s) noexcept
{
    GateLaw g {};
    static constexpr double dial[5] { 0.0, 5.0, 10.0, 20.0, 40.0 }, depth[5] { 0.0, 7.8, 17.1, 26.7, 47.3 };
    g.range = knobInterp (dial, depth, 5, std::clamp (s.expRange, 0.0, 40.0));
    g.threshold = std::clamp (s.expThreshold, -30.0, 10.0) - 23.3;
    g.attackMs = s.expFast ? 0.05 : 0.3;
    const double rel = std::clamp (s.expRelease, 0.1, 4.0) / 0.1;
    g.releaseMs = s.gate ? 117.0 * std::pow (rel, 0.58) : 21.0 * std::pow (rel, 0.6);
    g.detectorReleaseMs = 50.92;
    g.slope = 1.13;
    g.knee = 0.297;
    g.hysteresis = 3.859;
    g.gate = s.gate;
    return g;
}

CompLaw lawOf (const Settings& s) noexcept { return s.model == 1 ? k4CompLaw (s) : compLaw (s); }
GateLaw gateOf (const Settings& s) noexcept { return s.model == 1 ? k4GateLaw (s) : gateLaw (s); }

// the 4K's mic pre ([EXEC] 1 kHz / 100 Hz stairs at Mic 0 … 50 dB, level-compensated by the console): the drive is
// 0.744 dB per Mic dB into a curve that first expands a little (+0.5 … 0.8 dB just before it saturates) and then clips
// at 3.9 (+11.8 dB over full scale at the curve's input), with a slight asymmetry (H2 −40 … −75 dBc); Pre In alone −0.27 dB
double micPre (double x, double drive, double inv) noexcept
{
    constexpr double c = 3.901, p = 23.49, b = 0.0264, a = 0.0230;
    auto curve = [=] (double u) { const double v = u + a * u * u * u; return v / std::pow (1.0 + std::pow (std::abs (v / c), p), 1.0 / p); };
    static const double off = curve (b);
    return 0.96939 * (curve (x * drive + b) - off) * inv; // 0.96939 = −0.27 dB
}

// the Analogue VCA ([EXEC] 1 kHz at −30 … −6 dBFS: H2 −77 … −57 dBc, H3 −107 … −66 dBc, level unchanged): x + 0.0089 x² +
// 0.018 x³, the terms easing off above −10 dBFS
double vcaColour (double x) noexcept
{
    const double e = 1.0 / (1.0 + 2.5 * x * x);
    return x + (0.0089 * x * x + 0.018 * x * x * x) * e;
}
} // namespace

double compStaticGainDb (const Settings& s, double sineDbfs)
{
    // iterate the feedback loop to its fixed point for a steady sine (peak detector on the output)
    const auto law = lawOf (s);
    double gr = 0.0;
    for (int it = 0; it < 200; ++it)
    {
        const double o = sineDbfs - gr - (s.compThreshold + law.thresholdOffset);
        const double target = law.slope * softplus (o, law.knee);
        gr += 0.3 * (target - gr);
    }
    return law.makeup - gr;
}

double compMakeupDb (const Settings& s) noexcept
{
    return s.compRatio > 1.0 ? lawOf (s).makeup : 0.0;
}

void Channel::prepare (double sampleRate) noexcept
{
    sr = sampleRate;
    vcaHp.filter (sr, 1, 3.0, 0.7071); // round 148: the 4K VCA's coupling high-pass
    designedOnce = false;
    reset();
}

void Channel::reset() noexcept
{
    hp1.reset(); hp2.reset(); lp.reset(); vcaHp.reset();
    for (auto& b : bands)
        b.reset();
    for (auto& c : sc)
    {
        c.hp1.reset(); c.hp2.reset(); c.lp.reset();
        for (auto& b : c.bands)
            b.reset();
    }
    env = gr = gr1 = yl = yr = genv = ggr = 0.0;
    gateOpen = false;
}

void Channel::design (const Settings& s) noexcept
{
    const auto eq = [] (const Settings& a, const Settings& b) {
        return a.hpHz == b.hpHz && a.lpKHz == b.lpKHz && a.lfGain == b.lfGain && a.lfHz == b.lfHz && a.lfBell == b.lfBell
            && a.lmfGain == b.lmfGain && a.lmfKHz == b.lmfKHz && a.lmfQ == b.lmfQ && a.hmfGain == b.hmfGain && a.hmfKHz == b.hmfKHz
            && a.hmfQ == b.hmfQ && a.hfGain == b.hfGain && a.hfKHz == b.hfKHz && a.hfBell == b.hfBell
            && a.model == b.model && a.colour == b.colour
            && a.pink == b.pink && a.lmfDiv3 == b.lmfDiv3 && a.hmfMult3 == b.hmfMult3 && a.k4g == b.k4g && a.e18HighPass == b.e18HighPass // round 190lb
            && a.gLaw == b.gLaw; // round 190mt
    };
    if (designedOnce && eq (s, designed))
        return;
    designed = s;
    designedOnce = true;
    Biquad a, b;
    hpOn = highPass (s, sr, a, b);
    if (hpOn) { hp1.b0 = a.b0; hp1.b1 = a.b1; hp1.b2 = a.b2; hp1.a1 = a.a1; hp1.a2 = a.a2; hp2.b0 = b.b0; hp2.b1 = b.b1; hp2.b2 = b.b2; hp2.a1 = b.a1; hp2.a2 = b.a2; }
    lpOn = lowPass (s, sr, a);
    if (lpOn) { lp.b0 = a.b0; lp.b1 = a.b1; lp.b2 = a.b2; lp.a1 = a.a1; lp.a2 = a.a2; }
    for (int i = 0; i < 4; ++i)
    {
        bandOn[i] = bandBiquad (i, s, sr, a);
        if (bandOn[i]) { bands[i].b0 = a.b0; bands[i].b1 = a.b1; bands[i].b2 = a.b2; bands[i].a1 = a.a1; bands[i].a2 = a.a2; }
    }
    const auto coefs = [] (Biquad& to, const Biquad& from) { to.b0 = from.b0; to.b1 = from.b1; to.b2 = from.b2; to.a1 = from.a1; to.a2 = from.a2; };
    for (auto& c : sc) // round 165: the side chain's copies
    {
        coefs (c.hp1, hp1); coefs (c.hp2, hp2); coefs (c.lp, lp);
        for (int i = 0; i < 4; ++i)
            coefs (c.bands[i], bands[i]);
    }
}

void Channel::process (float* l, float* r, int n, const Settings& s) noexcept
{
    design (s);
    const double inG = dbToGain (roundTo (s.inputDb, 0.1)), outG = dbToGain (roundTo (s.outputDb, 0.1)) * (s.phase ? -1.0 : 1.0);
    const auto law = lawOf (s);
    const bool compOn = s.dynOn && s.compRatio > 1.0005;
    const double cA = coefMs (s.compFast ? law.attackFastMs : law.attackSlowMs, sr), cR = coefMs (law.releaseMs, sr), cR1 = coefMs (law.release1Ms, sr);
    const double frac = law.release1Fraction;
    const double dA = coefMs (law.detectorAttackMs, sr), dR = coefMs (law.detectorReleaseMs, sr);
    const double makeup = s.dynOn ? dbToGain (law.makeup) : 1.0;
    const auto gl = gateOf (s);
    const bool gateOn = s.dynOn && gl.range > 0.05;
    const double gA = coefMs (gl.attackMs, sr), gR = coefMs (gl.releaseMs, sr), gD = coefMs (gl.detectorReleaseMs, sr);
    float maxComp = 0.0f, maxGate = 0.0f;
    // round 148: the 4K's mic pre and compressor mix
    const bool micOn = s.model == 1 && s.pre;
    const double micDrive = dbToGain (0.744 * std::clamp (s.micDb, 0.0, 50.0)), micInv = 1.0 / micDrive;
    const double mix = std::clamp (s.compMix, 0.0, 1.0);
    const bool mixed = s.model == 1 && mix < 0.9995;
    // [EXEC] the 4K's hiss between impulses: -106.6 dBFS rms with the Analogue VCA, -104.2 dBFS with the mic pre in
    // (white, the two channels independent [model]), before the output trim
    const double hiss = s.model != 1 ? 0.0 : std::sqrt ((s.vca ? 2.188e-11 : 0.0) + (s.pre ? 3.802e-11 : 0.0));
    auto gauss = [] (std::uint32_t& st) noexcept {
        double sum = 0.0;
        for (int k = 0; k < 4; ++k)
        {
            st ^= st << 13; st ^= st >> 17; st ^= st << 5;
            sum += (double) (st >> 8) * (1.0 / 16777216.0);
        }
        return (sum - 2.0) * 1.7320508;
    };

    auto filters = [this] (double& a, double& b) {
        if (hpOn)
        {
            a = hp2.process (hp1.process ((float) a, 0), 0);
            b = hp2.process (hp1.process ((float) b, 1), 1);
        }
        if (lpOn)
        {
            a = lp.process ((float) a, 0);
            b = lp.process ((float) b, 1);
        }
    };
    auto eq = [this] (double& a, double& b) {
        for (int i = 0; i < 4; ++i)
            if (bandOn[i])
            {
                a = bands[i].process ((float) a, 0);
                b = bands[i].process ((float) b, 1);
            }
    };

    // round 165: EQ to the dynamics' side chain (model 0) — the detectors hear the filtered / equalised signal, the
    // audio goes through the dynamics without them (the filters stay at the input when split)
    const bool eqSc = s.eqToSidechain && s.model == 0;
    auto sideChain = [this, &s] (ScChain& c, double a, double b) {
        if (! s.filtersBeforeDynamics)
        {
            if (hpOn) { a = c.hp2.process (c.hp1.process ((float) a, 0), 0); b = c.hp2.process (c.hp1.process ((float) b, 1), 1); }
            if (lpOn) { a = c.lp.process ((float) a, 0); b = c.lp.process ((float) b, 1); }
        }
        if (s.eqOn)
            for (int k = 0; k < 4; ++k)
                if (bandOn[k]) { a = c.bands[k].process ((float) a, 0); b = c.bands[k].process ((float) b, 1); }
        return std::max (std::abs (a), std::abs (b));
    };

    for (int i = 0; i < n; ++i)
    {
        double a = l[i], b = r[i];
        if (micOn) // the 4K's mic pre sees the input before its trim ([EXEC] the drive does not follow the trim)
        {
            a = micPre (a, micDrive, micInv);
            b = micPre (b, micDrive, micInv);
        }
        a *= inG;
        b *= inG;
        if (eqSc ? s.filtersBeforeDynamics : (s.filtersBeforeDynamics || s.dynamicsAfterEq))
            filters (a, b);
        if (! eqSc && s.dynamicsAfterEq && s.eqOn)
            eq (a, b);

        if (gateOn)
        {
            const double lev = eqSc ? sideChain (sc[0], a, b) : std::max (std::abs (a), std::abs (b));
            genv = lev > genv ? lev : genv + gD * (lev - genv);
            const double levDb = gainToDb (genv);
            double target;
            if (gl.gate)
            {
                if (levDb >= gl.threshold) gateOpen = true;
                else if (levDb < gl.threshold - gl.hysteresis) gateOpen = false;
                target = gateOpen ? 0.0 : gl.range;
            }
            else
            {
                const double u = gl.threshold - levDb;
                target = std::min (gl.range, gl.slope * (u > 30.0 * gl.knee ? u : gl.knee * std::log1p (std::exp (u / gl.knee))));
            }
            ggr += (target < ggr ? gA : gR) * (target - ggr); // less reduction = opening (attack), more = closing (release)
            const double g = dbToGain (-ggr);
            a *= g;
            b *= g;
            maxGate = std::max (maxGate, (float) ggr);
        }
        if (compOn)
        {
            // feedback compressor: the detector hears the compressor's own output (the previous sample)
            const double det = eqSc ? sideChain (sc[1], yl, yr) : std::max (std::abs (yl), std::abs (yr));
            env += (det > env ? dA : dR) * (det - env);
            const double o = gainToDb (env) - (s.compThreshold + law.thresholdOffset);
            const double target = law.slope * softplus (o, law.knee);
            gr1 += (target > gr1 ? cA : cR1) * (target - gr1);
            gr += (target > gr ? cA : cR) * (target - gr);
            const double grNow = frac * gr1 + (1.0 - frac) * gr;
            const double g = dbToGain (-grNow);
            yl = a * g;
            yr = b * g;
            if (mixed) // round 148: the 4K's mix blends the reduction in dB ([EXEC] 50 % = half the dB at every level)
            {
                const double gm = dbToGain (-grNow * mix);
                a *= gm;
                b *= gm;
            }
            else
            {
                a = yl;
                b = yr;
            }
            maxComp = std::max (maxComp, (float) (frac * gr1 + (1.0 - frac) * gr));
        }
        if (s.model == 1 && s.vca)
        {
            // [EXEC] pink noise through the VCA: a coupling high-pass (2nd order, ~3 Hz: 25 deg at 10 Hz, -2.8 dB at 3 Hz)
            a = vcaHp.process ((float) vcaColour (a), 0);
            b = vcaHp.process ((float) vcaColour (b), 1);
        }
        a *= makeup;
        b *= makeup;

        if (! s.dynamicsAfterEq && ! eqSc)
        {
            if (! s.filtersBeforeDynamics)
                filters (a, b);
            if (s.eqOn)
                eq (a, b);
        }
        if (hiss > 0.0)
        {
            a += hiss * gauss (noise[0]);
            b += hiss * gauss (noise[1]);
        }
        l[i] = (float) (a * outG);
        r[i] = (float) (b * outG);
    }
    lastComp = maxComp;
    lastGate = maxGate;
}
} // namespace echannel


} // vmatch

