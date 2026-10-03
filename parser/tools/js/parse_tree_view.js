/**
 * The interactive view of a parse tree: draws the visible nodes, expands and collapses a node on
 * click, and pans and zooms the drawing. layoutTree comes from parse_tree_layout.js.
 *
 * A node is { l, k, t, c, collapsed }: the label, the kind, the tooltip, the children and whether
 * the children are hidden. setup() adds id, depth, size and label to every node.
 *
 * Note: This file was written by Claude (Anthropic)
 */

// ---------------------------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------------------------

const treeData = JSON.parse(document.getElementById("tree-data").textContent);
const constants = treeData.constants;
const rootNode = treeData.root;

const viewportElement = document.getElementById("view");
const canvasElement = document.getElementById("canvas");
const statusElement = document.getElementById("status");
const directionButton = document.getElementById("direction");

const allNodes = [];        // every node of the tree, indexed by node.id
let horizontal = treeData.horizontal;
let currentLayout = null;   // the result of the last layoutTree call
let viewBox = { x: 0, y: 0, w: 1, h: 1 };
let press = null;           // the mouse press in progress, or null

const PAN_THRESHOLD = 4;    // pixels the mouse has to move before a press becomes a drag
const ZOOM_STEP = 0.85;

// ---------------------------------------------------------------------------------------------
// The node list
// ---------------------------------------------------------------------------------------------

/**
 * Gives every node an id, a depth and the size of its subtree, and lists them in allNodes.
 *
 * @param {object} node The root of the subtree.
 * @param {number} depth The depth of node.
 */
function indexTree(node, depth) {
    node.depth = depth;
    node.id = allNodes.length;
    allNodes.push(node);

    node.size = 1;
    for (const child of node.c) {
        indexTree(child, depth + 1);
        node.size += child.size;
    }
}

/**
 * Opens the first levels of the tree and collapses the rest.
 *
 * @param {number} levels How many levels below the root are open, or 0 to open all of them.
 */
function expandTo(levels) {
    for (const node of allNodes) {
        node.collapsed = node.c.length > 0 && levels > 0 && node.depth >= levels;
    }
}

/**
 * Sets the text shown in every box. A collapsed node also shows how many nodes it hides.
 */
function updateLabels() {
    for (const node of allNodes) {
        node.label = node.collapsed ? node.l + " +" + (node.size - 1) : node.l;
    }
}

// ---------------------------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------------------------

/**
 * Escapes text for use inside SVG markup.
 *
 * @param {string} text The text.
 * @return {string} The text with &, < and > replaced.
 */
