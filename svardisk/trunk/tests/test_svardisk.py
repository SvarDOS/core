#!/usr/bin/env python3
"""Image tests for the SV_HOST build of SVARDISK.

The images are built in a temporary directory and are not kept.
"""

import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, "svardisk")


def fail(msg, out="", err=""):
    sys.stdout.write(msg + "\n")
    if out:
        sys.stdout.write("--- stdout ---\n")
        sys.stdout.write(out)
        if not out.endswith("\n"):
            sys.stdout.write("\n")
    if err:
        sys.stdout.write("--- stderr ---\n")
        sys.stdout.write(err)
        if not err.endswith("\n"):
            sys.stdout.write("\n")
    sys.exit(1)


def run(args):
    proc = subprocess.run(
        [BIN] + args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return proc.returncode, proc.stdout.decode("latin1"), proc.stderr.decode("latin1")


def once(out, err, phrase):
    n = out.count(phrase)
    if n != 1:
        fail("expected one %r, found %d" % (phrase, n), out, err)


def absent(out, err, phrase):
    if phrase in out:
        fail("did not expect %r" % phrase, out, err)


def put16(buf, off, val):
    buf[off] = val & 0xFF
    buf[off + 1] = (val >> 8) & 0xFF


def put32(buf, off, val):
    buf[off:off + 4] = (val & 0xFFFFFFFF).to_bytes(4, "little")


def name11(text):
    if "." in text:
        base, ext = text.split(".", 1)
    else:
        base, ext = text, ""
    raw = (base.upper()[:8].ljust(8) + ext.upper()[:3].ljust(3)).encode("ascii")
    if len(raw) != 11:
        raise SystemExit("bad name " + text)
    return raw


def checksum11(raw):
    total = 0
    for byte in raw:
        total = (((0x80 if (total & 1) else 0) + (total >> 1) + byte) & 0xFF)
    return total


def entry(name, attr, cluster, size):
    ent = bytearray(32)
    ent[0:11] = name11(name)
    ent[11] = attr
    ent[20] = (cluster >> 16) & 0xFF
    ent[21] = (cluster >> 24) & 0xFF
    ent[26] = cluster & 0xFF
    ent[27] = (cluster >> 8) & 0xFF
    ent[28:32] = (size & 0xFFFFFFFF).to_bytes(4, "little")
    return ent


def dot_entry(cluster):
    ent = bytearray(32)
    ent[0] = ord(".")
    for i in range(1, 11):
        ent[i] = ord(" ")
    ent[11] = 0x10
    ent[26] = cluster & 0xFF
    ent[27] = (cluster >> 8) & 0xFF
    return ent


def dotdot_entry(cluster):
    ent = bytearray(32)
    ent[0] = ord(".")
    ent[1] = ord(".")
    for i in range(2, 11):
        ent[i] = ord(" ")
    ent[11] = 0x10
    ent[26] = cluster & 0xFF
    ent[27] = (cluster >> 8) & 0xFF
    return ent


def layout(reserved, nfats, fatsz, root_ent, spc, total):
    root_sectors = ((root_ent * 32) + 511) >> 9
    root_sector = reserved + nfats * fatsz
    data_start = root_sector + root_sectors
    clusters = (total - data_start) // spc
    return clusters, data_start, root_sector, root_sectors


def write_boot(img, reserved, nfats, fatsz, root_ent, spc, total, fat32, typestr):
    img[0:3] = b"\xeb\x3c\x90"
    put16(img, 11, 512)
    img[13] = spc
    put16(img, 14, reserved)
    img[16] = nfats
    put16(img, 17, root_ent)
    if total < 65536:
        put16(img, 19, total)
    else:
        put32(img, 32, total)
    img[21] = 0xF8
    put16(img, 24, 32)
    put16(img, 26, 64)
    if fat32:
        put32(img, 36, fatsz)
        put32(img, 44, 2)
        put16(img, 48, 1)
        img[82:90] = typestr
    else:
        put16(img, 22, fatsz)
        img[54:62] = typestr
    img[510] = 0x55
    img[511] = 0xAA


def set_fat12(img, base, cluster, value):
    off = base + (cluster + (cluster >> 1))
    word = img[off] | (img[off + 1] << 8)
    if cluster & 1:
        word = (word & 0x000F) | ((value & 0xFFF) << 4)
    else:
        word = (word & 0xF000) | (value & 0xFFF)
    img[off] = word & 0xFF
    img[off + 1] = (word >> 8) & 0xFF


def get_fat12(img, base, cluster):
    off = base + (cluster + (cluster >> 1))
    word = img[off] | (img[off + 1] << 8)
    if cluster & 1:
        word >>= 4
    return word & 0xFFF


def set_fat16(img, base, cluster, value):
    off = base + cluster * 2
    put16(img, off, value)


def get_fat16(img, base, cluster):
    off = base + cluster * 2
    return img[off] | (img[off + 1] << 8)


def set_fat32(img, base, cluster, value):
    put32(img, base + cluster * 4, value)


def get_fat32(img, base, cluster):
    off = base + cluster * 4
    return int.from_bytes(img[off:off + 4], "little")


def find_name(img, text):
    raw = name11(text)
    hits = []
    start = 0
    while True:
        i = img.find(raw, start)
        if i < 0:
            break
        if i % 32 == 0:
            hits.append(i)
        start = i + 1
    return hits


def ent_cluster(img, off):
    return img[off + 26] | (img[off + 27] << 8)


def ent_size(img, off):
    return int.from_bytes(img[off + 28:off + 32], "little")


def build_fat12(broken):
    reserved, nfats, fatsz, root_ent, spc, total = 1, 2, 1, 32, 1, 105
    clusters, data_start, root_sector, root_sectors = layout(
        reserved, nfats, fatsz, root_ent, spc, total)
    if clusters != 100 or data_start != 5 or root_sector != 3 or root_sectors != 2:
        raise SystemExit("fat12 geometry %s" % (
            (clusters, data_start, root_sector, root_sectors),))
    img = bytearray(total * 512)
    write_boot(
        img, reserved, nfats, fatsz, root_ent, spc, total, False,
        b"BADTYPE " if broken else b"FAT12   ")
    base = reserved * 512
    if broken:
        set_fat12(img, base, 2, 0xFFF)
        set_fat12(img, base, 3, 4)
        set_fat12(img, base, 4, 5)
        set_fat12(img, base, 5, 0xFFF)
        set_fat12(img, base, 6, 0xFFF)
        set_fat12(img, base, 7, 8)
        set_fat12(img, base, 8, 7)
        set_fat12(img, base, 9, 0xFFF)
        set_fat12(img, base, 10, 0x0A00)
        set_fat12(img, base, 11, 0xFFF)
        set_fat12(img, base, 12, 0xFFF)
        set_fat12(img, base, 13, 14)
        set_fat12(img, base, 14, 13)
        set_fat12(img, base, 15, 16)
        set_fat12(img, base, 1, 0xFFF)
        if get_fat12(img, base, 0) != 0 or get_fat12(img, base, 3) != 4:
            raise SystemExit("fat12 pack mismatch")
        if get_fat12(img, base, 10) != 0x0A00 or get_fat12(img, base, 8) != 7:
            raise SystemExit("fat12 chain mismatch")
        names = [
            entry("A.TXT", 0x20, 2, 10),
            entry("B.TXT", 0x20, 6, 10),
            entry("C.TXT", 0x20, 6, 10),
            entry("D.TXT", 0x20, 7, 1024),
            entry("E.TXT", 0x20, 9, 5000),
            entry("F.TXT", 0x20, 10, 100),
        ]
        lfn = bytearray(32)
        lfn[0] = 0x41
        lfn[11] = 0x0F
        lfn[1] = ord("G")
        lfn[13] = (checksum11(name11("G.TXT")) + 1) & 0xFF
        names.append(lfn)
        names.append(entry("G.TXT", 0x20, 11, 10))
        names.append(entry("SUBDIR", 0x10, 12, 0))
        names.append(entry("H.TXT", 0x20, 15, 512))
        off = root_sector * 512
        for i, ent in enumerate(names):
            img[off + i * 32:off + i * 32 + 32] = ent
        sub = (data_start + (12 - 2) * spc) * 512
        img[sub:sub + 32] = dot_entry(150)
        img[sub + 32:sub + 64] = dotdot_entry(0)
        lfn_off = off + 6 * 32
    else:
        set_fat12(img, base, 0, 0xFF8)
        set_fat12(img, base, 1, 0xFFF)
        if get_fat12(img, base, 0) != 0xFF8 or get_fat12(img, base, 1) != 0xFFF:
            raise SystemExit("clean fat12 media mismatch")
        lfn_off = 0
    other = base + fatsz * 512
    img[other:other + 512] = img[base:base + 512]
    if broken:
        img[other + 400] = 0x5A
        if img[other + 400] == img[base + 400]:
            raise SystemExit("mirror byte did not change")
    info = {
        "base": base,
        "other": other,
        "data_start": data_start,
        "lfn_off": lfn_off,
        "clusters": clusters,
    }
    return img, info


def build_fat16():
    reserved, nfats, fatsz, root_ent, spc = 1, 2, 20, 512, 1
    root_sectors = ((root_ent * 32) + 511) >> 9
    data_start = reserved + nfats * fatsz + root_sectors
    clusters = 5000
    total = data_start + clusters * spc
    got, data2, root_sector, root2 = layout(
        reserved, nfats, fatsz, root_ent, spc, total)
    if got != 5000 or data2 != data_start or root2 != 32:
        raise SystemExit("fat16 geometry %s" % ((got, data2, root_sector, root2),))
    if (fatsz * 512) // 2 < clusters + 2:
        raise SystemExit("fat16 table too small")
    img = bytearray(total * 512)
    write_boot(
        img, reserved, nfats, fatsz, root_ent, spc, total, False, b"FAT16   ")
    base = reserved * 512
    set_fat16(img, base, 0, 0xFFF8)
    set_fat16(img, base, 1, 0x3FFF)
    other = base + fatsz * 512
    img[other:other + fatsz * 512] = img[base:base + fatsz * 512]
    return img, {"base": base, "other": other, "fatsz": fatsz}


def build_fat32(free_count):
    reserved, nfats, fatsz, spc = 32, 2, 512, 1
    clusters = 65525
    data_start = reserved + nfats * fatsz
    total = data_start + clusters * spc
    got, data2, root_sector, root_sectors = layout(
        reserved, nfats, fatsz, 0, spc, total)
    if got != 65525 or data2 != 1056 or root_sectors != 0 or total != 66581:
        raise SystemExit("fat32 geometry %s" % (
            (got, data2, root_sector, root_sectors, total),))
    if (fatsz * 512) // 4 < clusters + 2:
        raise SystemExit("fat32 table too small")
    img = bytearray(total * 512)
    write_boot(
        img, reserved, nfats, fatsz, 0, spc, total, True, b"FAT32   ")
    info = bytearray(512)
    put32(info, 0, 0x41615252)
    put32(info, 484, 0x61417272)
    put32(info, 488, free_count)
    put32(info, 492, 0xFFFFFFFF)
    info[510] = 0x55
    info[511] = 0xAA
    img[512:1024] = info
    base = reserved * 512
    set_fat32(img, base, 0, 0x0FFFFFF8)
    set_fat32(img, base, 1, 0x0FFFFFFF)
    set_fat32(img, base, 2, 0x0FFFFFFF)
    other = base + fatsz * 512
    img[other:other + fatsz * 512] = img[base:base + fatsz * 512]
    return img, {"base": base, "other": other, "fatsz": fatsz}


def write_file(path, data):
    with open(path, "wb") as handle:
        handle.write(data)


def read_file(path):
    with open(path, "rb") as handle:
        return handle.read()


def expect_code(code, out, err, want):
    if code != want:
        fail("exit %d, expected %d" % (code, want), out, err)
    if err:
        fail("unexpected stderr", out, err)


def test_cli():
    code, out, err = run(["/?"])
    expect_code(code, out, err, 0)
    once(out, err, "SvarDISK is the SvarDOS tool for disk repair")
    once(out, err, "CHKDSK or SCANDISK on other systems.")
    once(out, err, "Copyright (C) 2026 Mateusz Viste")
    once(out, err, "SVARDISK [drive:] [/F] [/S] [/R]")
    once(out, err, "/F   write repairs.")
    once(out, err, "/S   read clusters.")
    once(out, err, "/R   re-read bad clusters.")

    code, out, err = run([])
    expect_code(code, out, err, 3)
    once(out, err, "Name a drive or an image file.")

    code, out, err = run(["/Z"])
    expect_code(code, out, err, 3)
    once(out, err, "Bad command line.")

    code, out, err = run(["/FF"])
    expect_code(code, out, err, 3)
    once(out, err, "Bad command line.")

    code, out, err = run(["/SR"])
    expect_code(code, out, err, 3)
    once(out, err, "Bad command line.")

    code, out, err = run(["C:"])
    expect_code(code, out, err, 3)
    once(out, err, "The image file cannot be opened.")

    code, out, err = run(["/no/such/svardisk-image.img"])
    expect_code(code, out, err, 3)
    once(out, err, "The image file cannot be opened.")
    print("PASS cli", flush=True)


def test_not_fat(tmp):
    path = os.path.join(tmp, "empty.bin")
    write_file(path, b"\x00" * 1024)
    code, out, err = run([path])
    expect_code(code, out, err, 3)
    once(out, err, "This is not a FAT disk.")

    short = os.path.join(tmp, "short.bin")
    write_file(short, b"\x00" * 100)
    code, out, err = run([short])
    expect_code(code, out, err, 3)
    once(out, err, "The boot sector cannot be read.")

    wide = os.path.join(tmp, "wide.bin")
    blob = bytearray(512)
    put16(blob, 11, 1024)
    blob[510] = 0x55
    blob[511] = 0xAA
    write_file(wide, blob)
    code, out, err = run([wide])
    expect_code(code, out, err, 3)
    once(out, err, "Only 512-byte sectors are supported.")
    print("PASS not-fat", flush=True)


def test_clean12(tmp):
    img, _info = build_fat12(False)
    path = os.path.join(tmp, "clean12.img")
    write_file(path, img)
    before = bytes(img)
    code, out, err = run([path])
    expect_code(code, out, err, 0)
    once(out, err, "FAT12")
    once(out, err, "100 clusters")
    once(out, err, "Read only.")
    once(out, err, "Free clusters: 100")
    once(out, err, "No problems found.")
    if read_file(path) != before:
        fail("clean FAT12 was modified")
    code, out, err = run([path, "/F"])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "Repairs written.")
    if read_file(path) != before:
        fail("clean FAT12 changed under /F")
    print("PASS clean12", flush=True)


