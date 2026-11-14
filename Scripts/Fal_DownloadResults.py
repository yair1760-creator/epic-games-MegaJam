# usage: py fal_dl.py <persisted tool-result json> <outdir> ; NAMES maps index -> file name
import json, sys, os, urllib.request
NAMES = {}
for i, n in enumerate(["SwordSwing", "SwordHit", "BoarGrunt", "BoarAggro", "BoarCharge", "BoarBite", "BoarSqueal", "BoarDeath",
                       "PlayerHurt", "MeatPickup", "BossShot", "BossHit", "BossDefeat", "Cut", "QuestTick", "QuestComplete"]):
    for t in range(3):
        NAMES[i * 3 + t] = "S_%s_t%d" % (n, t + 1)
MUSIC = {100: "M_Heroic_sonilo", 101: "M_Frantic_sonilo", 102: "M_Romance_sonilo", 103: "M_Boss_sonilo", 104: "M_Victory_sonilo",
         110: "M_Heroic_sa3", 111: "M_Frantic_sa3", 112: "M_Romance_sa3", 113: "M_Boss_sa3", 114: "M_Victory_sa3"}
NAMES.update(MUSIC)
raw = json.load(open(sys.argv[1], encoding="utf-8"))
data = json.loads(raw[0]["text"]) if isinstance(raw, list) else raw   # tool-result wrapper, or the plain result JSON
out = sys.argv[2]
for j in data["jobs"]:
    idx = j["index"]
    res = j.get("result") or {}
    a = res.get("audio") or (res.get("audios") or [{}])[0]
    url = a.get("url")
    if not url:
        print(idx, "NO RESULT", j.get("status")); continue
    ext = os.path.splitext(url)[1] or ".bin"
    path = os.path.join(out, NAMES.get(idx, "job%d" % idx) + ext)
    urllib.request.urlretrieve(url, path)
    print(idx, os.path.basename(path), os.path.getsize(path))
