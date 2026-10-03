"""
Tests for parser/tools/visualize_parse_tree.py: parsing the tree text, compaction and the output files.
"""

import json
import shutil
import subprocess
import sys
import xml.dom.minidom
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]
TOOL = ROOT / "parser" / "tools" / "visualize_parse_tree.py"
sys.path.insert(0, str(TOOL.parent))

import visualize_parse_tree as viz  # noqa: E402

TREE = """\
Program
  Func
    <FUNCTION,>
    <IDENTIFIER, main>
    Params (empty)
    Block
      Stmts
        Stmt
          Expr
            AndExpr
              <INT, 1>
        Stmts (empty)
"""


def labels(node):
    return [node.lines] + [x for c in node.children for x in labels(c)]


def test_parse_tree_builds_the_nesting():
    root = viz.parse_tree(TREE)
    assert root.lines == ["Program"]
    func = root.children[0]
    assert [c.lines for c in func.children[:3]] == [["FUNCTION"], ["IDENTIFIER main"], ["Params"]]
    assert func.children[0].kind == viz.TOKEN
    assert func.children[2].kind == viz.EMPTY
    assert func.children[3].kind == viz.NONTERMINAL
    assert viz.count(root) == 12


@pytest.mark.parametrize(
    "text, message",
    [
        ("", "empty"),
        ("Program\nProgram\n", "more than one root"),
        ("Program\n    Func\n", "too deep"),
        ("Program\n Func\n", "multiple of 2"),
    ],
)
def test_parse_tree_rejects_malformed_text(text, message):
    with pytest.raises(ValueError, match=message):
        viz.parse_tree(text)


def test_hide_empty_removes_empty_nodes():
    root = viz.hide_empty(viz.parse_tree(TREE))
    assert viz.count(root) == 10
    assert all(" (empty)" not in "".join(n.lines) and n.kind != viz.EMPTY for n in viz.walk(root))


def test_collapse_chains_merges_single_child_nonterminals():
    root = viz.collapse_chains(viz.hide_empty(viz.parse_tree(TREE)))
    merged = [n for n in viz.walk(root) if n.tooltip.count(">") >= 2]
    assert [n.lines for n in merged] == [["Block > AndExpr"]]
    assert merged[0].tooltip == "Block > Stmts > Stmt > Expr > AndExpr"
    assert merged[0].children[0].lines == ["INT 1"]
    assert viz.count(root) == 5


def laid_out(text, horizontal):
    root = viz.parse_tree(text)
    viz.measure(root)
    viz.place(root, horizontal)
    return root


def overlap(a, b):
    return not (
        a.x + a.width <= b.x or b.x + b.width <= a.x or a.y + a.height <= b.y or b.y + b.height <= a.y
    )


@pytest.mark.parametrize("horizontal", [False, True])
def test_layout_never_overlaps_any_two_boxes(horizontal):
    nodes = list(viz.walk(laid_out(TREE, horizontal)))
    for index, a in enumerate(nodes):
        for b in nodes[index + 1 :]:
            assert not overlap(a, b), (a.lines, b.lines)


def test_vertical_layout_puts_children_below_the_parent():
    root = laid_out(TREE, False)
    for node in viz.walk(root):
        for child in node.children:
            assert child.y >= node.y + node.height


def test_horizontal_layout_puts_children_right_of_the_parent():
    root = laid_out(TREE, True)
    for node in viz.walk(root):
        for child in node.children:
            assert child.x >= node.x + node.width


def test_parent_is_centred_over_its_first_and_last_child():
    root = laid_out(TREE, False)
    for node in viz.walk(root):
        if node.children:
            first, last = node.children[0], node.children[-1]
            middle = (first.x + first.width / 2 + last.x + last.width / 2) / 2
            assert node.x + node.width / 2 == pytest.approx(middle)


def test_a_shallow_subtree_tucks_in_beside_a_deep_one():
    text = "R\n  A\n    B\n      C\n        D\n  Leaf1\n  Leaf2\n"
    root = laid_out(text, False)
    deep, leaf1, leaf2 = root.children
    # the leaves sit next to the deep branch, not beyond the full width of its widest level
    assert leaf1.x < deep.x + deep.width + 4 * viz.CHAR_WIDTH
    assert leaf2.x > leaf1.x


def test_the_layout_is_far_narrower_than_giving_every_subtree_its_widest_level():
    wide = "R\n" + "".join(f"  N{i}\n    M{i}\n      K{i}\n" for i in range(6))
    root = laid_out(wide, False)
    naive = sum(c.width for c in root.children) + viz.GAP_X * 5
    right = max(n.x + n.width for n in viz.walk(root))
    assert right < naive + 2 * viz.MARGIN


def test_svg_is_well_formed_and_escapes_text():
    root = viz.parse_tree("Program\n  <STRING_LIT, \"<a&b>\">\n")
    svg = viz.to_svg(root)
    document = xml.dom.minidom.parseString(svg)
    assert document.documentElement.tagName == "svg"
    assert "&lt;a&amp;b&gt;" in svg


def run(*args, stdin=None):
    return subprocess.run(
        [sys.executable, str(TOOL), *args], input=stdin, capture_output=True, text=True
    )


