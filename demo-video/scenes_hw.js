/* scenes_hw.js —— 项目背景之后插入的「整车硬件」章（图片素材源自 assets/，参数核对《产品说明书》§2.7 + 小车结构图）。
   hw-overview：真机全貌照(car_full.jpg) 作主视觉 + 全栈自研叙述 + 关键规格胶囊；
   hw-explode ：把整车「自顶向下」拆成六层——左侧分层爆炸堆叠(随真机部件缩略图)，右侧逐层焦点卡(部件图 + 规格 + 文字说明)。
   六层 16 个部件与 assets/01_*~16_*.png 一一对应；透明底 PNG 直接合成到暖纸背景。
   依赖 window.FILM（引擎已先加载）+ window.FX（scenes_fx 文件名排序在前，boot 时可用）。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。经 FILM.addScene 注册，片序在 scenes7.js setOrder。 */
(function () {
  "use strict";
  const F = window.FILM;
  const caption = F.caption, addCSS = F.addCSS, STAGE_W = F.STAGE_W, STAGE_H = F.STAGE_H;
  const A = 'assets/';

  /* ───────────────── 六层硬件数据（与 16 张部件图、结构图、说明书逐项核对）───────────────── */
  const LAYERS = [
    { num: '①', name: '顶部感知层', eye: 'TOP · PERCEPTION', thumb: '01_camera.png',
      parts: [{ img: '01_camera.png', name: '高清摄像头' }],
      chips: ['HD 1080P', 'USB 接口'],
      desc: '装在车体最高点、俯视采集工业仪表画面——视觉链路的图像输入源，交由香橙派实时识别。' },
    { num: '②', name: '边缘计算层', eye: 'EDGE · COMPUTE', thumb: '03_orange_pi_ai_pro.png',
      parts: [{ img: '03_orange_pi_ai_pro.png', name: '香橙派 AI Pro' }, { img: '02_gray_top_shell.png', name: '灰色舱体 · 散热' }],
      chips: ['昇腾 310B NPU', '8 核 64 位', '8–16GB LPDDR4', '32GB eMMC'],
      desc: '边缘 AI 算力中枢：在本机跑视觉推理 + DeepSeek 端侧大模型；灰色舱体集成散热与安装结构。' },
    { num: '③', name: '感知与外壳层', eye: 'LIDAR · SHELL', thumb: '04_lidar_module.png',
      parts: [{ img: '04_lidar_module.png', name: '激光雷达' }, { img: '05_white_upper_cover.png', name: '白色上盖' }],
      chips: ['LDS-50C-2 / RPLIDAR A2M8', '360° 二维测距'],
      desc: 'SLAM 的"眼睛"：360° 旋转扫描周围轮廓，为建图与定位提供感知输入；白色上盖兼作外部防护。' },
    { num: '④', name: '主控与电源层', eye: 'CONTROL · POWER', thumb: '06_main_power_assembly.png',
      parts: [{ img: '07_purple_pi_oh_pro.png', name: '紫派主板' }, { img: '08_custom_control_board.png', name: '自定义控制板' }, { img: '09_12v_converter.png', name: '12V 变压器' }, { img: '10_5v_converter.png', name: '5V 变压器' }, { img: '11_cable_harness.png', name: '线束' }],
      chips: ['Purple Pi OH Pro · RK3566', 'OpenHarmony 5.0', '85×56 mm', '12V / 5V 稳压'],
      desc: '系统中枢：紫派跑 OpenHarmony 导航四进程，自定义板做接口扩展(连电机/雷达/IO)，变压器分配 12V/5V 稳压供电，线束串联各模块。' },
    { num: '⑤', name: '驱动执行层', eye: 'DRIVE · ACTUATION', thumb: '13_dual_wheel_motors.png',
      parts: [{ img: '12_motor_driver_board.png', name: '电机驱动板' }, { img: '13_dual_wheel_motors.png', name: '双轮驱动电机' }, { img: '14_wheel.png', name: '橡胶轮组' }, { img: '15_caster_wheel.png', name: '万向轮' }],
      chips: ['TB6612FNG 驱动', 'ZWMD032032 行星减速电机', '差速底盘 + 万向轮'],
      desc: '两轮差速底盘执行运动并反馈里程计；橡胶轮防滑耐磨，前置万向轮支撑、提升机动性。' },
    { num: '⑥', name: '底盘支撑层', eye: 'CHASSIS · BASE', thumb: '16_black_base.png',
      parts: [{ img: '16_black_base.png', name: '黑色底盘' }],
      chips: ['结构支撑与安装', '集成走线空间'],
      desc: '承托上方全部模块、集成走线空间——整车的物理骨架，与上面五层一道构成完整的车体。' }
  ];

  addCSS(
    /* ── 通用 ── */
    '.hw-kick{font-family:var(--font-mono);font-size:22px;font-weight:600;letter-spacing:.3em;color:var(--text-secondary);opacity:0;}' +
    /* ── 概览 hw-overview ── */
    '.hw-ov{position:absolute;inset:0;}' +
    '.hw-photo{position:absolute;left:120px;top:150px;width:880px;height:660px;border-radius:30px;overflow:hidden;background:#1c1d17;box-shadow:0 50px 110px -34px rgba(28,30,22,.62),0 10px 26px -12px rgba(0,0,0,.4);opacity:0;}' +
    '.hw-photo img{width:100%;height:100%;object-fit:cover;display:block;}' +
    '.hw-glow{position:absolute;left:60px;top:120px;width:1000px;height:760px;border-radius:50%;background:radial-gradient(closest-side,rgba(72,92,17,.16),rgba(72,92,17,0) 70%);filter:blur(8px);opacity:0;}' +
    '.hw-ovx{position:absolute;left:1090px;top:228px;width:730px;}' +
    '.hw-ovt{font-size:64px;font-weight:800;color:var(--text-title);letter-spacing:-.03em;line-height:1.1;margin-top:18px;opacity:0;}' +
    '.hw-ovt b{color:var(--primary);}' +
    '.hw-ovs{font-size:25px;font-weight:600;color:var(--text-secondary);line-height:1.5;margin-top:22px;opacity:0;}' +
    '.hw-stats{display:flex;flex-wrap:wrap;gap:16px;margin-top:34px;}' +
    '.hw-stat{opacity:0;background:var(--surface);border:1.5px solid var(--primary);border-radius:16px;padding:16px 22px;box-shadow:var(--soft-shadow);}' +
    '.hw-stat .sv{font-size:30px;font-weight:800;color:var(--primary);letter-spacing:-.01em;}' +
    '.hw-stat .sl{font-size:17px;font-weight:600;color:var(--text-secondary);margin-top:4px;}' +
    '.hw-note{opacity:0;margin-top:34px;background:#fbf6e4;border:1px dashed #d8c98a;border-radius:14px;padding:18px 22px;font-size:21px;font-weight:600;color:var(--text-body);line-height:1.5;}' +
    '.hw-note b{color:var(--primary);}' +
    /* ── 爆炸图 hw-explode ── */
    '.hw-ex{position:absolute;inset:0;}' +
    '.hw-extitle{position:absolute;left:80px;top:96px;opacity:0;}' +
    '.hw-extitle .t{font-size:46px;font-weight:800;color:var(--text-title);letter-spacing:-.025em;}' +
    '.hw-extitle .t b{color:var(--primary);}' +
    '.hw-spine{position:absolute;border-left:2px dashed #c7cdb0;opacity:0;}' +
    '.hw-slab{position:absolute;display:flex;align-items:center;gap:20px;background:var(--surface);border:1.5px solid var(--divider);border-radius:18px;box-shadow:var(--soft-shadow);padding:0 24px;opacity:0;will-change:transform,opacity,filter;}' +
    '.hw-slab .sn{font-family:var(--font-mono);font-size:30px;font-weight:700;color:var(--text-caption);width:34px;flex:none;text-align:center;}' +
    '.hw-slab .sth{width:78px;height:60px;flex:none;display:flex;align-items:center;justify-content:center;}' +
    '.hw-slab .sth img{max-width:100%;max-height:100%;object-fit:contain;}' +
    '.hw-slab .snm{font-size:24px;font-weight:700;color:var(--text-title);letter-spacing:-.01em;}' +
    '.hw-focus{position:absolute;left:710px;top:206px;width:1130px;opacity:0;}' +
    '.hw-fhead{display:flex;align-items:center;gap:18px;}' +
    '.hw-fbadge{width:62px;height:62px;border-radius:16px;background:var(--primary);color:#fff;font-family:var(--font-mono);font-size:34px;font-weight:700;display:flex;align-items:center;justify-content:center;flex:none;}' +
    '.hw-fname{font-size:44px;font-weight:800;color:var(--text-title);letter-spacing:-.02em;line-height:1.1;}' +
    '.hw-feye{font-family:var(--font-mono);font-size:17px;font-weight:600;letter-spacing:.24em;color:var(--text-secondary);margin-top:4px;}' +
    '.hw-parts{display:flex;flex-wrap:wrap;gap:18px;margin-top:30px;}' +
    '.hw-tile{width:176px;opacity:0;will-change:transform,opacity;}' +
    '.hw-thumb{width:176px;height:130px;background:var(--surface);border:1.5px solid var(--divider);border-radius:16px;box-shadow:var(--soft-shadow);display:flex;align-items:center;justify-content:center;padding:14px;}' +
    '.hw-thumb img{max-width:100%;max-height:100%;object-fit:contain;display:block;}' +
    '.hw-tn{font-size:18px;font-weight:700;color:var(--text-body);text-align:center;margin-top:10px;}' +
    '.hw-chips{display:flex;flex-wrap:wrap;gap:12px;margin-top:30px;}' +
    '.hw-chip{opacity:0;font-size:20px;font-weight:700;color:var(--primary);background:var(--primary-soft);border-radius:999px;padding:9px 20px;}' +
    '.hw-desc{font-size:24px;font-weight:600;color:var(--text-body);line-height:1.5;margin-top:26px;opacity:0;}' +
    '.hw-desc b{color:var(--primary);}'
  );

  /* ════════════════ hw-overview：整车全貌 ════════════════ */
  function buildOverview(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'hw-ov'; root.appendChild(wrap);
    const glow = document.createElement('div'); glow.className = 'hw-glow'; wrap.appendChild(glow);
    const photo = document.createElement('div'); photo.className = 'hw-photo';
    photo.innerHTML = '<img src="' + A + 'car_full.jpg" alt="整车全貌">'; wrap.appendChild(photo);
    const x = document.createElement('div'); x.className = 'hw-ovx';
    x.innerHTML =
      '<div class="hw-kick">整车硬件 · HARDWARE</div>' +
      '<div class="hw-ovt">软硬全栈，<b>亲手搭建</b></div>' +
      '<div class="hw-ovs">一台从底盘、接线、供电到算力都由团队自己搭起来的巡检机器人——双计算单元 + 感知 + 执行 + 供电，四部分一体。</div>' +
      '<div class="hw-stats">' +
      '<div class="hw-stat"><div class="sv">双计算单元</div><div class="sl">紫派 · 香橙派</div></div>' +
      '<div class="hw-stat"><div class="sv">360°</div><div class="sl">激光雷达感知</div></div>' +
      '<div class="hw-stat"><div class="sv">1080P</div><div class="sl">高清视觉摄像头</div></div>' +
      '<div class="hw-stat"><div class="sv">差速底盘</div><div class="sl">双驱动轮 + 万向轮</div></div>' +
      '</div>' +
      '<div class="hw-note">硬件结构、电机与雷达接线、供电系统均由成员<b>亲手搭建调试</b>，与全栈自研软件一道，构成"软硬全栈国产自主"的闭环。</div>';
    wrap.appendChild(x);
    return {
      wrap: wrap, glow: glow, photo: photo,
      kick: x.querySelector('.hw-kick'), t: x.querySelector('.hw-ovt'), s: x.querySelector('.hw-ovs'),
      stats: Array.prototype.slice.call(x.querySelectorAll('.hw-stat')), note: x.querySelector('.hw-note')
    };
  }
  function animOverview(tl, s) {
    const a = s.start, r = s.refs, FX = window.FX;
    tl.fromTo(r.glow, { opacity: 0, scale: 0.9 }, { opacity: 1, scale: 1, duration: 1.0, ease: 'power2.out' }, a + 0.2);
    FX.enter(tl, r.photo, a + 0.3, 'zoomBlur', { dur: 1.0, from: 0.86, origin: '50% 52%' });
    FX.enter(tl, r.kick, a + 0.6, 'fall', { dur: 0.7 });
    FX.enter(tl, r.t, a + 0.85, 'rise', { dur: 0.85 });
    FX.enter(tl, r.s, a + 1.2, 'rise', { dur: 0.8 });
    FX.enterEach(tl, r.stats, a + 1.6, 'spring', 0.12, { dur: 0.6, from: 0.6 });
    FX.enter(tl, r.note, a + 2.5, 'rise', { dur: 0.8 });
    caption(tl, a + 0.7, 3.0, '<b>先看硬件——整车全貌</b>', '双计算单元 · 感知 · 执行 · 供电，四部分一体');
    caption(tl, a + 4.0, 2.6, '<b>软硬全栈，自主可控</b>', '硬件搭建、接线、供电由团队亲手完成');
    // 轻微 ken-burns（与架构章一致，不用无限 repeat）
    tl.fromTo(r.photo, { scale: 1.0 }, { scale: 1.035, duration: s.dur, ease: 'none' }, a + 0.3);
    tl.fromTo(r.wrap, { y: 0 }, { y: -6, duration: s.dur, ease: 'sine.inOut' }, a);
  }

  /* ════════════════ hw-explode：自顶向下六层拆解 ════════════════ */
  const SW = 530, SH = 92, GAP = 26, SX = 80, SY0 = 218;
  function slabTop(i) { return SY0 + i * (SH + GAP); }
  function buildExplode(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'hw-ex'; root.appendChild(wrap);
    const title = document.createElement('div'); title.className = 'hw-extitle';
    title.innerHTML = '<div class="t">整车<b>六层结构</b> · 自顶向下拆解</div>'; wrap.appendChild(title);
    // 爆炸轴（虚线脊）
    const spine = document.createElement('div'); spine.className = 'hw-spine';
    spine.style.cssText = 'left:' + (SX + 28) + 'px;top:' + (slabTop(0) + SH / 2) + 'px;height:' + (slabTop(5) - slabTop(0)) + 'px;';
    wrap.appendChild(spine);
    // 左侧分层堆叠
    const slabs = [];
    LAYERS.forEach(function (L, i) {
      const d = document.createElement('div'); d.className = 'hw-slab';
      d.style.cssText = 'left:' + SX + 'px;top:' + slabTop(i) + 'px;width:' + SW + 'px;height:' + SH + 'px;';
      d.innerHTML = '<div class="sn">' + L.num + '</div><div class="sth"><img src="' + A + L.thumb + '"></div><div class="snm">' + L.name + '</div>';
      wrap.appendChild(d); slabs.push(d);
    });
    // 右侧焦点卡（6 张叠同一处，逐层揭示）
    const focus = [];
    LAYERS.forEach(function (L) {
      const f = document.createElement('div'); f.className = 'hw-focus';
      const tiles = L.parts.map(function (p) {
        return '<div class="hw-tile"><div class="hw-thumb"><img src="' + A + p.img + '"></div><div class="hw-tn">' + p.name + '</div></div>';
      }).join('');
      const chips = L.chips.map(function (c) { return '<span class="hw-chip">' + c + '</span>'; }).join('');
      f.innerHTML =
        '<div class="hw-fhead"><div class="hw-fbadge">' + L.num + '</div><div><div class="hw-fname">' + L.name + '</div><div class="hw-feye">' + L.eye + '</div></div></div>' +
        '<div class="hw-parts">' + tiles + '</div>' +
        '<div class="hw-chips">' + chips + '</div>' +
        '<div class="hw-desc">' + L.desc + '</div>';
      wrap.appendChild(f); focus.push(f);
    });
    return { wrap: wrap, title: title, spine: spine, slabs: slabs, focus: focus };
  }
  function animExplode(tl, s) {
    const a = s.start, r = s.refs, FX = window.FX;
    FX.enter(tl, r.title, a + 0.2, 'fall', { dur: 0.7 });
    caption(tl, a + 0.5, 2.2, '<b>把整车自顶向下拆开</b>', '六层结构 · 16 个部件逐一登场');
    // 1) 分层爆炸：各 slab 从居中聚拢态向终位散开
    const cY = (slabTop(0) + slabTop(5) + SH) / 2, cgap = 6, cTotal = 6 * SH + 5 * cgap;
    tl.fromTo(r.spine, { opacity: 0, scaleY: 0.2, transformOrigin: '50% 50%' }, { opacity: 1, scaleY: 1, duration: 0.7, ease: 'power2.out' }, a + 0.6);
    r.slabs.forEach(function (d, i) {
      const collapsedTop = cY - cTotal / 2 + i * (SH + cgap), dy = collapsedTop - slabTop(i);
      tl.fromTo(d, { opacity: 0, y: dy, filter: 'blur(8px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.85, ease: 'back.out(1.3)' }, a + 0.55 + i * 0.07);
    });
    // 2) 逐层焦点：高亮左 slab + 揭示右焦点卡
    const LD = 1.95, base = a + 2.4;
    LAYERS.forEach(function (L, i) {
      const t = base + i * LD, slab = r.slabs[i], f = r.focus[i];
      // slab 高亮
      tl.to(slab, { scale: 1.055, backgroundColor: '#e4ebc8', borderColor: '#485c11', boxShadow: '0 16px 38px -14px rgba(72,92,17,.5)', duration: 0.4, ease: 'power2.out', transformOrigin: '0% 50%' }, t);
      tl.to(slab.querySelector('.sn'), { color: '#3a4a0e', duration: 0.3 }, t);
      tl.to(slab.querySelector('.snm'), { color: '#3a4a0e', duration: 0.3 }, t);
      tl.to(slab, { scale: 1.0, backgroundColor: '#ffffff', borderColor: '#e7e5da', boxShadow: '0 4px 14px -6px rgba(0,0,0,.18)', duration: 0.4, ease: 'power2.in' }, t + LD - 0.12);
      tl.to(slab.querySelector('.sn'), { color: '#9b9d8d', duration: 0.3 }, t + LD - 0.12);
      tl.to(slab.querySelector('.snm'), { color: '#1d2016', duration: 0.3 }, t + LD - 0.12);
      // 焦点卡揭示
      FX.enter(tl, f, t + 0.1, i % 2 ? 'rise' : 'zoomBlur', { dur: 0.6, from: 0.94 });
      FX.enterEach(tl, f.querySelectorAll('.hw-tile'), t + 0.35, 'spring', 0.1, { dur: 0.55, from: 0.6 });
      FX.enterEach(tl, f.querySelectorAll('.hw-chip'), t + 0.7, 'pop', 0.07, { dur: 0.45 });
      tl.fromTo(f.querySelector('.hw-desc'), { opacity: 0, y: 16, filter: 'blur(6px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.55, ease: 'power3.out' }, t + 0.85);
      caption(tl, t + 0.2, LD - 0.35, '<b>' + L.num + ' ' + L.name + '</b>', L.chips.slice(0, 2).join(' · '));
      if (i < LAYERS.length - 1) FX.exit(tl, f, t + LD - 0.2, 'scale', 0.4);
    });
    tl.fromTo(r.wrap, { scale: 1.0 }, { scale: 1.012, duration: s.dur, ease: 'none' }, a);
  }

  F.addScene({ id: 'hw-overview', dur: 7.0, build: buildOverview, anim: animOverview });
  F.addScene({ id: 'hw-explode', dur: 14.6, build: buildExplode, anim: animExplode });
})();