def test_broken12(tmp):
    img, info = build_fat12(True)
    path = os.path.join(tmp, "broken12.img")
    write_file(path, img)
    before = bytes(img)
    code, out, err = run([path])
    expect_code(code, out, err, 2)
    for phrase in (
        "Checking ",
        "FAT12",
        "100 clusters",
        "Read only.",
        "The filesystem name is wrong.",
        "The two copies of the allocation table differ.",
        "Invalid cluster pointer 10",
        "The media byte is wrong.",
        "File C.TXT cross-links cluster 6",
        "Loop in a cluster chain at 7",
        "File E.TXT is larger than its cluster chain.",
        "A broken long filename was removed.",
        "A subdirectory entry is wrong.",
        "File H.TXT has a broken cluster chain.",
        "Lost chain at cluster 3",
        "Loop in a cluster chain at 13",
        "Free clusters: 86",
        "Problems found: 12",
    ):
        once(out, err, phrase)
    absent(out, err, "Saved as")
    if read_file(path) != before:
        fail("read-only check modified the image")

    code, out, err = run([path, "/f"])
    expect_code(code, out, err, 1)
    once(out, err, "Repairs will be written.")
    once(out, err, "Saved as FILE0000.CHK")
    once(out, err, "Saved as FILE0001.CHK")
    once(out, err, "Repairs written. Problems fixed: 12")
    absent(out, err, "Restart the computer")
    fixed = read_file(path)
    base = info["base"]
    if fixed[54:62] != b"FAT12   ":
        fail("filesystem name was not repaired", out, err)
    if get_fat12(fixed, base, 0) != 0xFF8 or get_fat12(fixed, base, 1) != 0xFFF:
        fail("media bytes are %03X %03X" % (
            get_fat12(fixed, base, 0), get_fat12(fixed, base, 1)), out, err)
    if fixed[base:base + 3] != b"\xf8\xff\xff":
        fail("classic media bytes are %s" % fixed[base:base + 3].hex(), out, err)
    expect = {
        2: 0xFFF, 3: 4, 4: 5, 5: 0xFFF, 6: 0xFFF, 7: 8, 8: 0xFFF,
        9: 0xFFF, 10: 0xFFF, 11: 0xFFF, 12: 0xFFF, 13: 14, 14: 0xFFF,
        15: 0xFFF, 16: 0,
    }
    for cluster, value in expect.items():
        got = get_fat12(fixed, base, cluster)
        if got != value:
            fail("cluster %d is %03X, expected %03X" % (cluster, got, value), out, err)
    for cluster in range(17, 102):
        got = get_fat12(fixed, base, cluster)
        if got != 0:
            fail("cluster %d should be free, is %03X" % (cluster, got), out, err)
    if fixed[base:base + 512] != fixed[info["other"]:info["other"] + 512]:
        fail("FAT copies still differ", out, err)

    def one(text):
        hits = find_name(fixed, text)
        if len(hits) != 1:
            fail("%s found %d times" % (text, len(hits)), out, err)
        return hits[0]

    if ent_cluster(fixed, one("C.TXT")) != 0 or ent_size(fixed, one("C.TXT")) != 0:
        fail("cross-linked file was not cleared", out, err)
    if ent_size(fixed, one("E.TXT")) != 512 or ent_cluster(fixed, one("E.TXT")) != 9:
        fail("oversized file was not shortened", out, err)
    if ent_cluster(fixed, one("H.TXT")) != 15 or ent_size(fixed, one("H.TXT")) != 512:
        fail("broken chain file changed", out, err)
    if ent_cluster(fixed, one("FILE0000.CHK")) != 3 or ent_size(fixed, one("FILE0000.CHK")) != 1536:
        fail("lost chain file is wrong", out, err)
    if ent_cluster(fixed, one("FILE0001.CHK")) != 13 or ent_size(fixed, one("FILE0001.CHK")) != 1024:
        fail("orphan loop file is wrong", out, err)
    if find_name(fixed, "FILE0002.CHK"):
        fail("an extra recovered file was written", out, err)
    sub = (info["data_start"] + 10) * 512
    if ent_cluster(fixed, sub) != 12:
        fail("dot cluster is %d" % ent_cluster(fixed, sub), out, err)
    if ent_cluster(fixed, one("G.TXT")) != 11:
        fail("short name behind the long name was lost", out, err)
    for blob in (fixed[3 * 512:5 * 512], fixed[sub:sub + 512]):
        for i in range(0, len(blob), 32):
            if blob[i + 11] == 0x0F and blob[i] not in (0, 0xE5):
                fail("a long filename entry is still present", out, err)

    code, out, err = run([path, "/F"])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    once(out, err, "Free clusters: 86")
    if read_file(path) != fixed:
        fail("second /F changed a repaired image")
    print("PASS broken12", flush=True)


