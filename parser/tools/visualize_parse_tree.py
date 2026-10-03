#!/usr/bin/env python3
"""
Draw the parse tree printed by `octacc --emit-parse-tree` as an SVG or HTML file.

The input is either a .oc source file, which is run through build/octacc, or a file holding the
parse tree text, or - for the tree text on standard input. The output format follows the
extension of -o: .svg writes a plain SVG, anything else writes an interactive HTML page. In the
page a click on a node expands or collapses it, the mouse pans and the wheel zooms.

Only the Python standard library is used.

Note: This file was written by Claude (Anthropic)
"""

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass, field
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_COMPILER = ROOT / "build" / ("octacc.exe" if sys.platform == "win32" else "octacc")

INDENT_WIDTH = 2
CHAR_WIDTH = 6.6
LINE_HEIGHT = 14
PAD_X = 5
PAD_Y = 3
GAP_X = 5
GAP_Y = 16
GAP_COLUMN = 18
GAP_ROW = 3
MARGIN = 12

BACKGROUND = "#eff1f5"
TOOLS_DIR = Path(__file__).resolve().parent

NONTERMINAL, TOKEN, EMPTY = "nonterminal", "token", "empty"


@dataclass
class Node:
    """
    A node of the drawn tree.

    Attributes:
        lines: The text lines inside the box.
        kind: NONTERMINAL, TOKEN or EMPTY, which decides the colour.
        children: The child nodes, in order.
        tooltip: The text shown when hovering over the box.
        width: The box width, set by measure().
        height: The box height, set by measure().
        offset: The distance of the node centre from its parent centre along the breadth axis.
        x: The left edge of the box, set by place().
        y: The top edge of the box, set by place().
    """

    lines: list
    kind: str
    children: list = field(default_factory=list)
    tooltip: str = ""
    width: float = 0.0
    height: float = 0.0
    offset: float = 0.0
    x: float = 0.0
    y: float = 0.0


def make_node(text):
    """
    Build a node from one line of the parse tree text, without its indentation.
    """
    if text.startswith("<") and text.endswith(">"):
        name, _, value = text[1:-1].partition(",")
        value = value.strip()
        return Node([f"{name} {value}" if value else name], TOKEN, tooltip=text)
    if text.endswith(" (empty)"):
        return Node([text[: -len(" (empty)")]], EMPTY, tooltip="derived the empty string")
    return Node([text], NONTERMINAL, tooltip=text)


def parse_tree(text):
    """
    Parse the indented parse tree text into a Node tree.

    Raises:
        ValueError: If the text is empty or its indentation is not a valid tree.
    """
    root = None
    stack = []
    for number, line in enumerate(text.splitlines(), start=1):
        if not line.strip():
            continue
        stripped = line.lstrip(" ")
        spaces = len(line) - len(stripped)
        if spaces % INDENT_WIDTH != 0:
            raise ValueError(f"line {number}: indentation is not a multiple of {INDENT_WIDTH}")
        depth = spaces // INDENT_WIDTH
        node = make_node(stripped.rstrip())
        if depth == 0:
            if root is not None:
                raise ValueError(f"line {number}: more than one root")
            root = node
            stack = [node]
            continue
        if depth > len(stack):
            raise ValueError(f"line {number}: indented too deep")
        del stack[depth:]
        stack[-1].children.append(node)
        stack.append(node)
    if root is None:
        raise ValueError("the tree is empty")
    return root


def hide_empty(node):
    """
    Remove every node that derived the empty string.
    """
    node.children = [hide_empty(c) for c in node.children if c.kind != EMPTY]
    return node


def collapse_chains(node):
    """
    Merge each chain of nonterminals that have a single nonterminal child into one node.

    The merged node is labelled `Top > Bottom` and lists the whole chain in its tooltip.
    """
    chain = [node.lines[0]]
    while node.kind == NONTERMINAL and len(node.children) == 1 and (
        node.children[0].kind == NONTERMINAL and node.children[0].lines[0] != node.lines[0]
    ):
        node = node.children[0]
        chain.append(node.lines[0])
    node.children = [collapse_chains(c) for c in node.children]
    if len(chain) > 1:
        node.lines = [f"{chain[0]} > {chain[-1]}"] if len(chain) > 2 else [" > ".join(chain)]
        node.tooltip = " > ".join(chain)
    return node


