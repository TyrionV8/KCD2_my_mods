#!/usr/bin/env python3
"""Generate the small recipe/GUID lookup used by the 15693-only ASI plugin."""

import argparse
import re
import xml.etree.ElementTree as ET
from pathlib import Path
from zipfile import ZipFile


GUID = re.compile(r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\Z")


def checked_guid(value: str) -> str:
    lowered = value.lower()
    if not GUID.fullmatch(lowered):
        raise ValueError(f"invalid GUID: {value!r}")
    return lowered


def generate(tables_pak: Path) -> str:
    with ZipFile(tables_pak) as package:
        classes = {}
        for name in package.namelist():
            if name.startswith("Libs/Tables/item/item") and name.endswith(".xml"):
                for row in ET.fromstring(package.read(name)).iter():
                    if row.get("Id"):
                        classes[checked_guid(row.attrib["Id"])] = row

        root = ET.fromstring(package.read("Libs/Tables/minigame/AlchemyRecipe.xml"))
        recipes = []
        for recipe in root.findall(".//AlchemyRecipe"):
            recipe_id = int(recipe.attrib["RecipeId"])
            products = recipe.findall("./Products/AlchemyProduct")
            best = max(products, key=lambda product: int(product.attrib["Quality"]))
            if int(best.attrib["Quality"]) != 4:
                raise ValueError(f"recipe {recipe_id} lacks quality 4")
            max_yield = int(best.attrib["MaxYield"])
            if not 1 <= max_yield <= 255:
                raise ValueError(f"recipe {recipe_id} has invalid quality-4 yield")
            ingredients = []
            for item in recipe.findall("./Ingredients/AlchemyIngredient"):
                fresh = checked_guid(item.attrib["IngredientItemId"])
                row = classes.get(fresh)
                dried = row.get("DriedItemId") if row is not None and row.tag == "Herb" else None
                if dried and dried != "0":
                    dried = checked_guid(dried)
                else:
                    dried = ""
                quantity = int(item.attrib["Quantity"])
                if quantity < 1:
                    raise ValueError(f"recipe {recipe_id} has invalid ingredient quantity")
                ingredients.append((fresh, dried, quantity))
            if len(ingredients) > 4 or not ingredients:
                raise ValueError(f"recipe {recipe_id} has unsupported ingredient count")
            recipes.append((recipe_id, checked_guid(best.attrib["ProductItemId"]), max_yield, ingredients))

    recipes.sort()
    if len({row[0] for row in recipes}) != len(recipes):
        raise ValueError("duplicate recipe ID")
    lines = [
        "/* Generated from this game's Tables.pak; regenerate only for build 15693. */",
        "#ifndef INSTANT_PREPARE_RECIPES_H",
        "#define INSTANT_PREPARE_RECIPES_H",
        "#include <stdint.h>",
        "typedef struct { const char* fresh; const char* dried; uint32_t quantity; } Ingredient;",
        "typedef struct { uint32_t id; const char* product; uint32_t max_yield; uint32_t count; Ingredient items[4]; } Recipe;",
        "static const Recipe kRecipes[] = {",
    ]
    for recipe_id, product, max_yield, ingredients in recipes:
        parts = ", ".join(f'{{"{fresh}", "{dried}", {qty}}}' for fresh, dried, qty in ingredients)
        lines.append(f'    {{{recipe_id}, "{product}", {max_yield}, {len(ingredients)}, {{{parts}}}}},')
    lines += ["};", "#define RECIPE_COUNT (sizeof(kRecipes) / sizeof(kRecipes[0]))", "#endif", ""]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("tables_pak", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(generate(args.tables_pak), encoding="ascii")
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
