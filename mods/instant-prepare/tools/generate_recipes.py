#!/usr/bin/env python3
"""Generate the small recipe/GUID lookup used by the 15693-only ASI plugin."""

import argparse
import re
import xml.etree.ElementTree as ET
from contextlib import ExitStack
from pathlib import Path
from zipfile import ZipFile


GUID = re.compile(r"[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\Z")


def checked_guid(value: str) -> str:
    lowered = value.lower()
    if not GUID.fullmatch(lowered):
        raise ValueError(f"invalid GUID: {value!r}")
    return lowered


def generate(tables_pak: Path, recipe_mod_paks: tuple[Path, ...] = ()) -> str:
    with ExitStack() as stack:
        packages = [stack.enter_context(ZipFile(path)) for path in (tables_pak, *recipe_mod_paks)]
        classes = {}
        for package in packages:
            for name in package.namelist():
                if name.lower().startswith("libs/tables/item/item") and name.lower().endswith(".xml"):
                    for row in ET.fromstring(package.read(name)).iter():
                        item_id = row.get("Id", "").lower()
                        if GUID.fullmatch(item_id):
                            classes[item_id] = row

        recipe_sources = [(packages[0], "Libs/Tables/minigame/AlchemyRecipe.xml")]
        for package in packages[1:]:
            recipe_files = [name for name in package.namelist()
                            if name.lower().startswith("libs/tables/minigame/alchemyrecipe__")
                            and name.lower().endswith(".xml")]
            if not recipe_files:
                raise ValueError(f"recipe mod package has no AlchemyRecipe__*.xml: {package.filename}")
            recipe_sources.extend((package, name) for name in sorted(recipe_files))

        recipes = []
        for package, name in recipe_sources:
            root = ET.fromstring(package.read(name))
            for recipe in root.findall(".//AlchemyRecipe"):
                recipe_id = int(recipe.attrib["RecipeId"])
                products = recipe.findall("./Products/AlchemyProduct")
                best = max(products, key=lambda product: int(product.attrib["Quality"]))
                if int(best.attrib["Quality"]) != 4:
                    raise ValueError(f"recipe {recipe_id} lacks quality 4")
                product = checked_guid(best.attrib["ProductItemId"])
                product_row = classes.get(product)
                if product_row is None:
                    raise ValueError(f"recipe {recipe_id} has unknown quality-4 product")
                separate_items = product_row.get("IsDivisible", "false").lower() != "true"
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
                recipes.append((recipe_id, product, max_yield, separate_items, ingredients))

    recipes.sort()
    if len({row[0] for row in recipes}) != len(recipes):
        raise ValueError("duplicate recipe ID")
    lines = [
        "/* Generated from Tables.pak and selected recipe mod packages for build 15693. */",
        "#ifndef INSTANT_PREPARE_RECIPES_H",
        "#define INSTANT_PREPARE_RECIPES_H",
        "#include <stdint.h>",
        "typedef struct { const char* fresh; const char* dried; uint32_t quantity; } Ingredient;",
        "typedef struct { uint32_t id; const char* product; uint32_t max_yield; uint32_t separate_items; uint32_t count; Ingredient items[4]; } Recipe;",
        "static const Recipe kRecipes[] = {",
    ]
    for recipe_id, product, max_yield, separate_items, ingredients in recipes:
        parts = ", ".join(f'{{"{fresh}", "{dried}", {qty}}}' for fresh, dried, qty in ingredients)
        lines.append(f'    {{{recipe_id}, "{product}", {max_yield}, {int(separate_items)}, {len(ingredients)}, {{{parts}}}}},')
    lines += ["};", "#define RECIPE_COUNT (sizeof(kRecipes) / sizeof(kRecipes[0]))", "#endif", ""]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("tables_pak", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--recipe-mod", type=Path, action="append", default=[],
                        help="mod .pak containing AlchemyRecipe__*.xml and its item definitions")
    args = parser.parse_args()
    args.output.write_text(generate(args.tables_pak, tuple(args.recipe_mod)), encoding="ascii")
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()