def count(node):
    return 1 + sum(count(c) for c in node.children)


def measure(node):
    """
    Compute the size of every box from its text.
    """
    node.width = max(len(line) for line in node.lines) * CHAR_WIDTH + 2 * PAD_X
    node.height = len(node.lines) * LINE_HEIGHT + 2 * PAD_Y
    for child in node.children:
        measure(child)


def arrange(node, breadth, gap):
    """
    Position the children of every node along the breadth axis, packing subtrees by their shape.

    This is the contour based layout of Reingold and Tilford. Each subtree is summarised by its
    contour, the leftmost and rightmost extent at every depth. Siblings are placed one after the
    other, each as close to the previous ones as the contours allow, so a narrow subtree can sit
    beside a deep one instead of reserving the width of its widest level.

    Args:
        node: The root of the subtree.
        breadth: Function giving the size of a box along the breadth axis.
        gap: The least free space between two boxes at the same depth.

    Returns:
        The contour of the subtree as a list of (low, high) pairs, one per depth, relative to the
        centre of `node`. The offset of each child from its parent is stored in `offset`.
    """
    half = breadth(node) / 2
    if not node.children:
        return [(-half, half)]
    packed = []
    shifts = []
    for child in node.children:
        contour = arrange(child, breadth, gap)
        shift = 0.0
        if packed:
            shift = max(
                packed[d][1] + gap - contour[d][0] for d in range(min(len(packed), len(contour)))
            )
        shifts.append(shift)
        for depth, (low, high) in enumerate(contour):
            if depth < len(packed):
                packed[depth] = (min(packed[depth][0], shift + low), shift + high)
            else:
                packed.append((shift + low, shift + high))
    centre = (shifts[0] + shifts[-1]) / 2
    for child, shift in zip(node.children, shifts):
        child.offset = shift - centre
    return [(-half, half)] + [(low - centre, high - centre) for low, high in packed]


def centres(node, centre, found):
    """
    Turn the offsets into the centre of every node along the breadth axis.
    """
    found.append((node, centre))
    for child in node.children:
        centres(child, centre + child.offset, found)


def depth_sizes(node):
    """
    Return the largest size of a box at each depth, as (width, height) pairs.
    """
    sizes = [(node.width, node.height)]
    for child in node.children:
        for depth, (width, height) in enumerate(depth_sizes(child), start=1):
            if depth < len(sizes):
                sizes[depth] = (max(sizes[depth][0], width), max(sizes[depth][1], height))
            else:
                sizes.append((width, height))
    return sizes


def place(root, horizontal):
    """
    Give every node its position.

    The breadth axis is x for a top to bottom tree and y for a left to right one. Along the depth
    axis every level has the room of its largest box.
    """
    breadth = (lambda n: n.height) if horizontal else (lambda n: n.width)
    arrange(root, breadth, GAP_ROW if horizontal else GAP_X)
    found = []
    centres(root, 0.0, found)
    start = min(c - breadth(n) / 2 for n, c in found)

    sizes = depth_sizes(root)
    along = [MARGIN]
    for width, height in sizes[:-1]:
        along.append(along[-1] + (width + GAP_COLUMN if horizontal else height + GAP_Y))

    levels = {}
    stack = [(root, 0)]
    while stack:
        node, depth = stack.pop()
        levels[id(node)] = depth
        stack.extend((c, depth + 1) for c in node.children)
    for node, centre in found:
        low = centre - breadth(node) / 2 - start + MARGIN
        depth = levels[id(node)]
        if horizontal:
            node.x, node.y = along[depth], low
        else:
            node.x, node.y = low, along[depth]


def walk(node):
    yield node
    for child in node.children:
        yield from walk(child)


STYLE = """
    .edge { stroke: #8a94a3; stroke-width: 1.2; fill: none; }
    .box { stroke-width: 1.2; }
    .nonterminal .box { fill: #e6efff; stroke: #4a78c2; }
    .token .box { fill: #e5f6e8; stroke: #3f9a52; }
    .empty .box { fill: #f2f2f2; stroke: #aaaaaa; stroke-dasharray: 4 3; }
    text { font-family: ui-monospace, Menlo, Consolas, monospace; font-size: 11px; fill: #1d2430; }
    .empty text { fill: #777777; font-style: italic; }
"""


