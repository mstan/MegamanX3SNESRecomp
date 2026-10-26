"""Graphics prerequisites for disposable stage-8 Bit/Byte approach snapshots.

Checkpoint 1 loads graphics group 4, but retail progression retains door asset
$56 from group 1. A direct checkpoint warp omits it. Let the original guest
graphics job load both groups before the door actor caches its tile/attributes.
This is a preparation assist, not validation of the skipped stage progression.
"""


def _room_doors(driver):
    pool = driver.read(0x1518, 0x300)
    return [(0x1518 + i, pool[i:i + 0x60])
            for i in range(0, len(pool), 0x60)
            if pool[i] and pool[i + 10] == 0x11
            and 0x1400 <= int.from_bytes(pool[i + 5:i + 7], 'little') < 0x1700]


def validate_door_graphics(driver):
    """Reject a missing asset or an actor that cached the old missing asset."""
    base = driver.byte(0x18256)
    attributes = driver.byte(0x18356)
    assert (base, attributes) == (0xf0, 0x2f), (
        'Door asset $56 was not loaded', base, attributes)
    doors = _room_doors(driver)
    for address, actor in doors:
        assert (actor[0x18], actor[0x11]) == (base, attributes), (
            'Door cached an invalid graphics asset', hex(address),
            actor[0x18], actor[0x11])
    return dict(asset=0x56, tile_base=base, attributes=attributes,
                doors=[hex(address) for address, _ in doors])


def prepare_door_graphics(driver):
    """Run original $00:B1FC via its idle cooperative scheduler slot $10.

    Call after checkpoint 1 loads and before moving toward the room. The writes
    to $40/$42 reproduce $00:81A3's registration of this graphics-only job.
    No actors, camera state, collision state or encounter results are written.
    """
    assert driver.byte(0x1fae) == 8 and driver.byte(0x1fb5) == 1
    assert driver.byte(0x1f18) == 4
    assert not _room_doors(driver), 'Warm graphics before the room door initializes'
    before = driver.byte(0x1fd8)
    for group in (1, 4):
        assert driver.byte(0x40) == 0, 'Native graphics job slot is busy'
        driver.byte(0x1f18, group)
        driver.word(0x42, 0xb1fc)
        driver.word(0x40, 0x0101)
        driver.until(0x40, b'\0', 1800, 1)
        driver.step(3)
    assert driver.byte(0x1fd8) == before, 'Encounter results changed during preparation'
    return validate_door_graphics(driver)
