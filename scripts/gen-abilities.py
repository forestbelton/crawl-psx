import enum
from typing import TextIO

import yaml
import pydantic

ABILITIES_YAML_PATH = "data/abilities.yml"
ABILITIES_HEADER_PATH = "source/data/abilities.h"


class AbilityFlag(enum.Enum):
    ABFLAG_NONE = "ABFLAG_NONE"
    ABFLAG_BREATH = "ABFLAG_BREATH"
    ABFLAG_DELAY = "ABFLAG_DELAY"
    ABFLAG_PAIN = "ABFLAG_PAIN"
    ABFLAG_INSTANT = "ABFLAG_INSTANT"
    ABFLAG_PERMANENT_MP = "ABFLAG_PERMANENT_MP"


class Ability(pydantic.BaseModel):
    id: str
    name: str
    mp_cost: int
    hp_cost: int
    food_cost: int
    piety_cost: int
    flag: AbilityFlag


def parse_abilities(file: TextIO) -> list[Ability]:
    raw_abilities = yaml.safe_load(file)
    if "abilities" not in raw_abilities:
        raise Exception("No abilities found in file")
    return [Ability(**raw_ability) for raw_ability in raw_abilities["abilities"]]


def generate_abilities_entry(ability: Ability) -> str:
    return f"    {{ {ability.id}, \"{ability.name}\", {ability.mp_cost}, {ability.hp_cost}, {ability.food_cost}, {ability.piety_cost}, {ability.flag.value} }},"


def generate_abilities_header(abilities: list[Ability]) -> str:
    return f"""
// GENERATED FILE, DO NOT EDIT! See scripts/gen-abilities.py
#ifndef DATA_ABILITIES_H
#define DATA_ABILITIES_H

#include "enum.h"

constexpr ability_def Ability_List[] = {{
{"\n".join(generate_abilities_entry(ability) for ability in abilities)}
}};

#endif
""".lstrip()


def main() -> None:
    with open(ABILITIES_YAML_PATH, "r") as f:
        abilities = parse_abilities(f)
    with open(ABILITIES_HEADER_PATH, "w") as f:
        f.write(generate_abilities_header(abilities))


if __name__ == "__main__":
    main()
