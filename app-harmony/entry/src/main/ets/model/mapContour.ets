/**
 * 栅格地图 → **平滑矢量等高线**（楼层平面图风）。把占据栅格当填充方块画是"丑的根因"
 * （亚像素碎块/锯齿）；本模块改成：提取自由/墙体**边界** → 角点平滑 → 由 MapCanvas 描边+软填充成
 * 干净的矢量墙线（Roborock / 建筑图纸观感）。
 *
 * 管线（全纯函数，便于 tools/verify 镜像测试，规避 ArkTS 本机不可编译）：
 *   1. `ParsedMap.toDisplayPool(maxLen)` 把裁剪+正方形化栅格 max-pool 成粗网格 `CoarseGrid`（降噪+降复杂度）。
 *   2. `marchingSquares` 在粗网格上提取边界线段（midpoint 版，对角自带 45° 倒角）。
 *   3. `linkLoops` 按共享端点把线段连成多段线（闭环/开线）。
 *   4. `chaikin` 角点切割平滑（把 45° 折线变顺滑曲线）。
 *   5. `extractContours` 串起 2–4 并把粗网格坐标映射回 base 画布像素（×f×格尺寸）。
 *
 * 坐标：粗网格坐标 (cx,cy) → base px = (cx·f·gridWidth, cy·f·gridHeight)，与 `forEachWallRect` 的
 *   display 朝向一致（见 `MapService.toDisplayPool` 的 y 翻转）。误差在 1 格内，肉眼不可见。
 *
 * ⚠️ 未经 DevEco 验证；纯几何已由 `tools/verify/verify.mjs` 镜像断言覆盖。
 */

/** 粗网格（max-pool 后的二值占据图，display 朝向）。 */
export interface CoarseGrid {
  b: Uint8Array; // 行优先 b[y*w + x]：1=墙 0=空
  w: number;
  h: number;
  f: number;     // 相对原裁剪栅格的降采样因子（粗格 = f×f 原格）
}

/** 一条等高线（base 画布像素坐标，扁平 [x,y,x,y,...]）。 */
export interface Contour {
  pts: number[];
  closed: boolean;
}

/**
 * Marching Squares（midpoint）：在粗网格上提取墙/空边界线段。
 * 角点位＝四邻格 TL/TR/BR/BL（bit 8/4/2/1）；线段端点取各边中点（粗格坐标，含 .5）。
 * 返回扁平线段数组：每 4 个数 = 一条线段 [x1,y1,x2,y2]。
 */
export function marchingSquares(g: CoarseGrid): number[] {
  const b: Uint8Array = g.b;
  const w: number = g.w;
  const h: number = g.h;
  const segs: number[] = [];
  for (let y = 0; y < h - 1; y++) {
    for (let x = 0; x < w - 1; x++) {
      const tl: number = b[y * w + x];
      const tr: number = b[y * w + x + 1];
      const br: number = b[(y + 1) * w + x + 1];
      const bl: number = b[(y + 1) * w + x];
      const idx: number = tl * 8 + tr * 4 + br * 2 + bl * 1;
      if (idx === 0 || idx === 15) {
        continue;
      }
      // 各边中点
      const tx: number = x + 0.5;
      const ty: number = y;
      const rx: number = x + 1;
      const ry: number = y + 0.5;
      const bx: number = x + 0.5;
      const by: number = y + 1;
      const lx: number = x;
      const ly: number = y + 0.5;
      switch (idx) {
        case 1:  segs.push(lx, ly, bx, by); break;
        case 2:  segs.push(bx, by, rx, ry); break;
        case 3:  segs.push(lx, ly, rx, ry); break;
        case 4:  segs.push(tx, ty, rx, ry); break;
        case 5:  segs.push(lx, ly, tx, ty); segs.push(bx, by, rx, ry); break; // 鞍点
        case 6:  segs.push(tx, ty, bx, by); break;
        case 7:  segs.push(lx, ly, tx, ty); break;
        case 8:  segs.push(tx, ty, lx, ly); break;
        case 9:  segs.push(tx, ty, bx, by); break;
        case 10: segs.push(tx, ty, rx, ry); segs.push(lx, ly, bx, by); break; // 鞍点
        case 11: segs.push(tx, ty, rx, ry); break;
        case 12: segs.push(lx, ly, rx, ry); break;
        case 13: segs.push(rx, ry, bx, by); break;
        case 14: segs.push(lx, ly, bx, by); break;
        default: break;
      }
    }
  }
  return segs;
}

/** 端点量化键（粗格中点都是 .5 的倍数 → ×2 取整可精确比较）。 */
function ptKey(x: number, y: number): string {
  return `${Math.round(x * 2)}_${Math.round(y * 2)}`;
}

/**
 * 把线段按共享端点连成多段线。闭环回到起点标 `closed`；触及网格边界的链为开线（双向延伸）。
 * 贪心：任取未用线段，先向前延伸到无邻接/闭合，再从起点向后延伸。
 */
