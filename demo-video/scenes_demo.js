/* scenes_demo.js —— 片尾「实机演示」章：整屏播放 assets 的真车演示视频（用户 2026-06-17 放入 实机演示.mp4）。
   视频已转码为 ASCII 名 + faststart 的 `assets/demo_real.mp4`（1920×1080 · 30fps · 53.9s · 去音轨）。
   播放方式：把 video.currentTime 绑定到 GSAP 主时间轴（与全片一致：确定性、可 __seek、shot.mjs 可截、可逐帧导出）——
   正常播放时随时间轴前进 ≈ 实时播放；不靠 autoplay（autoplay 会在 load 期就开播，到片尾这一章时已播完/错位）。
   依赖 window.FILM（引擎已先加载）+ window.FX（boot 时取 window.FX）。
   ⚠ 本挂载 file-tool Edit 会截断——改本文件只用 Write/bash 整文件覆盖。片序在 scenes7.js setOrder（置于末尾）。 */
(function () {
  "use strict";
  const F = window.FILM;
  const caption = F.caption, addCSS = F.addCSS, STAGE_W = F.STAGE_W, STAGE_H = F.STAGE_H;
  const SRC = 'assets/demo_real.mp4';
  const VID = 53.9;                       // 视频真实时长（ffprobe）

  addCSS(
    '.dm{position:absolute;inset:0;}' +
    '.dm-frame{position:absolute;left:96px;top:54px;width:1728px;height:972px;border-radius:22px;overflow:hidden;background:#0d0e0b;box-shadow:0 50px 120px -36px rgba(20,22,14,.7),0 12px 30px -14px rgba(0,0,0,.5);}' +
    '.dm-vid{position:absolute;inset:0;width:100%;height:100%;object-fit:cover;display:block;background:#0d0e0b;}' +
    /* 顶部标题条（前几秒后淡出，让画面无遮挡）*/
    '.dm-intro{position:absolute;left:0;right:0;top:0;padding:34px 44px 70px;background:linear-gradient(180deg,rgba(13,14,11,.62),rgba(13,14,11,0));opacity:0;}' +
    '.dm-kick{font-family:var(--font-mono);font-size:21px;font-weight:600;letter-spacing:.3em;color:rgba(255,255,255,.82);}' +
    '.dm-title{font-size:50px;font-weight:800;color:#fff;letter-spacing:-.02em;margin-top:10px;text-shadow:0 2px 18px rgba(0,0,0,.5);}' +
    '.dm-title b{color:#cde07a;}' +
    /* 右上「实机录制」徽标（常驻，红点呼吸）*/
    '.dm-badge{position:absolute;right:34px;top:34px;display:flex;align-items:center;gap:11px;background:rgba(13,14,11,.5);border:1px solid rgba(255,255,255,.22);border-radius:999px;padding:11px 20px;opacity:0;backdrop-filter:blur(2px);}' +
    '.dm-badge .ld{width:13px;height:13px;border-radius:50%;background:#ff4d40;box-shadow:0 0 10px rgba(255,77,64,.8);animation:dmpulse 1.4s ease-in-out infinite;}' +
    '.dm-badge .lt{font-size:20px;font-weight:700;color:#fff;letter-spacing:.04em;}' +
    '@keyframes dmpulse{0%,100%{opacity:1;transform:scale(1);}50%{opacity:.3;transform:scale(.68);}}' +
    /* 底部进度条（随视频时间填充，强化"正在播放"）*/
    '.dm-prog{position:absolute;left:0;right:0;bottom:0;height:6px;background:rgba(255,255,255,.16);}' +
    '.dm-progf{position:absolute;left:0;top:0;bottom:0;width:100%;background:#cde07a;transform:scaleX(0);transform-origin:0 50%;}'
  );

  function buildDemo(root) {
    root.classList.add('paper');
    const wrap = document.createElement('div'); wrap.className = 'dm'; root.appendChild(wrap);
    const frame = document.createElement('div'); frame.className = 'dm-frame';
    frame.innerHTML =
      '<video class="dm-vid" muted playsinline preload="auto" src="' + SRC + '"></video>' +
      '<div class="dm-intro"><div class="dm-kick">实机演示 · LIVE DEMO</div><div class="dm-title">真车跑通<b>巡检全流程</b></div></div>' +
      '<div class="dm-badge"><span class="ld"></span><span class="lt">实机录制</span></div>' +
      '<div class="dm-prog"><div class="dm-progf"></div></div>';
    wrap.appendChild(frame);
    return { wrap: wrap, frame: frame, vid: frame.querySelector('.dm-vid'), intro: frame.querySelector('.dm-intro'), badge: frame.querySelector('.dm-badge'), progf: frame.querySelector('.dm-progf') };
  }

  function animDemo(tl, s) {
    const a = s.start, r = s.refs, FX = window.FX;
    FX.enter(tl, r.frame, a + 0.1, 'zoomBlur', { dur: 0.9, from: 0.9, origin: '50% 50%' });
    FX.enter(tl, r.intro, a + 0.5, 'fall', { dur: 0.7 });
    FX.enter(tl, r.badge, a + 0.7, 'left', { dur: 0.6, dist: 24 });
    // 标题条几秒后淡出，画面留白
    FX.exit(tl, r.intro, a + 5.0, 'up', 0.7);

    // 视频帧绑定主时间轴（确定性、可 seek）
    const o = { t: 0 };
    tl.to(o, {
      t: VID, duration: VID, ease: 'none',
      onUpdate: function () {
        const v = r.vid; if (!v) return;
        if (v.readyState >= 1) { const nt = o.t; if (Math.abs((v.currentTime || 0) - nt) > 0.034) { try { v.currentTime = nt; } catch (e) {} } }
      },
      onReverseComplete: function () { try { if (r.vid) r.vid.currentTime = 0; } catch (e) {} }
    }, a);
    // 进度条随视频填充
    tl.fromTo(r.progf, { scaleX: 0 }, { scaleX: 1, duration: VID, ease: 'none' }, a);

    // 字幕：开场叙述 + 收尾
    caption(tl, a + 0.7, 4.0, '<b>实机演示 · 真车跑通全流程</b>', '建图 → 规划 → 全覆盖 → 仪表识别，真机一镜到底');
    caption(tl, a + VID - 6.5, 5.0, '<b>从平板一键下发，到真车自主完成</b>', '软硬全栈国产自主 · 零 ROS 依赖');
  }

  F.addScene({ id: 'demo-real', dur: VID + 0.6, build: buildDemo, anim: animDemo });
})();