def test_fat16(tmp):
    img, info = build_fat16()
    path = os.path.join(tmp, "fat16.img")
    write_file(path, img)
    before = bytes(img)
    code, out, err = run([path])
    expect_code(code, out, err, 2)
    once(out, err, "FAT16")
    once(out, err, "5000 clusters")
    once(out, err, "The disk was not unmounted cleanly.")
    once(out, err, "Problems found: 1")
    once(out, err, "Free clusters: 5000")
    if read_file(path) != before:
        fail("read-only FAT16 was modified")
    code, out, err = run([path, "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Problems fixed: 1")
    fixed = read_file(path)
    if get_fat16(fixed, info["base"], 1) != 0xFFFF:
        fail("clean bit is %04X" % get_fat16(fixed, info["base"], 1), out, err)
    if get_fat16(fixed, info["other"], 1) != 0xFFFF:
        fail("second FAT clean bit was not copied", out, err)
    if get_fat16(fixed, info["base"], 0) != 0xFFF8:
        fail("FAT16 media changed", out, err)
    code, out, err = run([path])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    if read_file(path) != fixed:
        fail("clean recheck changed FAT16")
    print("PASS fat16", flush=True)


def test_fat32(tmp):
    img, info = build_fat32(0)
    path = os.path.join(tmp, "fat32.img")
    write_file(path, img)
    before = bytes(img)
    print("PASS fat32-built", flush=True)
    code, out, err = run([path])
    expect_code(code, out, err, 2)
    once(out, err, "FAT32")
    once(out, err, "65525 clusters")
    once(out, err, "The free-space record is wrong.")
    once(out, err, "Problems found: 1")
    once(out, err, "Free clusters: 65524")
    if read_file(path) != before:
        fail("read-only FAT32 was modified")
    code, out, err = run([path, "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Problems fixed: 1")
    fixed = read_file(path)
    if int.from_bytes(fixed[512 + 488:512 + 492], "little") != 65524:
        fail("free count was not rewritten", out, err)
    if int.from_bytes(fixed[512 + 492:512 + 496], "little") != 3:
        fail("next free is wrong", out, err)
    if get_fat32(fixed, info["base"], 2) != 0x0FFFFFFF:
        fail("root cluster changed", out, err)
    code, out, err = run([path])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "free-space record")
    if read_file(path) != fixed:
        fail("clean recheck changed FAT32")

    unknown = bytearray(before)
    put32(unknown, 512 + 488, 0xFFFFFFFF)
    upath = os.path.join(tmp, "fat32unk.img")
    write_file(upath, unknown)
    code, out, err = run([upath])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "free-space record")
    if read_file(upath) != bytes(unknown):
        fail("unknown free count was treated as a problem")
    code, out, err = run([upath, "/F"])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "Repairs written.")
    after = read_file(upath)
    if int.from_bytes(after[512 + 488:512 + 492], "little") != 65524:
        fail("unknown free count was not filled in", out, err)
    print("PASS fat32", flush=True)