export function linkLoops(segs: number[]): Contour[] {
  const n: number = Math.floor(segs.length / 4);
  const used: Uint8Array = new Uint8Array(n);
  // 端点 → 关联线段索引表
  const adj: Map<string, number[]> = new Map<string, number[]>();
  for (let s = 0; s < n; s++) {
    const ka: string = ptKey(segs[s * 4], segs[s * 4 + 1]);
    const kb: string = ptKey(segs[s * 4 + 2], segs[s * 4 + 3]);
    const la: number[] | undefined = adj.get(ka);
    if (la === undefined) {
      adj.set(ka, [s]);
    } else {
      la.push(s);
    }
    const lb: number[] | undefined = adj.get(kb);
    if (lb === undefined) {
      adj.set(kb, [s]);
    } else {
      lb.push(s);
    }
  }
  // 从端点 (cx,cy) 找一条未用的关联线段，返回其"另一端"坐标并标记已用；无则返回 null。
  const step = (cx: number, cy: number): number[] | null => {
    const cand: number[] | undefined = adj.get(ptKey(cx, cy));
    if (cand === undefined) {
      return null;
    }
    for (let i = 0; i < cand.length; i++) {
      const t: number = cand[i];
      if (used[t] === 1) {
        continue;
      }
      used[t] = 1;
      const px: number = segs[t * 4];
      const py: number = segs[t * 4 + 1];
      const qx: number = segs[t * 4 + 2];
      const qy: number = segs[t * 4 + 3];
      if (ptKey(px, py) === ptKey(cx, cy)) {
        return [qx, qy];
      }
      return [px, py];
    }
    return null;
  };

  const out: Contour[] = [];
  for (let s = 0; s < n; s++) {
    if (used[s] === 1) {
      continue;
    }
    used[s] = 1;
    const ax: number = segs[s * 4];
    const ay: number = segs[s * 4 + 1];
    const pts: number[] = [ax, ay, segs[s * 4 + 2], segs[s * 4 + 3]];
    // 向前延伸
    let cx: number = pts[2];
    let cy: number = pts[3];
    let closed: boolean = false;
    while (true) {
      const nxt: number[] | null = step(cx, cy);
      if (nxt === null) {
        break;
      }
      cx = nxt[0];
      cy = nxt[1];
      pts.push(cx, cy);
      if (ptKey(cx, cy) === ptKey(ax, ay)) {
        closed = true;
        break;
      }
    }
    // 未闭合 → 从起点向后延伸，结果反序拼到前面
    if (!closed) {
      const back: number[] = [];
      let bx2: number = ax;
      let by2: number = ay;
      while (true) {
        const nxt: number[] | null = step(bx2, by2);
        if (nxt === null) {
          break;
        }
        bx2 = nxt[0];
        by2 = nxt[1];
        back.push(bx2, by2);
      }
      if (back.length > 0) {
        const head: number[] = [];
        for (let i = back.length - 2; i >= 0; i -= 2) {
          head.push(back[i], back[i + 1]);
        }
        for (let i = 0; i < pts.length; i++) {
          head.push(pts[i]);
        }
        out.push({ pts: head, closed: false });
        continue;
      }
    }
    out.push({ pts, closed });
  }
  return out;
}

/**
 * Chaikin 角点切割平滑：每条边用 1/4、3/4 两点替换，迭代数次把折线变顺滑曲线。
 * 闭环首尾相连切割；开线保留两端点。
 */
export function chaikin(pts: number[], closed: boolean, iters: number): number[] {
  let cur: number[] = pts;
  for (let it = 0; it < iters; it++) {
    const m: number = Math.floor(cur.length / 2);
    if (m < 3) {
      break;
    }
    const next: number[] = [];
    if (!closed) {
      next.push(cur[0], cur[1]);
    }
    const segCount: number = closed ? m : m - 1;
    for (let i = 0; i < segCount; i++) {
      const i1: number = (i + 1) % m;
      const x0: number = cur[i * 2];
      const y0: number = cur[i * 2 + 1];
      const x1: number = cur[i1 * 2];
      const y1: number = cur[i1 * 2 + 1];
      next.push(x0 * 0.75 + x1 * 0.25, y0 * 0.75 + y1 * 0.25);
      next.push(x0 * 0.25 + x1 * 0.75, y0 * 0.25 + y1 * 0.75);
    }
    if (!closed) {
      next.push(cur[(m - 1) * 2], cur[(m - 1) * 2 + 1]);
    }
    cur = next;
  }
  return cur;
}

/**
 * 高层：粗网格 → 平滑等高线（base 画布像素坐标）。
 * @param g            ParsedMap.toDisplayPool 产出的粗网格
 * @param gridWidth    每原格像素宽（ParsedMap.gridWidth）
 * @param gridHeight   每原格像素高（ParsedMap.gridHeight）
 * @param chaikinIters Chaikin 迭代次数（2 较顺滑；0 = 仅 marching squares 折线）
 */
export function extractContours(g: CoarseGrid, gridWidth: number, gridHeight: number, chaikinIters: number): Contour[] {
  const loops: Contour[] = linkLoops(marchingSquares(g));
  const out: Contour[] = [];
  for (let k = 0; k < loops.length; k++) {
    const c: Contour = loops[k];
    const sm: number[] = chaikin(c.pts, c.closed, chaikinIters);
    const mapped: number[] = new Array<number>(sm.length);
    const sx: number = g.f * gridWidth;
    const sy: number = g.f * gridHeight;
    for (let i = 0; i < sm.length; i += 2) {
      mapped[i] = sm[i] * sx;
      mapped[i + 1] = sm[i + 1] * sy;
    }
    out.push({ pts: mapped, closed: c.closed });
  }
  return out;
}