function escapeText(text) {
    return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

/**
 * Builds the SVG path of the edge from a parent to one of its children.
 *
 * @param {object} parent The parent node, already laid out.
 * @param {object} child The child node, already laid out.
 * @return {string} The path data: right angle segments from the parent to the child.
 */
function edgePath(parent, child) {
    if (horizontal) {
        const startX = parent.x + parent.w;
        const startY = parent.y + parent.h / 2;
        const bendX = startX + constants.gapColumn / 2;
        return "M" + startX + "," + startY + " H" + bendX + " V" + (child.y + child.h / 2) +
            " H" + child.x;
    }
    const startX = parent.x + parent.w / 2;
    const startY = parent.y + parent.h;
    const bendY = startY + constants.gapY / 2;
    return "M" + startX + "," + startY + " V" + bendY + " H" + (child.x + child.w / 2) +
        " V" + child.y;
}

/**
 * Builds the SVG markup of one box: its outline and its text.
 *
 * @param {object} node The node, already laid out.
 * @return {string} A <g> element. Nodes with children get the class "expandable".
 */
function nodeMarkup(node) {
    let classes = "node " + node.k;
    if (node.c.length > 0) {
        classes += node.collapsed ? " collapsed expandable" : " expandable";
    }
    const textX = node.x + node.w / 2;
    const textY = node.y + constants.padY + constants.lineHeight - 3;
    return '<g class="' + classes + '" data-id="' + node.id + '">' +
        "<title>" + escapeText(node.t) + "</title>" +
        '<rect class="box" x="' + node.x + '" y="' + node.y + '" width="' + node.w +
        '" height="' + node.h + '" rx="3"/>' +
        '<text x="' + textX + '" y="' + textY + '" text-anchor="middle">' +
        escapeText(node.label) + "</text></g>";
}

/**
 * Lays out the tree again and redraws it.
 */
function render() {
    updateLabels();
    currentLayout = layoutTree(rootNode, horizontal, constants);

    const parts = [];
    for (const node of currentLayout.nodes) {
        if (!node.collapsed) {
            for (const child of node.c) {
                parts.push('<path class="edge" d="' + edgePath(node, child) + '"/>');
            }
        }
    }
    for (const node of currentLayout.nodes) {
        parts.push(nodeMarkup(node));
    }
    canvasElement.innerHTML = parts.join("");

    statusElement.textContent =
        currentLayout.nodes.length + " of " + allNodes.length + " nodes shown";
}

/**
 * Updates the label of the layout button to name the direction a click switches to.
 */
function showDirection() {
    directionButton.textContent = horizontal ? "Top to bottom" : "Left to right";
}

// ---------------------------------------------------------------------------------------------
// The visible part of the drawing
// ---------------------------------------------------------------------------------------------

/**
 * Applies viewBox to the SVG element.
 */
function applyViewBox() {
    canvasElement.setAttribute(
        "viewBox", viewBox.x + " " + viewBox.y + " " + viewBox.w + " " + viewBox.h);
}

/**
 * Shows the whole drawing, centred, with the proportions of the window.
 */
function fitToWindow() {
    const windowRatio = viewportElement.clientWidth / viewportElement.clientHeight;
    const width = currentLayout.width;
    const height = currentLayout.height;

    viewBox = { x: 0, y: 0, w: width, h: height };
    if (width / height < windowRatio) {
        viewBox.w = height * windowRatio;
        viewBox.x = (width - viewBox.w) / 2;
    } else {
        viewBox.h = width / windowRatio;
        viewBox.y = (height - viewBox.h) / 2;
    }
    applyViewBox();
}

/**
 * Zooms around a point of the window, which stays where it is on the screen.
 *
 * @param {number} clientX The x of the point, in window pixels.
 * @param {number} clientY The y of the point, in window pixels.
 * @param {number} factor The change of the visible area: below 1 zooms in, above 1 zooms out.
 */
function zoomAt(clientX, clientY, factor) {
    const rect = canvasElement.getBoundingClientRect();
    const fractionX = (clientX - rect.left) / rect.width;
    const fractionY = (clientY - rect.top) / rect.height;

    viewBox.x += viewBox.w * fractionX * (1 - factor);
    viewBox.y += viewBox.h * fractionY * (1 - factor);
    viewBox.w *= factor;
    viewBox.h *= factor;
    applyViewBox();
}

/**
 * Moves the visible area to follow the mouse.
 *
 * @param {number} deltaX The mouse movement in x, in window pixels.
 * @param {number} deltaY The mouse movement in y, in window pixels.
 */
function panBy(deltaX, deltaY) {
    const rect = canvasElement.getBoundingClientRect();
    viewBox.x -= deltaX * viewBox.w / rect.width;
    viewBox.y -= deltaY * viewBox.h / rect.height;
    applyViewBox();
}

// ---------------------------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------------------------

/**
 * Collapses or expands a node, keeping its centre where it was on the screen.
 *
 * @param {object} node The node to toggle.
 */
function toggleNode(node) {
    const beforeX = node.x + node.w / 2;
    const beforeY = node.y + node.h / 2;

    node.collapsed = !node.collapsed;
    render();

    viewBox.x += node.x + node.w / 2 - beforeX;
    viewBox.y += node.y + node.h / 2 - beforeY;
    applyViewBox();
}

/**
 * Opens or collapses the levels of the tree, redraws it and fits it to the window.
 *
 * @param {number} levels How many levels below the root are open, or 0 to open all of them.
 */
function showLevels(levels) {
    expandTo(levels);
    render();
    fitToWindow();
}

/**
 * Switches between the top to bottom and the left to right layout.
 */
function switchDirection() {
    horizontal = !horizontal;
    showDirection();
    render();
    fitToWindow();
}

// ---------------------------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------------------------

/**
 * Zooms with the wheel.
 *
 * @param {WheelEvent} event The event.
 */
function onWheel(event) {
    event.preventDefault();
    zoomAt(event.clientX, event.clientY, event.deltaY < 0 ? ZOOM_STEP : 1 / ZOOM_STEP);
}

/**
 * Starts a press. It becomes a drag if the mouse moves, and a click if it does not.
 *
 * @param {MouseEvent} event The event.
 */
function onMouseDown(event) {
    press = { startX: event.clientX, startY: event.clientY, last: event, moved: false };
    viewportElement.classList.add("drag");
}

/**
 * Pans the drawing while the mouse is dragged.
 *
 * @param {MouseEvent} event The event.
 */
function onMouseMove(event) {
    if (press === null) {
        return;
    }
    const distance = Math.abs(event.clientX - press.startX) + Math.abs(event.clientY - press.startY);
    if (distance > PAN_THRESHOLD) {
        press.moved = true;
    }
    if (!press.moved) {
        return;
    }
    panBy(event.clientX - press.last.clientX, event.clientY - press.last.clientY);
    press.last = event;
}

/**
 * Ends a press. A press that did not move is a click, and toggles the node under the mouse.
 *
 * @param {MouseEvent} event The event.
 */
function onMouseUp(event) {
    viewportElement.classList.remove("drag");
    if (press !== null && !press.moved) {
        const target = event.target.closest ? event.target.closest(".expandable") : null;
        if (target !== null) {
            toggleNode(allNodes[Number(target.getAttribute("data-id"))]);
        }
    }
    press = null;
}

// ---------------------------------------------------------------------------------------------
// Start
// ---------------------------------------------------------------------------------------------

/**
 * Connects the buttons and the mouse to the actions, and draws the tree for the first time.
 */
function setup() {
    indexTree(rootNode, 0);

    document.getElementById("expand-all").addEventListener("click", () => showLevels(0));
    document.getElementById("collapse-all").addEventListener("click", () => showLevels(1));
    document.getElementById("fit").addEventListener("click", fitToWindow);
    directionButton.addEventListener("click", switchDirection);

    viewportElement.addEventListener("wheel", onWheel, { passive: false });
    viewportElement.addEventListener("mousedown", onMouseDown);
    window.addEventListener("mousemove", onMouseMove);
    window.addEventListener("mouseup", onMouseUp);
    window.addEventListener("resize", fitToWindow);

    showDirection();
    expandTo(treeData.expand);
    render();
    fitToWindow();
}

setup();
