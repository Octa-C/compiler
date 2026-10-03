/**
 * Layout of a parse tree: the contour based algorithm of Reingold and Tilford.
 *
 * A node is { label, c: [children], collapsed }. A collapsed node is laid out as a leaf. The same
 * algorithm is implemented in visualize_parse_tree.py, and the tests check that both agree.
 *
 * The constants object k holds: charWidth, padX, padY, lineHeight, gapX, gapY, gapRow, gapColumn
 * and margin.
 *
 * A contour is a list with one [low, high] pair per depth: the leftmost and rightmost extent of a
 * subtree at that depth, measured from the centre of the subtree's root.
 *
 * Note: This file was written by Claude (Anthropic)
 */

/**
 * Returns the children of a node that are drawn.
 *
 * @param {object} node The node.
 * @return {object[]} No children when the node is collapsed, otherwise all of them.
 */
function visibleChildren(node) {
    return node.collapsed ? [] : node.c;
}

/**
 * Returns the size of a box along the breadth axis, the axis siblings are spread along.
 *
 * @param {object} node The node, already measured.
 * @param {boolean} horizontal true when the tree is drawn left to right.
 * @return {number} The box height for a left to right tree, otherwise the box width.
 */
function breadthOf(node, horizontal) {
    return horizontal ? node.h : node.w;
}

/**
 * Sets the width and height of every visible box from its label.
 *
 * @param {object} node The root of the subtree.
 * @param {object} k The layout constants.
 */
function measureTree(node, k) {
    node.w = node.label.length * k.charWidth + 2 * k.padX;
    node.h = k.lineHeight + 2 * k.padY;
    for (const child of visibleChildren(node)) {
        measureTree(child, k);
    }
}

/**
 * Finds how far a subtree has to move to sit beside the siblings placed before it.
 *
 * @param {number[][]} packed The merged contour of the siblings placed so far.
 * @param {number[][]} contour The contour of the subtree being placed.
 * @param {number} gap The least free space between boxes at the same depth.
 * @return {number} The shift to apply to the subtree, 0 for the first sibling.
 */
function shiftBeside(packed, contour, gap) {
    if (packed.length === 0) {
        return 0;
    }
    let shift = -Infinity;
    const shared = Math.min(packed.length, contour.length);
    for (let depth = 0; depth < shared; depth++) {
        shift = Math.max(shift, packed[depth][1] + gap - contour[depth][0]);
    }
    return shift;
}

/**
 * Adds a shifted subtree contour to the merged contour of its siblings.
 *
 * @param {number[][]} packed The merged contour, changed in place.
 * @param {number[][]} contour The contour of the subtree.
 * @param {number} shift How far the subtree was moved.
 */
function mergeContour(packed, contour, shift) {
    for (let depth = 0; depth < contour.length; depth++) {
        const low = shift + contour[depth][0];
        const high = shift + contour[depth][1];
        if (depth < packed.length) {
            packed[depth] = [Math.min(packed[depth][0], low), high];
        } else {
            packed.push([low, high]);
        }
    }
}

/**
 * Positions the children of every node along the breadth axis.
 *
 * Each child gets an offset from its parent's centre. Siblings are placed one after the other,
 * each as close to the previous ones as the contours allow, so a narrow subtree can sit beside a
 * deep one. The parent is centred over its first and last child.
 *
 * @param {object} node The root of the subtree, already measured.
 * @param {boolean} horizontal true when the tree is drawn left to right.
 * @param {object} k The layout constants.
 * @return {number[][]} The contour of the subtree, relative to the centre of node.
 */