def test_cli_writes_svg_from_a_tree_file(tmp_path):
    tree = tmp_path / "t.tree"
    tree.write_text(TREE, encoding="utf-8")
    out = tmp_path / "t.svg"
    proc = run(str(tree), "--compact", "-o", str(out))
    assert proc.returncode == 0
    xml.dom.minidom.parse(str(out))


def test_cli_layout_option(tmp_path):
    outputs = {}
    for layout in ("horizontal", "vertical"):
        out = tmp_path / f"{layout}.svg"
        assert run("-", "--layout", layout, "-o", str(out), stdin=TREE).returncode == 0
        outputs[layout] = out.read_text(encoding="utf-8")
    assert outputs["horizontal"] != outputs["vertical"]


def test_cli_writes_html_from_stdin(tmp_path):
    out = tmp_path / "t.html"
    proc = run("-", "-o", str(out), stdin=TREE)
    assert proc.returncode == 0
    page = out.read_text(encoding="utf-8")
    assert page.startswith("<!DOCTYPE html>")
    assert 'id="tree-data"' in page and "layoutTree" in page
    assert f"background: {viz.BACKGROUND}" in page


def page_data(page):
    start = page.index('id="tree-data">') + len('id="tree-data">')
    return json.loads(page[start : page.index("</script>", start)].replace("<\\/", "</"))


def test_html_embeds_the_whole_tree_and_the_options():
    page = viz.to_html(viz.parse_tree(TREE), "t", horizontal=True, expand=2)
    data = page_data(page)
    assert data["horizontal"] is True and data["expand"] == 2
    assert data["root"]["l"] == "Program"
    assert data["root"]["c"][0]["c"][1]["l"] == "IDENTIFIER main"
    assert data["root"]["c"][0]["c"][1]["k"] == viz.TOKEN


def test_html_cannot_be_broken_by_text_in_the_tree():
    page = viz.to_html(viz.parse_tree('Program\n  <STRING_LIT, "</script><b>">\n'), "t")
    assert page.count("</script>") == 3
    assert '"</script><b>"' in page_data(page)["root"]["c"][0]["l"]


def test_cli_expand_option(tmp_path):
    out = tmp_path / "t.html"
    assert run("-", "--expand", "1", "-o", str(out), stdin=TREE).returncode == 0
    assert page_data(out.read_text(encoding="utf-8"))["expand"] == 1


def test_background_colour_is_used_in_svg():
    assert f'fill="{viz.BACKGROUND}"' in viz.to_svg(viz.parse_tree(TREE))


LAYOUT_SCRIPT = """
const {layoutTree} = require(process.argv[1]);
const data = JSON.parse(require("fs").readFileSync(0, "utf8"));
(function label(n) { n.label = n.l; n.c.forEach(label); })(data.root);
const result = layoutTree(data.root, data.horizontal, data.constants);
console.log(JSON.stringify(result.nodes.map(n => [n.x, n.y, n.w, n.h])));
"""

needs_node = pytest.mark.skipif(shutil.which("node") is None, reason="node is not installed")


@needs_node
@pytest.mark.parametrize("horizontal", [False, True])
@pytest.mark.parametrize("compact", [False, True])
def test_javascript_layout_matches_the_python_layout(horizontal, compact):
    sample = ROOT / "tests" / "parser" / "expected" / "sample.tree"
    root = viz.parse_tree(sample.read_text(encoding="utf-8"))
    if compact:
        root = viz.collapse_chains(viz.hide_empty(root))
    payload = {"root": viz.to_data(root), "constants": viz.constants(), "horizontal": horizontal}
    proc = subprocess.run(
        ["node", "-e", LAYOUT_SCRIPT, str(TOOL.parent / "js" / "parse_tree_layout.js")],
        input=json.dumps(payload),
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0, proc.stderr
    from_js = json.loads(proc.stdout)

    viz.measure(root)
    viz.place(root, horizontal)
    from_python = [[n.x, n.y, n.width, n.height] for n in viz.walk(root)]
    assert len(from_js) == len(from_python)
    for got, want in zip(from_js, from_python):
        assert got == pytest.approx(want)


def test_cli_runs_the_compiler_for_a_source_file(compiler, tmp_path):
    source = tmp_path / "prog.oc"
    source.write_text("function main() -> i32 {\n    return 0;\n}\n", encoding="utf-8")
    out = tmp_path / "prog.svg"
    proc = run(str(source), "--octacc", str(compiler), "-o", str(out))
    assert proc.returncode == 0
    assert "IDENTIFIER" in out.read_text(encoding="utf-8")


def test_cli_reports_syntax_errors_from_the_compiler(compiler, tmp_path):
    source = tmp_path / "bad.oc"
    source.write_text("function main() -> i32 {\n", encoding="utf-8")
    proc = run(str(source), "--octacc", str(compiler), "-o", str(tmp_path / "x.svg"))
    assert proc.returncode == 1
    assert "expected '}'" in proc.stderr
    assert not (tmp_path / "x.svg").exists()


def test_cli_rejects_malformed_tree_text(tmp_path):
    proc = run("-", "-o", str(tmp_path / "x.svg"), stdin="Program\n    Func\n")
    assert proc.returncode == 1
    assert "too deep" in proc.stderr
