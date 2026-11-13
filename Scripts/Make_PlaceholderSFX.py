# Director's Cut - synthesized PLACEHOLDER sound effects (pure Python, no samples, no licenses).
# They only exist so every sound slot can be heard in the test map. The audio owner replaces them:
# reimport a real file over the same asset name (/Game/DirectorsCut/Audio/SFX/S_*) and nothing else changes.
#   py Scripts/Make_PlaceholderSFX.py   -> Art/Audio/SFX_Placeholder/S_*.wav (44.1 kHz, 16-bit mono)
import math, os, random, struct, wave

OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "Art", "Audio", "SFX_Placeholder")
os.makedirs(OUT, exist_ok=True)
SR = 44100
random.seed(7)


def write(name, samples, gain=0.8):
    peak = max(1e-6, max(abs(s) for s in samples))
    data = b"".join(struct.pack("<h", int(max(-1.0, min(1.0, s / peak * gain)) * 32767)) for s in samples)
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes(data)


def env(t, dur, a=0.005, r=None):
    """attack / exponential-ish release envelope"""
    r = r if r is not None else dur * 0.6
    if t < a:
        return t / a
    return max(0.0, 1.0 - (t - a) / max(1e-6, dur - a)) ** (dur / max(r, 1e-6) * 0.6)


def noise():
    return random.uniform(-1.0, 1.0)


class LP:
    """one-pole low-pass, cutoff can change per sample"""
    def __init__(self):
        self.y = 0.0
    def __call__(self, x, fc):
        a = 1.0 - math.exp(-2.0 * math.pi * max(20.0, fc) / SR)
        self.y += a * (x - self.y)
        return self.y


def saw(ph):
    return 2.0 * (ph - math.floor(ph + 0.5))


def make(dur, fn):
    return [fn(i / SR) for i in range(int(dur * SR))]


# sword whoosh: noise through a sweeping band (low-pass minus lower low-pass), swell then fade
def swing():
    lo, hi = LP(), LP()
    dur = 0.32
    def f(t):
        p = t / dur
        fc = 600 + 3200 * math.sin(math.pi * p)
        n = noise()
        return (hi(n, fc) - lo(n, fc * 0.35)) * math.sin(math.pi * p) ** 1.5
    return make(dur, f)


# sword hit: meaty thump + short bright crack
def sword_hit():
    lp = LP(); dur = 0.22
    def f(t):
        thump = math.sin(2 * math.pi * (110 - 160 * t) * t) * math.exp(-t * 22)
        crack = lp(noise(), 5000) * math.exp(-t * 60)
        return thump * 0.9 + crack * 0.8
    return make(dur, f)


# boar voice: buzzy saw with a nasal low-pass, pitch contour f(t)
def oink(dur, f0, f1, wobble=6.0, bright=900, rough=0.25):
    lp = LP(); ph = [0.0]
    def f(t):
        p = t / dur
        hz = f0 + (f1 - f0) * p + math.sin(2 * math.pi * wobble * t) * f0 * 0.06
        ph[0] += hz / SR
        v = saw(ph[0]) + rough * noise()
        return lp(v, bright + 600 * math.sin(math.pi * p)) * env(t, dur, 0.02, dur * 0.5)
    return make(dur, f)


# hooves: a burst of low thuds
def charge():
    dur = 0.7; hits = [0.0, 0.11, 0.2, 0.33, 0.43, 0.55]
    lp = LP()
    def f(t):
        s = 0.0
        for h in hits:
            if t >= h:
                d = t - h
                s += math.sin(2 * math.pi * 70 * d) * math.exp(-d * 30)
        return s * 0.8 + lp(noise(), 400) * 0.25 * (1 - t / dur)
    return make(dur, f)


# chomp: two quick clicks
def bite():
    dur = 0.2; lp = LP()
    def f(t):
        s = 0.0
        for h in (0.0, 0.08):
            if t >= h:
                d = t - h
                s += lp(noise(), 2500) * math.exp(-d * 90) + math.sin(2 * math.pi * 180 * d) * math.exp(-d * 50) * 0.6
        return s
    return make(dur, f)


# player "oof": falling low vowel
def hurt():
    return oink(0.24, 210, 130, wobble=0, bright=700, rough=0.08)


# meat pickup: two bright rising blips
def pickup():
    dur = 0.22
    def f(t):
        hz = 880 if t < 0.09 else 1320
        d = t if t < 0.09 else t - 0.09
        return math.sin(2 * math.pi * hz * t) * math.exp(-d * 18)
    return make(dur, f)


# boss shot: descending square zap
def boss_shot():
    dur = 0.35; ph = [0.0]
    def f(t):
        hz = 900 * math.exp(-t * 6) + 120
        ph[0] += hz / SR
        return (1.0 if (ph[0] % 1.0) < 0.5 else -1.0) * env(t, dur, 0.003, 0.25) * 0.6
    return make(dur, f)


# boss hit: dull thump + rattle
def boss_hit():
    lp = LP(); dur = 0.18
    def f(t):
        return math.sin(2 * math.pi * 90 * t) * math.exp(-t * 25) + lp(noise(), 1500) * math.exp(-t * 40) * 0.5
    return make(dur, f)


# boss defeat: sad trombone-ish descending steps
def boss_defeat():
    notes = [(392, 0.0), (370, 0.35), (349, 0.7), (330, 1.05)]
    dur = 1.9; lp = LP(); ph = [0.0]
    def f(t):
        hz = notes[0][0]
        for n, s in notes:
            if t >= s:
                hz = n
        if t > 1.05:
            hz *= 1.0 + 0.03 * math.sin(2 * math.pi * 6 * t)   # wobbly last note
        ph[0] += hz / SR
        return lp(saw(ph[0]), 1200) * env(t, dur, 0.02, 0.9)
    return make(dur, f)


write("S_SwordSwing", swing(), 0.7)
write("S_SwordHit", sword_hit())
write("S_BoarGrunt", oink(0.35, 95, 80, wobble=9, bright=700, rough=0.35))
write("S_BoarAggro", oink(0.45, 120, 260, wobble=12, bright=1100, rough=0.4))
write("S_BoarCharge", charge())
write("S_BoarBite", bite())
write("S_BoarSqueal", oink(0.3, 600, 1100, wobble=14, bright=2600, rough=0.15))
write("S_BoarDeath", oink(0.8, 900, 260, wobble=7, bright=2000, rough=0.2))
write("S_PlayerHurt", hurt(), 0.7)
write("S_MeatPickup", pickup(), 0.6)
write("S_BossShot", boss_shot(), 0.6)
write("S_BossHit", boss_hit())
write("S_BossDefeat", boss_defeat(), 0.7)
print("wrote", len([f for f in os.listdir(OUT) if f.endswith(".wav")]), "files to", OUT)