def to_svg(root, horizontal=False):
    """
    Lay out the tree and return it as an SVG document.

    Args:
        root: The root of the tree.
        horizontal: True to draw from left to right, False to draw from top to bottom.
    """
    measure(root)
    place(root, horizontal)
    nodes = list(walk(root))
    width = max(n.x + n.width for n in nodes) + MARGIN
    height = max(n.y + n.height for n in nodes) + MARGIN
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width:.0f}" height="{height:.0f}" '
        f'viewBox="0 0 {width:.0f} {height:.0f}">',
        f"<style>{STYLE}</style>",
        f'<rect width="100%" height="100%" fill="{BACKGROUND}"/>',
        '<g id="tree">',
    ]
    for node in nodes:
        for child in node.children:
            if horizontal:
                x1, y1 = node.x + node.width, node.y + node.height / 2
                x2, y2 = child.x, child.y + child.height / 2
                mid = x1 + GAP_COLUMN / 2
                path = f"M{x1:.1f},{y1:.1f} H{mid:.1f} V{y2:.1f} H{x2:.1f}"
            else:
                x1, y1 = node.x + node.width / 2, node.y + node.height
                x2, y2 = child.x + child.width / 2, child.y
                mid = y1 + GAP_Y / 2
                path = f"M{x1:.1f},{y1:.1f} V{mid:.1f} H{x2:.1f} V{y2:.1f}"
            parts.append(f'<path class="edge" d="{path}"/>')
    for node in nodes:
        parts.append(f'<g class="{node.kind}"><title>{escape(node.tooltip)}</title>')
        parts.append(
            f'<rect class="box" x="{node.x:.1f}" y="{node.y:.1f}" width="{node.width:.1f}" '
            f'height="{node.height:.1f}" rx="3"/>'
        )
        for index, line in enumerate(node.lines):
            css = ""
            parts.append(
                f'<text{css} x="{node.x + node.width / 2:.1f}" '
                f'y="{node.y + PAD_Y + LINE_HEIGHT * (index + 1) - 3:.1f}" '
                f'text-anchor="middle">{escape(line)}</text>'
            )
        parts.append("</g>")
    parts.append("</g></svg>")
    return "\n".join(parts) + "\n"


PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<style>
    html, body {{ margin: 0; height: 100%; background: {background}; }}
    #view {{ width: 100%; height: 100%; overflow: hidden; cursor: grab; }}
    #view.drag {{ cursor: grabbing; }}
    #canvas {{ width: 100%; height: 100%; display: block; }}
    #bar {{ position: fixed; left: 10px; top: 10px; display: flex; gap: 6px; }}
    #bar button {{
        font: 12px sans-serif; padding: 5px 10px; color: #1d2430; background: #ffffff;
        border: 1px solid #b8c0cc; border-radius: 5px; cursor: pointer;
    }}
    #bar button:hover {{ background: #e6efff; }}
    #status {{ position: fixed; left: 10px; bottom: 8px; font: 12px sans-serif; color: #5c6370; }}
{style}
    .expandable {{ cursor: pointer; }}
    .expandable:hover .box {{ stroke-width: 2.4; }}
    .collapsed .box {{ stroke-width: 2.4; fill: #cfdcf7; }}
</style>
</head>
<body>
<div id="view"><svg id="canvas" xmlns="http://www.w3.org/2000/svg"></svg></div>
<div id="bar">
    <button id="expand-all">Expand all</button>
    <button id="collapse-all">Collapse all</button>
    <button id="fit">Fit</button>
    <button id="direction"></button>
</div>
<div id="status"></div>
<script type="application/json" id="tree-data">{data}</script>
<script>
{layout_js}
</script>
<script>
{view_js}
</script>
</body>
</html>
"""


def constants():
    """
    Return the layout constants in the form used by parse_tree_layout.js.
    """
    return {
        "charWidth": CHAR_WIDTH,
        "padX": PAD_X,
        "padY": PAD_Y,
        "lineHeight": LINE_HEIGHT,
        "gapX": GAP_X,
        "gapY": GAP_Y,
        "gapRow": GAP_ROW,
        "gapColumn": GAP_COLUMN,
        "margin": MARGIN,
    }


def to_data(node):
    """
    Convert a tree to the plain data read by the scripts: label, kind, tooltip and children.
    """
    return {
        "l": " ".join(node.lines),
        "k": node.kind,
        "t": node.tooltip,
        "c": [to_data(child) for child in node.children],
    }


def to_html(root, title, horizontal=False, expand=0):
    """
    Return the tree as a self-contained HTML page.

    Clicking a node with children collapses or expands it. The page can be panned with the mouse
    and zoomed with the wheel.

    Args:
        root: The root of the tree.
        title: The title of the page.
        horizontal: True to start drawing from left to right, False from top to bottom.
        expand: The number of levels below the root that are open when the page loads, or 0 to
            open every level.
    """
    payload = json.dumps(
        {
            "root": to_data(root),
            "constants": constants(),
            "horizontal": horizontal,
            "expand": expand,
        },
        separators=(",", ":"),
    ).replace("</", "<\\/")
    return PAGE.format(
        title=escape(title),
        background=BACKGROUND,
        style=STYLE,
        data=payload,
        layout_js=(TOOLS_DIR / "js" / "parse_tree_layout.js").read_text(encoding="utf-8"),
        view_js=(TOOLS_DIR / "js" / "parse_tree_view.js").read_text(encoding="utf-8"),
    )


def read_tree_text(source, compiler):
    """
    Return the parse tree text for a command line input.

    Raises:
        RuntimeError: If the compiler rejects the source file or cannot be run.
    """
    if source == "-":
        return sys.stdin.read()
    path = Path(source)
    if path.suffix != ".oc":
        return path.read_text(encoding="utf-8")
    try:
        proc = subprocess.run(
            [str(compiler), "--emit-parse-tree", str(path)],
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
    except OSError as error:
        raise RuntimeError(f"cannot run {compiler}: {error}. Run `make` first or pass --octacc.")
    if proc.returncode != 0:
        raise RuntimeError(proc.stderr.rstrip() or f"{compiler} exited with {proc.returncode}")
    return proc.stdout


def main(argv=None):
    sys.setrecursionlimit(20000)
    parser = argparse.ArgumentParser(
        description="Draw an OctaC parse tree as an SVG or HTML file.",
    )
    parser.add_argument("input", help="a .oc file, a file with parse tree text, or - for stdin")
    parser.add_argument(
        "-o",
        "--output",
        help="output file, .svg for a plain SVG, otherwise HTML (default: <input>.html)",
    )
    parser.add_argument(
        "--compact",
        action="store_true",
        help="leave out empty nodes and merge chains of single-child nonterminals into one node",
    )
    parser.add_argument(
        "--hide-empty", action="store_true", help="leave out nodes that derived the empty string"
    )
    parser.add_argument(
        "--layout",
        choices=("horizontal", "vertical"),
        default="vertical",
        help="draw from top to bottom (default) or from left to right, which is more compact",
    )
    parser.add_argument(
        "--expand",
        type=int,
        default=3,
        metavar="N",
        help="levels below the root that are open when an HTML page loads, 0 for all (default: 3)",
    )
    parser.add_argument(
        "--octacc", default=str(DEFAULT_COMPILER), help="the compiler used for .oc inputs"
    )
    args = parser.parse_args(argv)

    try:
        root = parse_tree(read_tree_text(args.input, args.octacc))
    except (RuntimeError, ValueError, OSError) as error:
        print(f"visualize_parse_tree: error: {error}", file=sys.stderr)
        return 1
    if args.hide_empty or args.compact:
        root = hide_empty(root)
    if args.compact:
        root = collapse_chains(root)

    default_name = "tree" if args.input == "-" else Path(args.input).stem
    output = Path(args.output) if args.output else Path(f"{default_name}.html")
    horizontal = args.layout == "horizontal"
    if output.suffix == ".svg":
        text = to_svg(root, horizontal)
    else:
        text = to_html(root, f"{default_name} parse tree", horizontal, args.expand)
    output.write_text(text, encoding="utf-8")
    print(f"wrote {output} ({count(root)} nodes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
