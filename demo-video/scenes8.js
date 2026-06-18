/* scenes8.js —— 08-trust(PIN 互信演示) + algo-trio(三平板并演 + 平板下方"三套自研规划算法"实时原理动画)。
   本轮(用户反馈)：把原 pr-algo 流程图并入 algo-trio——上半三平板演三种规划(单机/全覆盖/多机)，
   **下半三块画布实时演算法原理**：A*(波前扩散+回溯最短路) / 牛耕 BCD(障碍切 cell+弓字刷扫) / 生成树 STC(最小生成树生长+螺旋覆盖)。
   动效不止渐入：方向性入场(平板视差) + 实时算法过程(逐格扩散/描绘/生长) + 焦点下移(平板淡出让位算法)。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。 */
(function () {
  "use strict";
  const F = window.FILM, MK = window.MAPKIT;
  const centerOf = F.centerOf, cursorShow = F.cursorShow, cursorHide = F.cursorHide, tapEl = F.tapEl,
    caption = F.caption, addCSS = F.addCSS, I = F.I, STAGE_W = F.STAGE_W;

  function setText(tl, at, el, txt, prev) { tl.to(el, { duration: 0.01, onComplete: function () { el.textContent = txt; }, onReverseComplete: function () { el.textContent = prev; } }, at); }
  function addClass(tl, at, el, cls) { tl.to(el, { duration: 0.01, onComplete: function () { el.classList.add(cls); }, onReverseComplete: function () { el.classList.remove(cls); } }, at); }
  function makeTablet(cls, html) {
    const cam = document.createElement('div'); cam.className = 'cam';
    const T = document.createElement('div'); T.className = 'tablet';
    const TW = 1300, TH = 812, TX = (STAGE_W - TW) / 2, TY = 128;
    T.style.cssText += 'left:' + TX + 'px;top:' + TY + 'px;width:' + TW + 'px;height:' + TH + 'px;';
    T.innerHTML = '<div class="cam-dot"></div>';
    const screen = document.createElement('div'); screen.className = 'screen';
    const page = document.createElement('div'); page.className = cls; page.innerHTML = html;
    screen.appendChild(page); T.appendChild(screen); cam.appendChild(T);
    return { cam: cam, T: T, page: page, screen: screen };
  }
  function initCam(tl, cam, at, sc, tx, ty) { cam._cs = sc; cam._cx = tx; cam._cy = ty; tl.set(cam, { scale: sc, x: tx, y: ty, transformOrigin: '0 0' }, at); }
  function moveCam(tl, cam, at, sc, tx, ty, dur) { cam._cs = sc; cam._cx = tx; cam._cy = ty; tl.to(cam, { scale: sc, x: tx, y: ty, transformOrigin: '0 0', duration: dur, ease: 'power3.inOut' }, at); }

  /* ============================ 08-trust：PIN 码互信演示（未改） ============================ */
  const PIN = '638495';
  const PI_IMG = 'assets/pi-board.png';
  const MON_IMG = 'assets/monitor.png';
  const DEV_NAME = 'Purple Pi OH', DEV_ID = 'b098a1…285c';

  addCSS(
    '.devs{position:absolute;left:0;top:0;width:920px;height:1080px;opacity:0;z-index:1;}' +
    '.devs .mon{position:absolute;left:104px;top:118px;width:500px;}' +
    '.devs .mon>img{width:100%;display:block;filter:drop-shadow(0 24px 50px rgba(40,46,14,.22));}' +
    '.devs .mon-scr{position:absolute;left:9%;top:19%;width:82%;height:45%;}' +
    '.mon-dlg{background:#f3f3f1;border-radius:14px;box-shadow:0 12px 30px rgba(0,0,0,.4);padding:16px 20px;width:76%;text-align:center;opacity:0;position:absolute;left:50%;top:50%;}' +
    '.mon-dlg .md-t{font-size:14px;color:#1b1b1b;line-height:1.5;}' +
    '.mon-dlg .md-q{font-size:12px;color:#5a5a5a;margin-top:6px;}' +
    '.mon-dlg .md-opts{margin-top:12px;display:flex;flex-direction:column;gap:7px;}' +
    '.mon-dlg .md-opt{font-size:13px;color:#2f6fd6;padding:5px 0;border-top:1px solid #e2e2e0;}' +
    '.mon-dlg .md-opt.sel{color:#fff;background:#2f6fd6;border-radius:8px;border-top:none;font-weight:600;}' +
    '.mon-dlg .md-code{font-size:15px;font-weight:700;color:#1b1b1b;}' +
    '.mon-dlg .md-digs{display:flex;gap:13px;justify-content:center;margin:14px 0 10px;}' +
    '.mon-dlg .md-dig{font-family:var(--font-mono);font-size:30px;font-weight:700;color:#1b1b1b;}' +
    '.mon-dlg .md-cancel{font-size:13px;color:#2f6fd6;}' +
    '.devs .pi{position:absolute;left:214px;top:660px;width:236px;}' +
    '.devs .pi>img{width:100%;display:block;border-radius:10px;filter:drop-shadow(0 18px 40px rgba(40,46,14,.26));}' +
    '.devs .dev-cap{position:absolute;font-size:17px;font-weight:600;color:var(--text-secondary);}' +
    '.devs .dev-cap b{color:var(--primary);font-weight:700;}' +
    '.devs .cable{position:absolute;left:0;top:0;width:920px;height:1080px;pointer-events:none;}' +
    '.app-trust .pin-mask{position:absolute;inset:0;background:rgba(20,22,14,.30);opacity:0;z-index:20;}' +
    '.app-trust .pindlg{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);width:560px;background:var(--surface);border-radius:28px;box-shadow:0 40px 90px -24px rgba(30,32,22,.5);padding:38px 40px 30px;text-align:center;opacity:0;z-index:21;}' +
    '.app-trust .pindlg .pd-t{font-size:30px;font-weight:700;color:var(--text-title);}' +
    '.app-trust .pindlg .pd-s{font-size:18px;color:var(--text-secondary);margin-top:10px;}' +
    '.app-trust .pindlg .pd-circs{display:flex;gap:22px;justify-content:center;margin:34px 0 28px;}' +
    '.app-trust .pindlg .pd-c{width:40px;height:40px;border-radius:50%;border:2px solid var(--text-caption);display:flex;align-items:center;justify-content:center;font-family:var(--font-mono);font-size:25px;font-weight:700;color:var(--primary);}' +
    '.app-trust .pindlg .pd-c.on{border-color:var(--primary);background:var(--primary-soft);}' +
    '.app-trust .pindlg .pd-cancel{font-size:22px;color:var(--info);font-weight:600;}'
  );

  function buildTrust2(root) {
    root.classList.add('paper');
    const html =
      '<div class="tr-top"><span class="tr-back">〈</span><span class="tr-title">设备互信</span><span style="flex:1"></span><span class="tr-research">重新搜索</span></div>' +
      '<div class="tr-note"><div class="nt">配对步骤</div><div class="nb">① 平板与车连同一 WiFi/热点　② 车上接显示器+鼠标　③ 点车辆「配对」→ 车屏先弹"请求连接·是否信任"，信任后弹系统 PIN 码 → 按平板系统提示输入该 PIN。配对一次即可，之后自动互信。</div></div>' +
      '<div class="tr-sec"><span class="st">已信任设备</span><span class="sc" data-tc>（0）</span></div>' +
      '<div class="tr-slot">' +
        '<div class="tr-hint" data-thint>尚无已信任设备。在下方发现列表里选中车辆配对。</div>' +
        '<div class="tr-row" data-trow style="opacity:0"><span class="dot" style="background:var(--success)"></span><div><div class="dn">' + DEV_NAME + '</div><div class="di">' + DEV_ID + '</div></div><span class="sp"></span><span class="tr-pill trusted">已信任</span><span class="tr-pill ghost">解绑</span></div>' +
      '</div>' +
      '<div class="tr-sec"><span class="st">发现的新设备</span><span class="sc" data-dc>（1）</span></div>' +
      '<div class="tr-slot">' +
        '<div class="tr-hint" data-dhint style="opacity:0">未发现新设备。确保平板与车在同一 WiFi/热点。</div>' +
        '<div class="tr-row" data-drow><span class="dot" style="background:var(--text-caption)"></span><div><div class="dn">' + DEV_NAME + '</div><div class="di">' + DEV_ID + '</div></div><span class="sp"></span><span class="tr-pill pair" data-pair><span class="tr-spin" data-spin style="display:none"></span><span data-pairtext>配对</span></span><span class="tr-pill ghost">解绑重置</span></div>' +
      '</div>' +
      '<div class="tr-status" data-status style="opacity:0"></div>' +
      '<div class="pin-mask" data-mask></div>' +
      '<div class="pindlg" data-dlg><div class="pd-t">连接 ' + DEV_NAME + '</div><div class="pd-s">请输入对端设备上显示的连接码</div>' +
        '<div class="pd-circs">' + PIN.split('').map(function () { return '<div class="pd-c"></div>'; }).join('') + '</div>' +
        '<div class="pd-cancel">取消</div></div>';
    const m = makeTablet('app-trust', html);
    m.cam.style.zIndex = '5';
    root.appendChild(m.cam);

    const devs = document.createElement('div'); devs.className = 'devs';
    devs.innerHTML =
      '<svg class="cable" viewBox="0 0 920 1080"><path data-cable d="M334 612 C 334 648, 322 650, 320 664" fill="none" stroke="#485c11" stroke-width="7" stroke-linecap="round" stroke-dasharray="1" stroke-dashoffset="1" pathLength="1" opacity=".7"/></svg>' +
      '<div class="mon"><img src="' + MON_IMG + '" alt="显示器"><div class="mon-scr">' +
        '<div class="mon-dlg" data-perm><div class="md-t">「191****12 的 MatePad · 巡检机器人」请求连接本机</div><div class="md-q">是否信任此应用并允许连接？</div><div class="md-opts"><div class="md-opt sel" data-permok>始终信任</div><div class="md-opt">临时信任</div><div class="md-opt">不信任</div></div></div>' +
        '<div class="mon-dlg" data-pin><div class="md-code">连接码</div><div class="md-digs">' + PIN.split('').map(function (d) { return '<div class="md-dig">' + d + '</div>'; }).join('') + '</div><div class="md-cancel">取消</div></div>' +
      '</div></div>' +
      '<div class="pi"><img src="' + PI_IMG + '" alt="紫派"></div>' +
      '<div class="dev-cap" style="left:128px;top:86px">车载显示器 · <b>车屏弹 PIN</b></div>' +
      '<div class="dev-cap" style="left:214px;top:828px">紫派 · <b>RK3566 · OpenHarmony</b></div>';
    root.appendChild(devs);

    const q = function (s) { return m.page.querySelector(s); };
    return {
      cam: m.cam, T: m.T, page: m.page, devs: devs,
      pair: q('[data-pair]'), pairtext: q('[data-pairtext]'), spin: q('[data-spin]'), status: q('[data-status]'),
      trow: q('[data-trow]'), thint: q('[data-thint]'), drow: q('[data-drow]'), dhint: q('[data-dhint]'),
      tc: q('[data-tc]'), dc: q('[data-dc]'),
      mask: q('[data-mask]'), dlg: q('[data-dlg]'), circs: Array.prototype.slice.call(m.page.querySelectorAll('.pd-c')),
      perm: devs.querySelector('[data-perm]'), permok: devs.querySelector('[data-permok]'), pin: devs.querySelector('[data-pin]'),
      mondigs: Array.prototype.slice.call(devs.querySelectorAll('.md-dig')), cable: devs.querySelector('[data-cable]')
    };
  }

  function animTrust2(tl, s) {
    const a = s.start, r = s.refs;
    tl.fromTo(r.T, { opacity: 0, scale: 0.965, y: 24, filter: 'blur(10px)' }, { opacity: 1, scale: 1, y: 0, filter: 'blur(0px)', duration: 1.0, ease: 'power4.out' }, a + 0.2);
    caption(tl, a + 0.5, 2.6, '<b>软总线一次互信</b>', '配对一次，跨重启持久');

    let t = a + 2.4;
    cursorShow(tl, a + 1.6, centerOf(r.pair).x, centerOf(r.pair).y + 64);
    tapEl(tl, t, r.pair, 0.6);
    cursorHide(tl, t + 0.9);

    t = a + 3.2;
    tl.to(r.T, { x: 372, y: 60, scale: 0.66, transformOrigin: '50% 50%', duration: 1.1, ease: 'power3.inOut' }, t);
    tl.to(r.devs, { opacity: 1, duration: 0.8, ease: 'power2.out' }, t + 0.4);
    tl.fromTo(r.cable, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: 0.7, ease: 'power2.inOut' }, t + 0.8);
    tl.set(r.spin, { display: 'inline-block' }, t + 0.5);
    setText(tl, t + 0.5, r.pairtext, '认证中', '配对');
    tl.to(r.status, { opacity: 1, duration: 0.4 }, t + 0.5);
    setText(tl, t + 0.5, r.status, '认证中：车屏先弹"请求连接"，信任后弹 PIN 码，请按平板提示输入…', '');
    caption(tl, t + 0.7, 2.6, '<b>平板缩小 · 车接显示器</b>', '车上插显示器+鼠标，配对走系统弹窗');

    t = a + 5.2;
    tl.fromTo(r.perm, { opacity: 0, scale: 0.94, xPercent: -50, yPercent: -50 }, { opacity: 1, scale: 1, xPercent: -50, yPercent: -50, duration: 0.45, ease: 'back.out(1.3)' }, t);
    caption(tl, t + 0.3, 2.4, '<b>车屏：请求连接 · 是否信任</b>', '点「始终信任」放行（账号无关）');
    tl.fromTo(r.permok, { backgroundColor: '#2f6fd6' }, { backgroundColor: '#1f55b5', duration: 0.18, yoyo: true, repeat: 1 }, t + 1.4);
    tl.to(r.perm, { opacity: 0, scale: 0.96, duration: 0.35, ease: 'power2.in' }, t + 2.1);

    t = a + 7.7;
    tl.fromTo(r.pin, { opacity: 0, scale: 0.94, xPercent: -50, yPercent: -50 }, { opacity: 1, scale: 1, xPercent: -50, yPercent: -50, duration: 0.45, ease: 'back.out(1.3)' }, t);
    r.mondigs.forEach(function (d, i) { tl.fromTo(d, { opacity: 0, y: 8 }, { opacity: 1, y: 0, duration: 0.28, ease: 'back.out(1.4)' }, t + 0.3 + i * 0.1); });
    caption(tl, t + 0.4, 2.4, '<b>车屏弹出连接码 ' + PIN + '</b>', '系统级配对码 · 平板按提示输入');

    t = a + 9.9;
    tl.to(r.mask, { opacity: 1, duration: 0.4 }, t);
    tl.fromTo(r.dlg, { opacity: 0, scale: 0.9 }, { opacity: 1, scale: 1, transformOrigin: '50% 50%', duration: 0.5, ease: 'back.out(1.4)' }, t);
    caption(tl, t + 0.4, 2.6, '<b>平板按提示逐位输入</b>', '输入车屏显示的连接码 ' + PIN);
    r.circs.forEach(function (c, i) {
      const at = t + 1.0 + i * 0.32;
      setText(tl, at, c, PIN[i], '');
      addClass(tl, at, c, 'on');
      tl.fromTo(c, { scale: 0.7 }, { scale: 1, duration: 0.26, ease: 'back.out(2)' }, at);
    });

    t = a + 12.6;
    tl.to([r.dlg, r.mask], { opacity: 0, duration: 0.45, ease: 'power2.in' }, t);
    tl.set(r.spin, { display: 'none' }, t + 0.45);
    tl.to(r.devs, { opacity: 0, duration: 0.6, ease: 'power2.inOut' }, t + 0.2);
    tl.to(r.T, { x: 0, y: 0, scale: 1, duration: 1.0, ease: 'power3.inOut' }, t + 0.2);
    tl.to(r.drow, { opacity: 0, y: -10, duration: 0.5, ease: 'power2.in' }, t + 0.3);
    tl.to(r.dhint, { opacity: 1, duration: 0.5 }, t + 0.7);
    tl.to(r.thint, { opacity: 0, duration: 0.4 }, t + 0.3);
    tl.fromTo(r.trow, { opacity: 0, y: 10 }, { opacity: 1, y: 0, duration: 0.6, ease: 'power3.out' }, t + 0.6);
    setText(tl, t + 0.6, r.tc, '（1）', '（0）');
    setText(tl, t + 0.6, r.dc, '（0）', '（1）');
    setText(tl, t + 0.6, r.status, '配对成功，已建立互信。', '认证中：车屏先弹"请求连接"，信任后弹 PIN 码，请按平板提示输入…');
    caption(tl, t + 0.9, 2.8, '<b>互信建立</b>', '之后无界面 agent 直接入会、同步黑板');
  }

  F.addScene({ id: '08-trust', dur: 16.5, build: buildTrust2, anim: animTrust2 });

  /* ============================ algo-trio：三平板 + 三套规划算法实时原理 ============================ */
  const VID = 'assets/meter.mp4';
  const G = MK.TH.green, B = MK.TH.blue, PW = 536, PH = 356;

  addCSS(
    '.mtab{position:absolute;border-radius:22px;background:#26281f;box-shadow:0 30px 64px -26px rgba(30,32,22,.55),0 6px 18px -8px rgba(0,0,0,.4);}' +
    '.mtab .mscreen{position:absolute;inset:12px;border-radius:13px;overflow:hidden;background:#e9e7dc;}' +
    '.mk-svg{position:absolute;inset:0;width:100%;height:100%;}' +
    '.mk-top{position:absolute;left:12px;top:11px;right:12px;display:flex;align-items:center;gap:7px;pointer-events:none;}' +
    '.mk-pill{display:flex;align-items:center;height:25px;padding:0 9px;background:#fff;border-radius:999px;box-shadow:var(--soft-shadow);gap:5px;}' +
    '.mk-pill .d{width:7px;height:7px;border-radius:50%;background:var(--success);}' +
    '.mk-pill .ip{font-family:var(--font-mono);font-size:11px;color:var(--text-body);}' +
    '.mk-pill .md{font-size:11px;color:var(--text-secondary);}' +
    '.mk-pill .cr{font-size:8px;color:var(--text-caption);}' +
    '.mk-rebuild{margin-left:auto;font-size:10px;color:var(--primary);border:1px solid var(--primary);border-radius:999px;padding:3px 8px;background:#fff;}' +
    '.mk-vbtn{width:25px;height:25px;border-radius:50%;background:#fff;box-shadow:var(--soft-shadow);display:flex;align-items:center;justify-content:center;}' +
    '.mk-vbtn svg{width:14px;height:14px;}' +
    '.mk-fabs{position:absolute;right:12px;top:48%;display:flex;flex-direction:column;gap:6px;}' +
    '.mk-fab{width:26px;height:26px;border-radius:50%;background:#fff;box-shadow:var(--soft-shadow);display:flex;align-items:center;justify-content:center;font-size:15px;color:var(--text-secondary);}' +
    '.mk-pip{position:absolute;left:14px;top:44px;display:flex;align-items:center;height:24px;padding:0 9px;border-radius:999px;background:#fff;border:1px solid var(--primary);color:var(--primary);font-size:11px;font-weight:600;gap:4px;box-shadow:var(--soft-shadow);}' +
    '.mk-pip svg{width:13px;height:13px;}' +
    '.mk-pipcard{position:absolute;left:14px;top:40px;width:118px;background:#fff;border-radius:10px;box-shadow:var(--card-shadow);padding:7px;opacity:0;z-index:8;}' +
    '.mk-pipcard .ph{display:flex;align-items:center;gap:4px;margin-bottom:5px;}' +
    '.mk-pipcard .pe{flex:1;font-size:10px;font-weight:600;color:var(--text-body);}' +
    '.mk-pipcard .pmax{font-size:9.5px;color:var(--primary);border:1px solid var(--primary);border-radius:999px;padding:1px 7px;font-weight:600;}' +
    '.mk-pipcard .pclose{width:15px;height:15px;border-radius:50%;background:var(--surface-muted);display:flex;align-items:center;justify-content:center;font-size:10px;color:var(--text-secondary);}' +
    '.mk-pipvid{width:104px;height:65px;border-radius:5px;overflow:hidden;background:#16180f;position:relative;}' +
    '.mk-pipvid video{position:absolute;inset:0;width:100%;height:100%;object-fit:cover;}' +
    '.mk-hud{position:absolute;right:13px;top:44px;width:200px;background:#fff;border-radius:9px;padding:8px 10px;box-shadow:var(--card-shadow);opacity:0;}' +
    '.mk-hud .ht{font-size:10px;font-weight:700;color:var(--text-secondary);margin-bottom:5px;}' +
    '.mk-hud .hr{display:flex;align-items:center;gap:6px;margin-top:5px;}' +
    '.mk-hud .hl{font-size:10px;color:var(--text-body);width:22px;}' +
    '.mk-hud .hbar{flex:1;height:6px;border-radius:3px;background:var(--divider);position:relative;overflow:hidden;}' +
    '.mk-hud .hfill{position:absolute;left:0;top:0;bottom:0;width:0;border-radius:3px;}' +
    '.mk-hud .hv{font-size:9px;font-family:var(--font-mono);color:var(--text-secondary);width:66px;text-align:right;}' +
    '.mk-sheet{position:absolute;left:14px;right:14px;bottom:12px;background:#fff;border-radius:12px;padding:8px 11px;display:flex;align-items:center;gap:8px;box-shadow:var(--card-shadow);}' +
    '.mk-sheet .cr{font-size:11px;color:var(--text-secondary);}' +
    '.mk-sheet .sum{flex:1;font-size:10.5px;color:var(--text-body);white-space:nowrap;overflow:hidden;text-overflow:ellipsis;}' +
    '.mk-sheet .btn{font-size:10.5px;font-weight:600;border-radius:999px;padding:5px 12px;white-space:nowrap;}' +
    '.mk-sheet .btn.ghost{color:var(--primary);border:1px solid var(--primary);background:#fff;}' +
    '.mk-sheet .btn.fill{color:#fff;background:var(--primary);border:1px solid var(--primary);}' +
    /* —— 下半：算法原理面板 —— */
    '.alg-panel{position:absolute;background:var(--surface);border-radius:18px;box-shadow:var(--card-shadow);border:1px solid var(--divider);padding:15px 18px;opacity:0;will-change:transform,opacity;}' +
    '.alg-head{display:flex;align-items:baseline;gap:10px;}' +
    '.alg-no{font-family:var(--font-mono);font-size:23px;font-weight:800;color:var(--primary);letter-spacing:-.01em;}' +
    '.alg-t{font-size:21px;font-weight:700;color:var(--text-title);}' +
    '.alg-tag{margin-left:auto;font-size:13px;color:var(--text-secondary);background:var(--surface-muted);border-radius:999px;padding:3px 11px;font-family:var(--font-mono);}' +
    '.alg-canvas{margin-top:11px;height:262px;border-radius:12px;background:#fbfaf4;border:1px solid var(--divider);position:relative;overflow:hidden;}' +
    '.alg-canvas svg{position:absolute;inset:0;width:100%;height:100%;}' +
    '.alg-steps{margin-top:11px;}' +
    '.alg-step{font-size:15px;color:var(--text-body);margin-top:6px;display:flex;gap:8px;opacity:0;}' +
    '.alg-step .si{color:var(--primary);font-weight:800;flex:none;}'
  );

  /* ---- 三平板 mini ControlPage（未改：地图 MAPKIT + 顶栏/FAB/HUD/底卡/PiP） ---- */
  function regionG(cls, x0, y0, x1, y1, color) {
    const x = Math.min(x0, x1), y = Math.min(y0, y1), w = Math.abs(x1 - x0), h = Math.abs(y1 - y0), c = 13;
    const cor = function (cxp, cyp, dx, dy) { return '<path d="M' + (cxp + dx * c) + ' ' + cyp + ' L' + cxp + ' ' + cyp + ' L' + cxp + ' ' + (cyp + dy * c) + '" fill="none" stroke="' + color + '" stroke-width="3" stroke-linecap="round"/>'; };
    return '<g class="' + cls + '" opacity="0"><rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + h + '" fill="' + color + '" opacity=".10"/>' +
      '<rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + h + '" fill="none" stroke="' + color + '" stroke-width="2"/>' +
      cor(x, y, 1, 1) + cor(x + w, y, -1, 1) + cor(x, y + h, 1, -1) + cor(x + w, y + h, -1, -1) + '</g>';
  }
  function pinG(cls, x, y) {
    return '<g class="' + cls + '" transform="translate(' + x + ',' + y + ')" opacity="0"><path d="M0 0 Q-9 -14 -9 -23 A9 9 0 0 1 9 -23 Q9 -14 0 0 Z" fill="' + MK.TH.target + '"/><circle cy="-23" r="3.6" fill="#fff"/></g>';
  }
  function trailPair(thin, brush, color) {
    let s = '';
    if (brush) s += '<path class="' + brush + '" d="" fill="none" stroke="' + color + '" stroke-width="22" stroke-linecap="round" stroke-linejoin="round" opacity=".16" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>';
    s += '<path class="' + thin + '" d="" fill="none" stroke="' + color + '" stroke-width="3" stroke-linecap="round" stroke-linejoin="round" opacity=".9" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>';
    return s;
  }
  function svgRipple(tl, ov, x, y, at, color) {
    const ns = 'http://www.w3.org/2000/svg';
    const c = document.createElementNS(ns, 'circle');
    c.setAttribute('cx', x); c.setAttribute('cy', y); c.setAttribute('r', 3);
    c.setAttribute('fill', 'none'); c.setAttribute('stroke', color || MK.TH.region); c.setAttribute('stroke-width', '2');
    c.setAttribute('opacity', '0'); ov.appendChild(c);
    tl.fromTo(c, { attr: { r: 3 }, opacity: 0.8 }, { attr: { r: 22 }, opacity: 0, duration: 0.6, ease: 'power2.out' }, at);
  }
  function buildMini(mode, cx, withPip) {
    const cam = document.createElement('div'); cam.className = 'cam';
    const tab = document.createElement('div'); tab.className = 'mtab';
    tab.style.cssText = 'left:' + (cx - 280) + 'px;top:140px;width:560px;height:380px;';
    const screen = document.createElement('div'); screen.className = 'mscreen';
    const seed = mode === 'astar' ? 7 : (mode === 'cover' ? 23 : 42);
    const grid = MK.makeGrid(64, 42, seed);
    let ov = '';
    if (mode === 'astar') {
      ov = trailPair('mk-t1', null, G) + pinG('mk-pin', 410, 250) + MK.robotSVG('mk-r1', G, '1');
    } else if (mode === 'cover') {
      ov = regionG('mk-reg', 150, 200, 400, 312, MK.TH.region) + trailPair('mk-t1', 'mk-b1', G) + MK.robotSVG('mk-r1', G, '1');
    } else {
      ov = regionG('mk-reg1', 120, 200, 295, 312, G) + regionG('mk-reg2', 305, 200, 470, 312, B) +
        trailPair('mk-t1', 'mk-b1', G) + trailPair('mk-t2', 'mk-b2', B) + MK.robotSVG('mk-r1', G, '1') + MK.robotSVG('mk-r2', B, '2');
    }
    const md = mode === 'astar' ? '单机导航' : (mode === 'cover' ? '全路径覆盖' : '多机协同');
    const svg = '<svg class="mk-svg" viewBox="0 0 ' + PW + ' ' + PH + '"><g class="mk-map">' + MK.mapSVG(grid, PW, PH, 2) + '</g><g class="mk-ov">' + ov + '</g></svg>';
    const pipChrome = '<div class="mk-pip" data-pip>' + I.video + '<span>仪表视频</span></div>' +
      (withPip ? '<div class="mk-pipcard" data-pipcard><div class="ph"><span class="pe">实时仪表</span><span class="pmax" data-pmax>放大</span><span class="pclose">✕</span></div><div class="mk-pipvid"><video data-pvid autoplay loop muted playsinline src="' + VID + '"></video></div></div>' : '');
    let hud = '';
    if (mode === 'cover') hud = '<div class="mk-hud" data-hud><div class="ht">覆盖进度</div><div class="hr"><div class="hbar"><div class="hfill" data-h1 style="background:' + G + '"></div></div><div class="hv" data-hv1>0% · 0.0m²</div></div></div>';
    else if (mode === 'fleet') hud = '<div class="mk-hud" data-hud><div class="ht">覆盖进度</div><div class="hr"><div class="hl">车1</div><div class="hbar"><div class="hfill" data-h1 style="background:' + G + '"></div></div><div class="hv" data-hv1>0%·0.0m²</div></div><div class="hr"><div class="hl">车2</div><div class="hbar"><div class="hfill" data-h2 style="background:' + B + '"></div></div><div class="hv" data-hv2>0%·0.0m²</div></div></div>';
    const chrome =
      '<div class="mk-top"><div class="mk-pill"><span class="d"></span><span class="ip">192.168.43.12</span><span class="md">' + md + '</span><span class="cr">▾</span></div><div class="mk-rebuild">重新建图</div><div class="mk-vbtn">' + I.video + '</div></div>' +
      pipChrome +
      '<div class="mk-fabs"><div class="mk-fab">＋</div><div class="mk-fab">−</div><div class="mk-fab">⊙</div></div>' +
      hud +
      '<div class="mk-sheet"><span class="cr" data-cr>▴</span><span class="sum" data-sum></span><span class="btn ghost" data-btn>选目标点</span></div>';
    screen.innerHTML = svg + chrome;
    tab.appendChild(screen); cam.appendChild(tab);
    const q = function (sel) { return screen.querySelector(sel); };
    tab._refs = {
      ov: screen.querySelector('.mk-ov'),
      r1: q('.mk-r1'), r2: q('.mk-r2'), t1: q('.mk-t1'), t2: q('.mk-t2'), b1: q('.mk-b1'), b2: q('.mk-b2'),
      pin: q('.mk-pin'), reg: q('.mk-reg'), reg1: q('.mk-reg1'), reg2: q('.mk-reg2'),
      sum: q('[data-sum]'), btn: q('[data-btn]'), cr: q('[data-cr]'),
      hud: q('[data-hud]'), h1: q('[data-h1]'), hv1: q('[data-hv1]'), h2: q('[data-h2]'), hv2: q('[data-hv2]'),
      pip: q('[data-pip]'), pipcard: q('[data-pipcard]'), pmax: q('[data-pmax]'), pvid: q('[data-pvid]')
    };
    return { cam: cam, tab: tab };
  }
  function btnArm(tl, r, at, label) { setText(tl, at, r.btn, label, '选目标点'); tl.fromTo(r.btn, { scale: 0.94 }, { scale: 1, duration: 0.3, ease: 'back.out(2)' }, at); }
  function btnFill(tl, r, at) { tl.to(r.btn, { duration: 0.01, onComplete: function () { r.btn.classList.add('fill'); r.btn.classList.remove('ghost'); }, onReverseComplete: function () { r.btn.classList.remove('fill'); r.btn.classList.add('ghost'); } }, at); }
  function animMiniAstar(tl, a, t0, r) {
    setText(tl, t0, r.sum, '点「选目标点」再点地图选点', '');
    setText(tl, t0, r.btn, '选目标点', '选目标点');
    btnArm(tl, r, t0 + 0.4, '选目标点');
    svgRipple(tl, r.ov, 410, 250, t0 + 1.2, MK.TH.target);
    tl.to(r.pin, { opacity: 1, duration: 0.4, ease: 'back.out(1.5)' }, t0 + 1.5);
    setText(tl, t0 + 1.7, r.sum, '已选目标点 · 点「开始导航」', '点「选目标点」再点地图选点');
    btnArm(tl, r, t0 + 1.9, '开始导航'); btnFill(tl, r, t0 + 1.9);
    const pts = [[150, 165], [232, 165], [232, 250], [410, 250]];
    tl.set(r.t1, { attr: { d: MK.polyPath(pts) } }, t0 + 2.6);
    MK.driveRobot({ tl: tl, robot: r.r1, trail: r.t1, pts: pts, t0: t0 + 2.7, segDur: 1.05, turnDur: 0.3 });
    setText(tl, t0 + 2.9, r.sum, '自主导航中 · 沿规划路径行驶', '已选目标点 · 点「开始导航」');
  }
  function animMiniCover(tl, a, t0, r) {
    setText(tl, t0, r.sum, '点「选顶点」再点地图选 4 个顶点', '');
    setText(tl, t0, r.btn, '选顶点', '选目标点');
    btnArm(tl, r, t0 + 0.4, '选顶点');
    const corners = [[150, 312], [400, 312], [400, 200], [150, 200]];
    corners.forEach(function (c, i) { svgRipple(tl, r.ov, c[0], c[1], t0 + 0.9 + i * 0.42, MK.TH.region); });
    tl.to(r.reg, { opacity: 1, duration: 0.45 }, t0 + 2.7);
    setText(tl, t0 + 2.7, r.sum, '牛耕往复 · 一寸不漏', '点「选顶点」再点地图选 4 个顶点');
    btnArm(tl, r, t0 + 2.7, '停止'); btnFill(tl, r, t0 + 2.7);
    const sweep = MK.boustro(165, 214, 388, 300, 24);
    tl.set([r.b1, r.t1], { attr: { d: MK.polyPath(sweep) } }, t0 + 2.8);
    tl.to(r.hud, { opacity: 1, duration: 0.4 }, t0 + 2.8);
    MK.driveRobot({ tl: tl, robot: r.r1, trail: r.b1, pts: sweep, t0: t0 + 2.9, segDur: 0.42, turnDur: 0.16 });
    tl.to(r.t1, { attr: { 'stroke-dashoffset': 0 }, duration: (sweep.length - 1) * 0.58, ease: 'none' }, t0 + 3.06);
    tl.to(r.h1, { width: '88%', duration: (sweep.length - 1) * 0.58, ease: 'none' }, t0 + 2.9);
    tl.to({ v: 0 }, { v: 88, duration: (sweep.length - 1) * 0.58, ease: 'none', onUpdate: function () { r.hv1.textContent = Math.round(this.targets()[0].v) + '% · ' + (this.targets()[0].v * 0.07).toFixed(1) + 'm²'; } }, t0 + 2.9);
  }
  function animMiniFleet(tl, a, t0, r) {
    setText(tl, t0, r.sum, '车1：点「选点划区域」选 2 对角点', '');
    setText(tl, t0, r.btn, '选点划区域', '选目标点');
    btnArm(tl, r, t0 + 0.4, '选点划区域');
    svgRipple(tl, r.ov, 120, 200, t0 + 0.9, G); svgRipple(tl, r.ov, 295, 312, t0 + 1.35, G);
    tl.to(r.reg1, { opacity: 1, duration: 0.4 }, t0 + 1.7);
    setText(tl, t0 + 1.8, r.sum, '车2：点「选点划区域」选 2 对角点', '车1：点「选点划区域」选 2 对角点');
    svgRipple(tl, r.ov, 305, 200, t0 + 2.0, B); svgRipple(tl, r.ov, 470, 312, t0 + 2.45, B);
    tl.to(r.reg2, { opacity: 1, duration: 0.4 }, t0 + 2.8);
    btnArm(tl, r, t0 + 2.8, '停止'); btnFill(tl, r, t0 + 2.8);
    setText(tl, t0 + 2.9, r.sum, '双车分区并行 · 互不重叠', '车2：点「选点划区域」选 2 对角点');
    tl.to(r.hud, { opacity: 1, duration: 0.4 }, t0 + 2.9);
    const sw1 = MK.boustro(133, 214, 283, 300, 24), sw2 = MK.boustro(318, 214, 458, 300, 24);
    tl.set([r.b1], { attr: { d: MK.polyPath(sw1) } }, t0 + 3.0); tl.set([r.b2], { attr: { d: MK.polyPath(sw2) } }, t0 + 3.0);
    tl.set([r.t1], { attr: { d: MK.polyPath(sw1) } }, t0 + 3.0); tl.set([r.t2], { attr: { d: MK.polyPath(sw2) } }, t0 + 3.0);
    const dur1 = (sw1.length - 1) * 0.5;
    MK.driveRobot({ tl: tl, robot: r.r1, trail: r.b1, pts: sw1, t0: t0 + 3.0, segDur: 0.36, turnDur: 0.14 });
    MK.driveRobot({ tl: tl, robot: r.r2, trail: r.b2, pts: sw2, t0: t0 + 3.0, segDur: 0.36, turnDur: 0.14 });
    tl.to([r.t1, r.t2], { attr: { 'stroke-dashoffset': 0 }, duration: dur1, ease: 'none' }, t0 + 3.14);
    tl.to(r.h1, { width: '86%', duration: dur1, ease: 'none' }, t0 + 3.0);
    tl.to(r.h2, { width: '80%', duration: dur1, ease: 'none' }, t0 + 3.0);
    tl.to({ v: 0 }, { v: 86, duration: dur1, ease: 'none', onUpdate: function () { r.hv1.textContent = Math.round(this.targets()[0].v) + '%·' + (this.targets()[0].v * 0.06).toFixed(1) + 'm²'; } }, t0 + 3.0);
    tl.to({ v: 0 }, { v: 80, duration: dur1, ease: 'none', onUpdate: function () { r.hv2.textContent = Math.round(this.targets()[0].v) + '%·' + (this.targets()[0].v * 0.058).toFixed(1) + 'm²'; } }, t0 + 3.0);
  }

  /* ---- 下半：三套规划算法实时原理动画（A* / 牛耕 BCD / 生成树 STC） ---- */
  const CW = 404, CH = 262;
  function vizAstar() {
    const C = 11, R = 7, cs = 24, gx = (CW - C * cs) / 2, gy = (CH - R * cs) / 2;
    const cxy = function (c, r) { return [gx + c * cs + cs / 2, gy + r * cs + cs / 2]; };
    const obst = { '3,1': 1, '3,2': 1, '3,3': 1, '6,3': 1, '6,4': 1, '6,5': 1, '8,1': 1 };
    const start = [1, 5], goal = [9, 1];
    const free = function (c, r) { return c >= 0 && c < C && r >= 0 && r < R && !obst[c + ',' + r]; };
    const h = function (c, r) { return Math.abs(c - goal[0]) + Math.abs(r - goal[1]); };
    const open = [{ c: start[0], r: start[1], g: 0, f: h(start[0], start[1]) }], par = {}, closed = {}, order = [];
    while (open.length) {
      open.sort(function (a, b) { return a.f - b.f; });
      const n = open.shift(), k = n.c + ',' + n.r; if (closed[k]) continue; closed[k] = 1; order.push([n.c, n.r]);
      if (n.c === goal[0] && n.r === goal[1]) break;
      [[1, 0], [-1, 0], [0, 1], [0, -1]].forEach(function (d) {
        const nc = n.c + d[0], nr = n.r + d[1], nk = nc + ',' + nr; if (!free(nc, nr) || closed[nk]) return;
        if (!(nk in par)) par[nk] = k; open.push({ c: nc, r: nr, g: n.g + 1, f: n.g + 1 + h(nc, nr) });
      });
    }
    const path = []; let cur = goal.join(','); while (cur && cur !== start.join(',')) { const p = cur.split(',').map(Number); path.unshift(p); cur = par[cur]; } path.unshift(start);
    let s = '<g stroke="rgba(72,92,17,.10)" stroke-width="1">';
    for (let i = 0; i <= C; i++) s += '<line x1="' + (gx + i * cs) + '" y1="' + gy + '" x2="' + (gx + i * cs) + '" y2="' + (gy + R * cs) + '"/>';
    for (let i = 0; i <= R; i++) s += '<line x1="' + gx + '" y1="' + (gy + i * cs) + '" x2="' + (gx + C * cs) + '" y2="' + (gy + i * cs) + '"/>';
    s += '</g>';
    Object.keys(obst).forEach(function (k) { const p = k.split(',').map(Number); s += '<rect x="' + (gx + p[0] * cs) + '" y="' + (gy + p[1] * cs) + '" width="' + cs + '" height="' + cs + '" fill="#cfccbd"/>'; });
    let ex = ''; order.forEach(function (p) { ex += '<rect class="ax-ex" x="' + (gx + p[0] * cs + 1.5) + '" y="' + (gy + p[1] * cs + 1.5) + '" width="' + (cs - 3) + '" height="' + (cs - 3) + '" rx="3" fill="#7d9b3a" opacity="0"/>'; });
    s += '<g>' + ex + '</g>';
    let pd = 'M'; path.forEach(function (p, i) { const xy = cxy(p[0], p[1]); pd += (i ? 'L' : '') + xy[0] + ' ' + xy[1]; });
    s += '<path class="ax-path" d="' + pd + '" fill="none" stroke="#485c11" stroke-width="5" stroke-linecap="round" stroke-linejoin="round" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>';
    const sc = cxy(start[0], start[1]), gc = cxy(goal[0], goal[1]);
    s += '<circle class="ax-start" cx="' + sc[0] + '" cy="' + sc[1] + '" r="8" fill="#3f9352" opacity="0"/>';
    s += '<g class="ax-goal" opacity="0"><path transform="translate(' + gc[0] + ',' + gc[1] + ')" d="M0 -1 Q-8 -13 -8 -21 A8 8 0 0 1 8 -21 Q8 -13 0 -1 Z" fill="#d9503f"/><circle cx="' + gc[0] + '" cy="' + (gc[1] - 21) + '" r="3" fill="#fff"/></g>';
    s += '<circle class="ax-dot" cx="' + sc[0] + '" cy="' + sc[1] + '" r="6" fill="#485c11" stroke="#fff" stroke-width="2" opacity="0"/>';
    return { inner: s, mk: function (svg) { return { ex: Array.prototype.slice.call(svg.querySelectorAll('.ax-ex')), path: svg.querySelector('.ax-path'), dot: svg.querySelector('.ax-dot'), start: svg.querySelector('.ax-start'), goal: svg.querySelector('.ax-goal'), pts: path.map(function (p) { return cxy(p[0], p[1]); }) }; } };
  }
  function animAstar(tl, at, r, TOT) {            // TOT 统一 → 三算法同开同停
    TOT = TOT || 6.5;
    tl.fromTo(r.start, { opacity: 0, scale: 0, transformOrigin: '50% 50%' }, { opacity: 1, scale: 1, duration: 0.4, ease: 'back.out(2)' }, at);
    tl.fromTo(r.goal, { opacity: 0, y: -8 }, { opacity: 1, y: 0, duration: 0.4, ease: 'back.out(1.6)' }, at + 0.2);
    const fStart = at + 0.5, fDur = TOT * 0.6, step = fDur / Math.max(1, r.ex.length);
    r.ex.forEach(function (c, i) { tl.to(c, { opacity: 0.5, duration: Math.min(0.4, step * 3 + 0.1), ease: 'power1.out' }, fStart + i * step); });
    const pStart = at + TOT * 0.64, pDur = TOT * 0.32;
    tl.to(r.dot, { opacity: 1, duration: 0.2 }, pStart);
    tl.fromTo(r.path, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: pDur, ease: 'power2.inOut' }, pStart);
    const pts = r.pts, seg = pDur / Math.max(1, pts.length - 1); let dt = pStart;
    pts.forEach(function (p, i) { if (i === 0) { tl.set(r.dot, { attr: { cx: p[0], cy: p[1] } }, dt); return; } tl.to(r.dot, { attr: { cx: p[0], cy: p[1] }, duration: seg, ease: 'none' }, dt); dt += seg; });
  }
  function vizBcd() {
    const x0 = 44, x1 = 360, y0 = 40, y1 = 244, ob = { x: 172, y: 108, w: 60, h: 74 };
    function strips(xa, xb, st, ya, yb, down) { const p = []; for (let x = xa; x <= xb + 0.1; x += st) { if (down) p.push([x, ya], [x, yb]); else p.push([x, yb], [x, ya]); down = !down; } return { p: p, down: down }; }
    const sL = strips(58, 158, 22, y0 + 8, y1 - 8, true);
    const sT = strips(186, 222, 18, y0 + 8, 100, sL.down);
    const sB = strips(186, 222, 18, 192, y1 - 8, sT.down);
    const sR = strips(246, 348, 22, y0 + 8, y1 - 8, sB.down);
    const pts = [].concat(sL.p, sT.p, sB.p, sR.p);
    let pd = 'M'; pts.forEach(function (p, i) { pd += (i ? 'L' : '') + p[0] + ' ' + p[1]; });
    let s = '<rect class="bc-region" x="' + x0 + '" y="' + y0 + '" width="' + (x1 - x0) + '" height="' + (y1 - y0) + '" rx="6" fill="none" stroke="#aeb977" stroke-width="2" opacity="0"/>';
    s += '<rect class="bc-cell" x="' + x0 + '" y="' + y0 + '" width="' + (172 - x0) + '" height="' + (y1 - y0) + '" fill="#7d9b3a" opacity="0"/>';
    s += '<rect class="bc-cell" x="172" y="' + y0 + '" width="60" height="' + (108 - y0) + '" fill="#dfa32f" opacity="0"/>';
    s += '<rect class="bc-cell" x="172" y="182" width="60" height="' + (y1 - 182) + '" fill="#dfa32f" opacity="0"/>';
    s += '<rect class="bc-cell" x="232" y="' + y0 + '" width="' + (x1 - 232) + '" height="' + (y1 - y0) + '" fill="#4678b8" opacity="0"/>';
    s += '<rect class="bc-ob" x="' + ob.x + '" y="' + ob.y + '" width="' + ob.w + '" height="' + ob.h + '" rx="4" fill="#5b5e4c" opacity="0"/>';
    s += '<path class="bc-brush" d="' + pd + '" fill="none" stroke="#485c11" stroke-width="17" stroke-linecap="round" stroke-linejoin="round" opacity=".14" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>';
    s += '<path class="bc-thin" d="' + pd + '" fill="none" stroke="#485c11" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round" opacity=".85" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>';
    s += '<circle class="bc-dot" cx="' + pts[0][0] + '" cy="' + pts[0][1] + '" r="6" fill="#485c11" stroke="#fff" stroke-width="2" opacity="0"/>';
    return { inner: s, mk: function (svg) { return { region: svg.querySelector('.bc-region'), cells: Array.prototype.slice.call(svg.querySelectorAll('.bc-cell')), ob: svg.querySelector('.bc-ob'), brush: svg.querySelector('.bc-brush'), thin: svg.querySelector('.bc-thin'), dot: svg.querySelector('.bc-dot'), pts: pts }; } };
  }
  function animBcd(tl, at, r, TOT) {
    TOT = TOT || 6.5;
    tl.fromTo(r.region, { opacity: 0 }, { opacity: 1, duration: 0.5 }, at);
    tl.fromTo(r.ob, { opacity: 0, scale: 0.8, transformOrigin: '50% 50%' }, { opacity: 1, scale: 1, duration: 0.45, ease: 'back.out(1.5)' }, at + 0.4);
    const cStart = at + 0.7, cStep = (TOT * 0.26) / Math.max(1, r.cells.length);
    r.cells.forEach(function (c, i) { tl.fromTo(c, { opacity: 0 }, { opacity: 0.16, duration: 0.5, ease: 'power2.out' }, cStart + i * cStep); });
    const sw = at + TOT * 0.36, dur = TOT * 0.6;
    tl.to(r.dot, { opacity: 1, duration: 0.2 }, sw);
    tl.fromTo(r.brush, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: dur, ease: 'none' }, sw);
    tl.fromTo(r.thin, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: dur, ease: 'none' }, sw);
    const pts = r.pts, seg = dur / Math.max(1, pts.length - 1); let dt = sw;
    pts.forEach(function (p, i) { if (i === 0) { tl.set(r.dot, { attr: { cx: p[0], cy: p[1] } }, dt); return; } tl.to(r.dot, { attr: { cx: p[0], cy: p[1] }, duration: seg, ease: 'none' }, dt); dt += seg; });
  }
  function vizStc() {
    const C = 5, R = 4, cs = 50, gx = (CW - C * cs) / 2, gy = (CH - R * cs) / 2;
    const cc = function (c, r) { return [gx + c * cs + cs / 2, gy + r * cs + cs / 2]; };
    const seen = { '0,0': 1 }, q = [[0, 0]], edges = [];
    while (q.length) {
      const n = q.shift();
      [[1, 0], [0, 1], [-1, 0], [0, -1]].forEach(function (d) {
        const nc = n[0] + d[0], nr = n[1] + d[1], k = nc + ',' + nr;
        if (nc < 0 || nc >= C || nr < 0 || nr >= R || seen[k]) return; seen[k] = 1; edges.push([[n[0], n[1]], [nc, nr]]); q.push([nc, nr]);
      });
    }
    const spiral = []; let top = 0, bot = R - 1, lf = 0, rt = C - 1;
    while (top <= bot && lf <= rt) {
      for (let c = lf; c <= rt; c++) spiral.push([c, top]); top++;
      for (let r = top; r <= bot; r++) spiral.push([rt, r]); rt--;
      if (top <= bot) { for (let c = rt; c >= lf; c--) spiral.push([c, bot]); bot--; }
      if (lf <= rt) { for (let r = bot; r >= top; r--) spiral.push([lf, r]); lf++; }
    }
    let s = '';
    for (let r = 0; r < R; r++) for (let c = 0; c < C; c++) s += '<rect class="st-cell" x="' + (gx + c * cs + 2) + '" y="' + (gy + r * cs + 2) + '" width="' + (cs - 4) + '" height="' + (cs - 4) + '" rx="6" fill="#eef2dd" stroke="#cfccbd" stroke-width="1" opacity="0"/>';
    let eg = ''; edges.forEach(function (e) { const a = cc(e[0][0], e[0][1]), b = cc(e[1][0], e[1][1]); eg += '<line class="st-edge" x1="' + a[0] + '" y1="' + a[1] + '" x2="' + b[0] + '" y2="' + b[1] + '" stroke="#485c11" stroke-width="3.5" stroke-linecap="round" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>'; });
    s += '<g>' + eg + '</g>';
    const rc = cc(0, 0); s += '<circle class="st-root" cx="' + rc[0] + '" cy="' + rc[1] + '" r="7" fill="#3f9352" opacity="0"/>';
    let pd = 'M'; spiral.forEach(function (p, i) { const xy = cc(p[0], p[1]); pd += (i ? 'L' : '') + xy[0] + ' ' + xy[1]; });
    s += '<path class="st-brush" d="' + pd + '" fill="none" stroke="#4678b8" stroke-width="15" stroke-linecap="round" stroke-linejoin="round" opacity=".14" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>';
    s += '<path class="st-path" d="' + pd + '" fill="none" stroke="#4678b8" stroke-width="2.4" stroke-linecap="round" stroke-linejoin="round" opacity=".9" pathLength="1" stroke-dasharray="1" stroke-dashoffset="1"/>';
    const s0 = cc(spiral[0][0], spiral[0][1]); s += '<circle class="st-dot" cx="' + s0[0] + '" cy="' + s0[1] + '" r="6" fill="#4678b8" stroke="#fff" stroke-width="2" opacity="0"/>';
    return { inner: s, mk: function (svg) { return { cells: Array.prototype.slice.call(svg.querySelectorAll('.st-cell')), edges: Array.prototype.slice.call(svg.querySelectorAll('.st-edge')), root: svg.querySelector('.st-root'), brush: svg.querySelector('.st-brush'), path: svg.querySelector('.st-path'), dot: svg.querySelector('.st-dot'), pts: spiral.map(function (p) { return cc(p[0], p[1]); }) }; } };
  }
  function animStc(tl, at, r, TOT) {
    TOT = TOT || 6.5;
    const cDur = TOT * 0.16, cStep = cDur / Math.max(1, r.cells.length);
    r.cells.forEach(function (c, i) { tl.fromTo(c, { opacity: 0, scale: 0.8, transformOrigin: '50% 50%' }, { opacity: 1, scale: 1, duration: 0.4, ease: 'back.out(1.4)' }, at + i * cStep); });
    const tc = at + cDur + 0.15;
    tl.fromTo(r.root, { opacity: 0, scale: 0, transformOrigin: '50% 50%' }, { opacity: 1, scale: 1, duration: 0.4, ease: 'back.out(2)' }, tc);
    const tDur = TOT * 0.32, eStep = tDur / Math.max(1, r.edges.length);
    r.edges.forEach(function (e, i) { tl.fromTo(e, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: Math.min(0.4, eStep * 2 + 0.1), ease: 'power2.out' }, tc + i * eStep); });
    const tt = at + TOT * 0.54, dur = TOT * 0.42;
    tl.to(r.dot, { opacity: 1, duration: 0.2 }, tt);
    tl.fromTo(r.brush, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: dur, ease: 'none' }, tt);
    tl.fromTo(r.path, { attr: { 'stroke-dashoffset': 1 } }, { attr: { 'stroke-dashoffset': 0 }, duration: dur, ease: 'none' }, tt);
    const pts = r.pts, seg = dur / Math.max(1, pts.length - 1); let dt = tt;
    pts.forEach(function (p, i) { if (i === 0) return; tl.to(r.dot, { attr: { cx: p[0], cy: p[1] }, duration: seg, ease: 'none' }, dt); dt += seg; });
  }
  function buildAlgPanel(o, vizFn) {
    const panel = document.createElement('div'); panel.className = 'alg-panel';
    panel.style.cssText = 'left:' + o.left + 'px;top:412px;width:440px;';
    panel.innerHTML = '<div class="alg-head"><span class="alg-no">' + o.no + '</span><span class="alg-t">' + o.title + '</span><span class="alg-tag">' + o.tag + '</span></div>' +
      '<div class="alg-canvas"></div>' +
      '<div class="alg-steps">' + o.steps.map(function (st) { return '<div class="alg-step"><span class="si">·</span><span>' + st + '</span></div>'; }).join('') + '</div>';
    const v = vizFn(), canvas = panel.querySelector('.alg-canvas');
    canvas.innerHTML = '<svg viewBox="0 0 ' + CW + ' ' + CH + '" preserveAspectRatio="xMidYMid meet">' + v.inner + '</svg>';
    return { panel: panel, steps: Array.prototype.slice.call(panel.querySelectorAll('.alg-step')), viz: v.mk(canvas.querySelector('svg')) };
  }

  function buildTrio(root) {
    root.classList.add('paper');
    const L = buildMini('astar', 370, false), M = buildMini('cover', 960, true), R = buildMini('fleet', 1550, false);
    root.appendChild(L.cam); root.appendChild(R.cam); root.appendChild(M.cam);
    const P1 = buildAlgPanel({ left: 150, no: 'A*', title: '单机最短路', tag: 'graph search', steps: ['每步取 f = g + h 最小，带方向感扩散', '回溯 parent 得最短路 · 代价分层离墙更远'] }, vizAstar);
    const P2 = buildAlgPanel({ left: 740, no: '牛耕', title: '全覆盖 BCD', tag: 'boustrophedon', steps: ['障碍把区域切成简单 cell（分解）', 'cell 内弓字往返 · 一寸不漏 · 零重复'] }, vizBcd);
    const P3 = buildAlgPanel({ left: 1330, no: '生成树', title: 'STC 覆盖', tag: 'spanning tree', steps: ['对栅格生成最小生成树（绿色枝）', '沿树螺旋遍历 · 完备全覆盖（蓝）'] }, vizStc);
    root.appendChild(P1.panel); root.appendChild(P2.panel); root.appendChild(P3.panel);
    return { L: L, M: M, R: R, P1: P1, P2: P2, P3: P3 };
  }
  function animTrio(tl, s) {
    const a = s.start, r = s.refs, L = r.L, M = r.M, R = r.R;
    const SC = 0.72, ty = 232 - SC * 330;
    initCam(tl, L.cam, a, SC, 370 * (1 - SC), ty); initCam(tl, M.cam, a, SC, 960 * (1 - SC), ty); initCam(tl, R.cam, a, SC, 1550 * (1 - SC), ty);
    tl.fromTo(M.tab, { opacity: 0, y: 30, scale: 0.94, filter: 'blur(10px)' }, { opacity: 1, y: 0, scale: 1, filter: 'blur(0px)', duration: 0.9, ease: 'power4.out' }, a + 0.4);
    tl.fromTo(L.tab, { opacity: 0, x: -46, filter: 'blur(9px)' }, { opacity: 1, x: 0, filter: 'blur(0px)', duration: 0.8, ease: 'power3.out' }, a + 0.6);
    tl.fromTo(R.tab, { opacity: 0, x: 46, filter: 'blur(9px)' }, { opacity: 1, x: 0, filter: 'blur(0px)', duration: 0.8, ease: 'power3.out' }, a + 0.8);
    caption(tl, a + 0.5, 2.2, '<b>一套界面，三种规划</b>', '控制页裂成三块 · 单机 / 全覆盖 / 多机');
    animMiniAstar(tl, a, a + 1.4, L.tab._refs); animMiniCover(tl, a, a + 1.4, M.tab._refs); animMiniFleet(tl, a, a + 1.4, R.tab._refs);
    // 出现约 1s → 平板变淡，下半算法面板出 + 三算法同开同停
    const algAt = a + 2.2, TOT = 6.6;
    tl.to([L.tab, M.tab, R.tab], { opacity: 0.46, duration: 0.6, ease: 'power2.inOut' }, algAt - 0.4);
    [r.P1, r.P2, r.P3].forEach(function (P, i) { tl.fromTo(P.panel, { opacity: 0, y: 32, filter: 'blur(9px)' }, { opacity: 1, y: 0, filter: 'blur(0px)', duration: 0.7, ease: 'power3.out' }, algAt - 0.4 + i * 0.12); });
    caption(tl, algAt + 0.2, 3.8, '<b>支撑它们的三套自研规划算法</b>', 'A* 带方向感最短路 · 牛耕一寸不漏 · 生成树完备覆盖');
    animAstar(tl, algAt, r.P1.viz, TOT); animBcd(tl, algAt, r.P2.viz, TOT); animStc(tl, algAt, r.P3.viz, TOT);
    [r.P1, r.P2, r.P3].forEach(function (P) { P.steps.forEach(function (st, i) { tl.fromTo(st, { opacity: 0, x: -8 }, { opacity: 1, x: 0, duration: 0.5, ease: 'power3.out' }, algAt + 0.5 + i * 0.42); }); });
    // 结尾转场：两侧平板 + 三面板移出，中间平板放大移到正中
    const te = algAt + TOT + 0.5, mr = M.tab._refs;
    tl.to(L.tab, { opacity: 0, x: -160, filter: 'blur(8px)', duration: 0.7, ease: 'power2.in' }, te);
    tl.to(R.tab, { opacity: 0, x: 160, filter: 'blur(8px)', duration: 0.7, ease: 'power2.in' }, te);
    tl.to([r.P1.panel, r.P2.panel, r.P3.panel], { opacity: 0, y: 56, filter: 'blur(8px)', duration: 0.7, ease: 'power2.in' }, te);
    tl.to(M.tab, { opacity: 1, duration: 0.5 }, te);
    moveCam(tl, M.cam, te + 0.15, 1.85, -816, -70, 1.1);   // 中间平板放大到 ~1036px 居中（之前 1.1 太小）
    tl.set(M.cam, { willChange: 'auto' }, te + 1.35);      // 放大到位后取消 GPU 层缓存 → 浏览器按终态重栅格化，消除上采样糊
    // 点摄像头 → 放大 → 仪表页
    const tc = te + 1.7;
    const sp = F.screenOfEl(mr.pip);
    cursorShow(tl, tc + 0.1, sp.x, sp.y + 46);
    tapEl(tl, tc + 0.7, mr.pip, 0.6);
    tl.to(mr.pip, { opacity: 0, duration: 0.3 }, tc + 1.3);
    tl.fromTo(mr.pipcard, { opacity: 0, scale: 0.9, transformOrigin: '0 0' }, { opacity: 1, scale: 1, duration: 0.5, ease: 'back.out(1.4)' }, tc + 1.4);
    tapEl(tl, tc + 2.9, mr.pmax, 0.6);
    cursorHide(tl, tc + 3.7);
  }

  F.addScene({ id: 'algo-trio', dur: 15.5, build: buildTrio, anim: animTrio });
})();