function arrangeSubtree(node, horizontal, k) {
    const half = breadthOf(node, horizontal) / 2;
    const children = visibleChildren(node);
    if (children.length === 0) {
        return [[-half, half]];
    }

    const gap = horizontal ? k.gapRow : k.gapX;  // the least free space between siblings
    const packed = [];
    const shifts = [];
    for (const child of children) {
        const contour = arrangeSubtree(child, horizontal, k);
        const shift = shiftBeside(packed, contour, gap);
        shifts.push(shift);
        mergeContour(packed, contour, shift);
    }

    const centre = (shifts[0] + shifts[shifts.length - 1]) / 2;
    for (let i = 0; i < children.length; i++) {
        children[i].offset = shifts[i] - centre;
    }

    const contour = [[-half, half]];
    for (const [low, high] of packed) {
        contour.push([low - centre, high - centre]);
    }
    return contour;
}

/**
 * Lists the visible nodes in depth first order, with their centre and depth.
 *
 * @param {object} node The root of the subtree.
 * @param {number} centre The centre of node along the breadth axis.
 * @param {number} depth The depth of node.
 * @param {object[]} found The list that receives { node, centre, depth } entries.
 */
function collectNodes(node, centre, depth, found) {
    found.push({ node: node, centre: centre, depth: depth });
    for (const child of visibleChildren(node)) {
        collectNodes(child, centre + child.offset, depth + 1, found);
    }
}

/**
 * Finds the largest box at each depth.
 *
 * @param {object[]} found The entries made by collectNodes.
 * @return {number[][]} One [width, height] pair per depth.
 */
function largestBoxPerDepth(found) {
    const sizes = [];
    for (const entry of found) {
        const width = entry.node.w;
        const height = entry.node.h;
        if (entry.depth < sizes.length) {
            sizes[entry.depth] = [
                Math.max(sizes[entry.depth][0], width),
                Math.max(sizes[entry.depth][1], height),
            ];
        } else {
            sizes.push([width, height]);
        }
    }
    return sizes;
}

/**
 * Finds the position of every depth along the depth axis.
 *
 * @param {number[][]} sizes The largest [width, height] at each depth.
 * @param {boolean} horizontal true when the tree is drawn left to right.
 * @param {object} k The layout constants.
 * @return {number[]} The start of each depth: the x of a column, or the y of a row.
 */
function depthStarts(sizes, horizontal, k) {
    const starts = [k.margin];
    for (let depth = 0; depth < sizes.length - 1; depth++) {
        const step = horizontal ? sizes[depth][0] + k.gapColumn : sizes[depth][1] + k.gapY;
        starts.push(starts[depth] + step);
    }
    return starts;
}

/**
 * Lays out the visible part of a tree.
 *
 * Every visible node gets x, y, w and h.
 *
 * @param {object} root The root node.
 * @param {boolean} horizontal true to draw left to right, false to draw top to bottom.
 * @param {object} k The layout constants.
 * @return {{nodes: object[], width: number, height: number}} The visible nodes in depth first
 *     order, and the size of the drawing.
 */
function layoutTree(root, horizontal, k) {
    measureTree(root, k);
    arrangeSubtree(root, horizontal, k);

    const found = [];
    collectNodes(root, 0, 0, found);
    const starts = depthStarts(largestBoxPerDepth(found), horizontal, k);

    let lowest = Infinity;
    for (const entry of found) {
        lowest = Math.min(lowest, entry.centre - breadthOf(entry.node, horizontal) / 2);
    }

    let right = 0;
    let bottom = 0;
    for (const entry of found) {
        const node = entry.node;
        const breadthStart = entry.centre - breadthOf(node, horizontal) / 2 - lowest + k.margin;
        node.x = horizontal ? starts[entry.depth] : breadthStart;
        node.y = horizontal ? breadthStart : starts[entry.depth];
        right = Math.max(right, node.x + node.w);
        bottom = Math.max(bottom, node.y + node.h);
    }

    return {
        nodes: found.map((entry) => entry.node),
        width: right + k.margin,
        height: bottom + k.margin,
    };
}

if (typeof module !== "undefined") {
    module.exports = { layoutTree: layoutTree };
}
