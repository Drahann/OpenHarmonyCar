/* =====================================================================================
   场景库 act 8：更多细节 —— 设备互信(DeviceTrustPage) + 手填 IP 兜底(SetIPPage)。自包含。
   勿用 file-tool Edit（会截断）。经 FILM.addScene 注册。文案/色值见 00-视频制作总纲.md §1(08)/§2.1/§5.4。
   页面内容逐字复刻 app-harmony/.../pages/DeviceTrustPage.ets、SetIPPage.ets。
   ===================================================================================== */
(function () {
  "use strict";
  const F = window.FILM;
  const centerOf = F.centerOf, camTo = F.camTo, camReset = F.camReset,
    cursorShow = F.cursorShow, cursorHide = F.cursorHide, cursorMove = F.cursorMove,
    tapEl = F.tapEl, caption = F.caption, addCSS = F.addCSS, I = F.I, STAGE_W = F.STAGE_W;

  addCSS(
    /* ── 设备互信 ── */
    '.app-trust{position:absolute;inset:0;background:var(--page-bg);padding:26px 34px;display:flex;flex-direction:column;}' +
    '.tr-top{display:flex;align-items:center;}' +
    '.tr-back{font-size:34px;line-height:1;color:var(--primary);}' +
    '.tr-title{font-size:32px;font-weight:700;color:var(--text-title);margin-left:14px;}' +
    '.tr-research{height:46px;border-radius:999px;background:var(--primary);color:#fff;font-size:17px;font-weight:600;display:flex;align-items:center;justify-content:center;padding:0 20px;}' +
    '.tr-note{background:var(--surface);border-radius:14px;box-shadow:var(--soft-shadow);padding:16px 20px;margin-top:18px;}' +
    '.tr-note .nt{font-size:18px;font-weight:700;color:var(--text-title);}' +
    '.tr-note .nb{font-size:15px;color:var(--text-secondary);margin-top:6px;line-height:1.55;}' +
    '.tr-sec{display:flex;align-items:baseline;margin-top:22px;}' +
    '.tr-sec .st{font-size:23px;font-weight:700;color:var(--text-title);}' +
    '.tr-sec .sc{font-size:16px;color:var(--text-caption);margin-left:2px;}' +
    '.tr-slot{position:relative;min-height:82px;margin-top:10px;}' +
    '.tr-slot>*{position:absolute;left:0;right:0;top:0;}' +
    '.tr-hint{font-size:17px;color:var(--text-secondary);padding:8px 2px;}' +
    '.tr-row{display:flex;align-items:center;background:var(--surface);border-radius:12px;box-shadow:var(--soft-shadow);padding:16px 18px;}' +
    '.tr-row .dot{width:12px;height:12px;border-radius:50%;margin-right:16px;flex:none;}' +
    '.tr-row .dn{font-size:20px;color:var(--text-body);}' +
    '.tr-row .di{font-size:15px;color:var(--text-secondary);font-family:var(--font-mono);margin-top:2px;}' +
    '.tr-row .sp{flex:1;}' +
    '.tr-pill{height:40px;border-radius:999px;display:flex;align-items:center;justify-content:center;font-size:16px;font-weight:600;padding:0 16px;}' +
    '.tr-pill.trusted{background:var(--primary);color:#fff;}' +
    '.tr-pill.pair{background:var(--surface);color:var(--primary);border:1.5px solid var(--primary);gap:7px;}' +
    '.tr-pill.ghost{background:var(--surface);color:var(--text-secondary);border:1px solid var(--text-caption);margin-left:10px;}' +
    '.tr-spin{width:15px;height:15px;border-radius:50%;border:2.5px solid var(--primary-soft);border-top-color:var(--primary);animation:spin 1s linear infinite;}' +
    '.tr-status{font-size:16px;color:var(--text-secondary);text-align:center;margin-top:auto;padding-top:14px;}' +
    /* ── 手动连接 ── */
    '.app-setip{position:absolute;inset:0;background:var(--page-bg);padding:30px 42px;display:flex;flex-direction:column;}' +
    '.si-title{font-size:44px;font-weight:800;color:var(--text-title);margin-top:26px;}' +
    '.si-sub{font-size:18px;color:var(--text-secondary);margin-top:14px;line-height:1.55;max-width:920px;}' +
    '.si-field{margin-top:46px;}' +
    '.si-label{font-size:15px;color:var(--text-secondary);}' +
    '.si-input{height:66px;border-radius:12px;background:var(--surface);border:1.5px solid var(--border);display:flex;align-items:center;padding:0 22px;margin-top:10px;}' +
    '.si-input.invalid{border-color:var(--danger);}' +
    '.si-input .val{font-family:var(--font-mono);font-size:27px;color:var(--text-body);white-space:pre;}' +
    '.si-input .ph{font-family:var(--font-mono);font-size:27px;color:var(--text-caption);}' +
    '.si-input .caret{width:2px;height:32px;background:var(--primary);margin-left:1px;opacity:0;}' +
    '.si-err{font-size:15px;color:var(--danger);margin-top:9px;opacity:0;}' +
    '.si-save{height:68px;border-radius:999px;background:var(--surface-muted);color:var(--text-caption);font-size:23px;font-weight:600;display:flex;align-items:center;justify-content:center;margin-top:auto;}'
  );

  /* ── 工具 ──────────────────────────────────────────────────────────────────── */
  function sec(num, title, sub) {
    return {
      build: function (root) {
        root.classList.add('paper');
        const col = document.createElement('div'); col.className = 'center-col';
        col.innerHTML = '<div class="secnum">' + num + '</div><div class="sectitle">' + title + '</div><div class="secsub">' + sub + '</div>';
        root.appendChild(col); return { n: col.children[0], t: col.children[1], sb: col.children[2], col: col };
      },
      anim: function (tl, s) {
        const a = s.start, r = s.refs;
        F.rise(tl, r.n, a + 0.3, { y: 24, dur: 0.9 }); F.rise(tl, r.t, a + 0.6, { y: 24, dur: 0.9 }); F.rise(tl, r.sb, a + 0.95, { y: 18, dur: 0.8 });
        tl.fromTo(r.col, { scale: 1.0 }, { scale: 1.03, duration: s.dur, ease: 'none' }, a);
      }
    };
  }
  function setText(tl, at, el, txt, prev) { tl.to(el, { duration: 0.01, onComplete: function () { el.textContent = txt; }, onReverseComplete: function () { el.textContent = prev; } }, at); }
  function setClass(tl, at, el, cls, add) {
    tl.to(el, { duration: 0.01,
      onComplete: function () { el.classList.toggle(cls, add); },
      onReverseComplete: function () { el.classList.toggle(cls, !add); } }, at);
  }
  /* 可 seek 的打字机：从 fromLen 续打到 full（无 setTimeout）。 */
  function typeText(tl, at, dur, el, full, fromLen) {
    fromLen = fromLen || 0;
    const o = { n: fromLen };
    tl.to(o, { n: full.length, duration: dur, ease: 'none',
      onUpdate: function () { el.textContent = full.substring(0, Math.round(o.n)); },
      onReverseComplete: function () { el.textContent = full.substring(0, fromLen); } }, at);
  }
  function makeTablet(cls, html) {
    const cam = document.createElement('div'); cam.className = 'cam';
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (STAGE_W - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const page = document.createElement('div'); page.className = cls; page.innerHTML = html;
    screen.appendChild(page); T.appendChild(screen); cam.appendChild(T);
    return { cam: cam, T: T, page: page };
  }

  /* ── 08b-1 设备互信 ─────────────────────────────────────────────────────────── */
  const SHORT_ID = 'a1c93f…7f4e';   // shortId(deviceId)=前6…后4，见 DeviceTrustPage.shortId
  function buildTrust(root) {
    root.classList.add('paper');
    const html =
      '<div class="tr-top"><span class="tr-back">〈</span><span class="tr-title">设备互信</span><span style="flex:1"></span><span class="tr-research">重新搜索</span></div>' +
      '<div class="tr-note"><div class="nt">配对步骤</div><div class="nb">① 平板与车连同一 WiFi/热点　② 车上接显示器+鼠标　③ 点车辆「配对」→ 车屏弹出系统 PIN 码 → 按平板系统提示输入该 PIN。配对一次即可，之后自动互信。</div></div>' +
      '<div class="tr-sec"><span class="st">已信任设备</span><span class="sc" data-tc>（0）</span></div>' +
      '<div class="tr-slot">' +
        '<div class="tr-hint" data-thint>尚无已信任设备。在下方发现列表里选中车辆配对。</div>' +
        '<div class="tr-row" data-trow style="opacity:0"><span class="dot" style="background:var(--success)"></span><div><div class="dn">巡检车</div><div class="di">' + SHORT_ID + '</div></div><span class="sp"></span><span class="tr-pill trusted">已信任</span><span class="tr-pill ghost">解绑</span></div>' +
      '</div>' +
      '<div class="tr-sec"><span class="st">发现的新设备</span><span class="sc" data-dc>（1）</span></div>' +
      '<div class="tr-slot">' +
        '<div class="tr-hint" data-dhint style="opacity:0">未发现新设备。确保平板与车在同一 WiFi/热点。</div>' +
        '<div class="tr-row" data-drow><span class="dot" style="background:var(--text-caption)"></span><div><div class="dn">巡检车</div><div class="di">' + SHORT_ID + '</div></div><span class="sp"></span><span class="tr-pill pair" data-pair><span class="tr-spin" data-spin style="display:none"></span><span data-pairtext>配对</span></span><span class="tr-pill ghost">解绑重置</span></div>' +
      '</div>' +
      '<div class="tr-status" data-status style="opacity:0"></div>';
    const m = makeTablet('app-trust', html);
    root.appendChild(m.cam);
    const q = function (s) { return m.page.querySelector(s); };
    return { cam: m.cam, T: m.T, page: m.page,
      tc: q('[data-tc]'), dc: q('[data-dc]'), thint: q('[data-thint]'), dhint: q('[data-dhint]'),
      trow: q('[data-trow]'), drow: q('[data-drow]'), pair: q('[data-pair]'),
      pairtext: q('[data-pairtext]'), spin: q('[data-spin]'), status: q('[data-status]') };
  }
  function animTrust(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.8, '<b>软总线一次互信</b>', '配对一次，跨重启持久');

    // 聚焦发现行「配对」
    let t = a + 2.8;
    cursorShow(tl, a + 1.8, centerOf(r.pair).x, centerOf(r.pair).y + 64);
    camTo(tl, r.cam, t, centerOf(r.drow).x, centerOf(r.drow).y, 1.45, 1.0);
    tapEl(tl, t + 1.0, r.pair, 0.6);

    // 认证中：spinner + 文案 + 状态栏
    t = a + 4.4;
    tl.set(r.spin, { display: 'inline-block' }, t);
    setText(tl, t, r.pairtext, '认证中', '配对');
    tl.to(r.status, { opacity: 1, duration: 0.4 }, t);
    setText(tl, t, r.status, '认证中：车屏会弹出 PIN 码，请按平板系统提示输入…', '');
    caption(tl, a + 4.2, 2.8, '<b>平板发起 · 车屏确认 PIN</b>', '系统弹窗代劳 · 账号无关认证');
    camReset(tl, r.cam, a + 7.2, 1.0);

    // 配对成功：新设备 → 已信任（计数互换 + 行交叉淡入）
    t = a + 7.8;
    tl.set(r.spin, { display: 'none' }, t);
    tl.to(r.drow, { opacity: 0, y: -10, duration: 0.5, ease: 'power2.in' }, t);
    tl.to(r.dhint, { opacity: 1, duration: 0.5 }, t + 0.4);
    tl.to(r.thint, { opacity: 0, duration: 0.4 }, t);
    tl.fromTo(r.trow, { opacity: 0, y: 10 }, { opacity: 1, y: 0, duration: 0.6, ease: 'power3.out' }, t + 0.3);
    setText(tl, t + 0.3, r.tc, '（1）', '（0）');
    setText(tl, t + 0.3, r.dc, '（0）', '（1）');
    setText(tl, t + 0.4, r.status, '配对成功，已建立互信。', '认证中：车屏会弹出 PIN 码，请按平板系统提示输入…');
    caption(tl, a + 8.4, 2.8, '<b>互信建立</b>', '之后无界面 agent 直接入会、同步黑板');
    cursorHide(tl, a + 11.6);
  }

  /* ── 08b-2 手动连接（兜底）─────────────────────────────────────────────────── */
  function buildSetIP(root) {
    root.classList.add('paper');
    const backHtml = '<div class="ctl-back" style="box-shadow:none;background:transparent">' + I.back + '</div>';
    const html =
      '<div class="tr-top">' + backHtml + '<span style="flex:1"></span></div>' +
      '<div class="si-title">手动连接</div>' +
      '<div class="si-sub">发现不可用时（客户端隔离 / 固定 IP / 调试）手填车辆 IP。优先使用首页“发现设备”。</div>' +
      '<div class="si-field"><div class="si-label">车辆 IP</div>' +
        '<div class="si-input" data-input><span class="val" data-val></span><span class="ph" data-ph>192.168.x.x</span><span class="caret" data-caret></span></div>' +
        '<div class="si-err" data-err>IP 格式不正确</div>' +
      '</div>' +
      '<div class="si-save" data-save>保存并连接</div>';
    const m = makeTablet('app-setip', html);
    root.appendChild(m.cam);
    const q = function (s) { return m.page.querySelector(s); };
    return { cam: m.cam, T: m.T, page: m.page,
      input: q('[data-input]'), val: q('[data-val]'), ph: q('[data-ph]'), caret: q('[data-caret]'),
      err: q('[data-err]'), save: q('[data-save]') };
  }
  function animSetIP(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.8, '<b>手填 IP 兜底</b>', '客户端隔离 · 固定 IP · 调试时用');

    // 聚焦输入框 → 取焦点
    let t = a + 2.6;
    cursorShow(tl, a + 1.8, centerOf(r.input).x, centerOf(r.input).y + 60);
    camTo(tl, r.cam, t, centerOf(r.input).x, centerOf(r.input).y, 1.5, 1.0);
    tapEl(tl, t + 0.9, r.input, 0.6);
    tl.to(r.ph, { opacity: 0, duration: 0.25 }, t + 1.4);
    tl.to(r.caret, { opacity: 1, duration: 0.25 }, t + 1.4);

    // 输入不完整 IP → isValidIp 失败
    t = a + 4.6;
    typeText(tl, t, 1.0, r.val, '192.168.43', 0);
    setClass(tl, t + 1.1, r.input, 'invalid', true);
    tl.to(r.err, { opacity: 1, duration: 0.35 }, t + 1.1);
    caption(tl, a + 5.2, 2.6, '<b>实时校验</b>', 'isValidIp 不合法即提示 · 按钮置灰');

    // 补全 → 校验通过 → 按钮启用
    t = a + 6.6;
    typeText(tl, t, 0.6, r.val, '192.168.43.12', 10);
    setClass(tl, t + 0.7, r.input, 'invalid', false);
    tl.to(r.err, { opacity: 0, duration: 0.3 }, t + 0.7);
    tl.to(r.save, { backgroundColor: '#485c11', color: '#ffffff', duration: 0.4 }, t + 0.7);

    // 保存并连接
    t = a + 8.0;
    camReset(tl, r.cam, t, 1.0);
    tapEl(tl, t + 0.9, r.save, 0.7);
    caption(tl, a + 8.6, 2.6, '<b>保存并连接</b>', '直达控制页 · A* 单机模式');
    cursorHide(tl, a + 10.2);
  }

  F.addScene(Object.assign({ id: '08-sec', dur: 4.2 }, sec('08', '更多细节', '互信配对 · 手填兜底')));
  F.addScene({ id: '08-trust', dur: 13, build: buildTrust, anim: animTrust });
  F.addScene({ id: '08-setip', dur: 12, build: buildSetIP, anim: animSetIP });
})();