def mirror_fat(img, info):
    fatsz = info.get("fatsz", 1)
    n = fatsz * 512
    img[info["other"]:info["other"] + n] = img[info["base"]:info["base"] + n]


def build_spc2():
    reserved, nfats, fatsz, root_ent, spc, total = 1, 2, 1, 32, 2, 25
    clusters, data_start, root_sector, root_sectors = layout(
        reserved, nfats, fatsz, root_ent, spc, total)
    if clusters != 10 or data_start != 5 or root_sectors != 2:
        raise SystemExit("spc2 geometry %s" % (
            (clusters, data_start, root_sector, root_sectors),))
    img = bytearray(total * 512)
    write_boot(
        img, reserved, nfats, fatsz, root_ent, spc, total, False, b"FAT12   ")
    base = reserved * 512
    set_fat12(img, base, 0, 0xFF8)
    set_fat12(img, base, 1, 0xFFF)
    info = {"base": base, "other": base + fatsz * 512, "fatsz": fatsz}
    mirror_fat(img, info)
    return img, info


def test_surface(tmp):
    img, info = build_fat12(False)
    path = os.path.join(tmp, "surf12.img")
    write_file(path, img)
    before = bytes(img)
    code, out, err = run([path, "/s"])
    expect_code(code, out, err, 0)
    once(out, err, "Reading the surface.")
    once(out, err, "No problems found.")
    if read_file(path) != before:
        fail("clean /S changed the image")
    code, out, err = run([path, "/S", "/F"])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "Repairs written.")
    if read_file(path) != before:
        fail("clean /S /F changed the image")

    short_path = os.path.join(tmp, "surf12-free.img")
    write_file(short_path, before[:-512])
    code, out, err = run([short_path, "/S"])
    expect_code(code, out, err, 2)
    once(out, err, "Unreadable free cluster 101")
    once(out, err, "Problems found: 1")
    absent(out, err, "Repairs written.")
    absent(out, err, "Unreadable cluster in use")
    if read_file(short_path) != before[:-512]:
        fail("read-only surface scan changed a free cluster")
    code, out, err = run([short_path, "/s", "/f"])
    expect_code(code, out, err, 1)
    once(out, err, "Unreadable free cluster 101")
    once(out, err, "Repairs written. Problems fixed: 1")
    once(out, err, "Free clusters: 99")
    marked = read_file(short_path)
    if get_fat12(marked, info["base"], 101) != 0xFF7:
        fail("free cluster was not marked bad", out, err)
    if get_fat12(marked, info["other"], 101) != 0xFF7:
        fail("second FAT was not marked bad", out, err)
    if get_fat12(marked, info["base"], 100) != 0:
        fail("a neighbor cluster was marked", out, err)
    code, out, err = run([short_path])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "Unreadable")
    absent(out, err, "bad cluster")
    if read_file(short_path) != marked:
        fail("recheck changed a marked cluster")

    used = bytearray(before)
    set_fat12(used, info["base"], 100, 101)
    set_fat12(used, info["base"], 101, 0xFFF)
    mirror_fat(used, info)
    used[3 * 512:3 * 512 + 32] = entry("A.TXT", 0x20, 100, 1024)
    used_path = os.path.join(tmp, "surf12-used.img")
    used_short = bytes(used)[:-512]
    write_file(used_path, used_short)
    code, out, err = run([used_path, "/S", "/F"])
    expect_code(code, out, err, 2)
    once(out, err, "Unreadable cluster in use 101")
    once(out, err, "Problems found: 1")
    absent(out, err, "Repairs written.")
    absent(out, err, "Unreadable free cluster")
    if read_file(used_path) != used_short:
        fail("in-use cluster was changed")
    kept = read_file(used_path)
    if get_fat12(kept, info["base"], 100) != 101 or get_fat12(kept, info["base"], 101) != 0xFFF:
        fail("chain was rewritten", out, err)
    hits = find_name(kept, "A.TXT")
    if len(hits) != 1 or ent_cluster(kept, hits[0]) != 100 or ent_size(kept, hits[0]) != 1024:
        fail("directory entry was rewritten", out, err)

    wide, wide_info = build_spc2()
    wide_path = os.path.join(tmp, "surf12-spc2.img")
    write_file(wide_path, bytes(wide)[:-512])
    code, out, err = run([wide_path, "/S", "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Unreadable free cluster 11")
    once(out, err, "Free clusters: 9")
    wide_marked = read_file(wide_path)
    if get_fat12(wide_marked, wide_info["base"], 11) != 0xFF7:
        fail("two-sector cluster was not marked", out, err)
    if get_fat12(wide_marked, wide_info["other"], 11) != 0xFF7:
        fail("second FAT missed the two-sector cluster", out, err)
    if get_fat12(wide_marked, wide_info["base"], 10) != 0:
        fail("earlier cluster of a two-sector disk was marked", out, err)

    bad = bytearray(before)
    set_fat12(bad, info["base"], 5, 0xFF7)
    mirror_fat(bad, info)
    bad_path = os.path.join(tmp, "surf12-bad.img")
    write_file(bad_path, bad)
    bad_before = bytes(bad)
    code, out, err = run([bad_path, "/S"])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "Bad cluster can be read")
    absent(out, err, "Unreadable")
    if read_file(bad_path) != bad_before:
        fail("/S reread a cluster marked bad")
    code, out, err = run([bad_path, "/R"])
    expect_code(code, out, err, 2)
    once(out, err, "Bad cluster can be read 5")
    once(out, err, "Problems found: 1")
    absent(out, err, "Unreadable")
    if read_file(bad_path) != bad_before:
        fail("read-only /R changed a bad cluster")
    code, out, err = run([bad_path, "/R", "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Repairs written. Problems fixed: 1")
    once(out, err, "Free clusters: 100")
    freed = read_file(bad_path)
    if get_fat12(freed, info["base"], 5) != 0 or get_fat12(freed, info["other"], 5) != 0:
        fail("readable bad cluster was not freed", out, err)

    stuck = bytearray(before)
    set_fat12(stuck, info["base"], 101, 0xFF7)
    mirror_fat(stuck, info)
    stuck_path = os.path.join(tmp, "surf12-stuck.img")
    stuck_short = bytes(stuck)[:-512]
    write_file(stuck_path, stuck_short)
    code, out, err = run([stuck_path, "/R"])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    absent(out, err, "Bad cluster can be read")
    absent(out, err, "Unreadable")
    if read_file(stuck_path) != stuck_short:
        fail("/R cleared a cluster that still cannot be read")
    if get_fat12(read_file(stuck_path), info["base"], 101) != 0xFF7:
        fail("unreadable bad mark was removed")

    both = bytearray(before)
    set_fat12(both, info["base"], 5, 0xFF7)
    mirror_fat(both, info)
    both_path = os.path.join(tmp, "surf12-both.img")
    write_file(both_path, bytes(both)[:-512])
    code, out, err = run([both_path, "/S", "/R", "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Bad cluster can be read 5")
    once(out, err, "Unreadable free cluster 101")
    once(out, err, "Problems fixed: 2")
    once(out, err, "Free clusters: 99")
    both_fixed = read_file(both_path)
    if get_fat12(both_fixed, info["base"], 5) != 0:
        fail("combined pass left the readable bad cluster", out, err)
    if get_fat12(both_fixed, info["other"], 5) != 0:
        fail("second FAT kept the readable bad cluster", out, err)
    if get_fat12(both_fixed, info["base"], 101) != 0xFF7:
        fail("combined pass did not mark the free cluster", out, err)
    if get_fat12(both_fixed, info["other"], 101) != 0xFF7:
        fail("second FAT missed the new bad cluster", out, err)
    print("PASS surface12", flush=True)


def test_surface16(tmp):
    img, info = build_fat16()
    set_fat16(img, info["base"], 1, 0xFFFF)
    mirror_fat(img, info)
    path = os.path.join(tmp, "surf16.img")
    raw = bytes(img)
    write_file(path, raw[:-512])
    code, out, err = run([path, "/S"])
    expect_code(code, out, err, 2)
    once(out, err, "Unreadable free cluster 5001")
    once(out, err, "Problems found: 1")
    if read_file(path) != raw[:-512]:
        fail("read-only FAT16 surface scan wrote the disk")
    code, out, err = run([path, "/S", "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Problems fixed: 1")
    once(out, err, "Free clusters: 4999")
    marked = read_file(path)
    if get_fat16(marked, info["base"], 5001) != 0xFFF7:
        fail("FAT16 mark is %04X" % get_fat16(marked, info["base"], 5001), out, err)
    if get_fat16(marked, info["other"], 5001) != 0xFFF7:
        fail("second FAT16 was not marked", out, err)
    if get_fat16(marked, info["base"], 5000) != 0:
        fail("FAT16 neighbor was marked", out, err)
    code, out, err = run([path])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")
    print("PASS surface16", flush=True)


def test_surface32(tmp):
    img, info = build_fat32(65524)
    path = os.path.join(tmp, "surf32.img")
    raw = bytes(img)
    print("surface32 scan", flush=True)
    write_file(path, raw[:-512])
    code, out, err = run([path, "/S", "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Unreadable free cluster 65526")
    once(out, err, "Problems fixed: 1")
    once(out, err, "Free clusters: 65523")
    marked = read_file(path)
    if get_fat32(marked, info["base"], 65526) != 0x0FFFFFF7:
        fail("FAT32 mark is %08X" % get_fat32(marked, info["base"], 65526), out, err)
    if get_fat32(marked, info["other"], 65526) != 0x0FFFFFF7:
        fail("second FAT32 was not marked", out, err)
    if int.from_bytes(marked[512 + 488:512 + 492], "little") != 65523:
        fail("FAT32 free count was not updated", out, err)
    code, out, err = run([path])
    expect_code(code, out, err, 0)
    once(out, err, "No problems found.")

    re = bytearray(img)
    set_fat32(re, info["base"], 10, 0x0FFFFFF7)
    mirror_fat(re, info)
    put32(re, 512 + 488, 65523)
    re_path = os.path.join(tmp, "surf32-re.img")
    write_file(re_path, re)
    re_before = bytes(re)
    code, out, err = run([re_path, "/R"])
    expect_code(code, out, err, 2)
    once(out, err, "Bad cluster can be read 10")
    once(out, err, "Problems found: 1")
    absent(out, err, "Unreadable")
    if read_file(re_path) != re_before:
        fail("read-only /R changed FAT32")
    code, out, err = run([re_path, "/R", "/F"])
    expect_code(code, out, err, 1)
    once(out, err, "Free clusters: 65524")
    freed = read_file(re_path)
    if get_fat32(freed, info["base"], 10) != 0 or get_fat32(freed, info["other"], 10) != 0:
        fail("FAT32 bad cluster was not freed", out, err)
    if int.from_bytes(freed[512 + 488:512 + 492], "little") != 65524:
        fail("FAT32 free count was not restored", out, err)
    print("PASS surface32", flush=True)


def main():
    if not os.path.isfile(BIN):
        fail("missing " + BIN)
    test_cli()
    with tempfile.TemporaryDirectory(prefix="svardisk-") as tmp:
        test_not_fat(tmp)
        test_clean12(tmp)
        test_broken12(tmp)
        test_fat16(tmp)
        test_fat32(tmp)
        test_surface(tmp)
        test_surface16(tmp)
        test_surface32(tmp)
    print("PASS all", flush=True)


if __name__ == "__main__":
    main()
