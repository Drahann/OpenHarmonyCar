const metricsEl = document.getElementById('metrics');
const imgA = document.getElementById('stream-a');
const hudA = document.getElementById('hud-a');
const readingEl = document.getElementById('reading');
const scaleEl = document.getElementById('scale');
const statusEl = document.getElementById('status');
const timelineEl = document.getElementById('timeline');
const llmStatusEl = document.getElementById('llm-status');
const dialogEl = document.getElementById('msgs-deepseek');
const promptEl = document.getElementById('prompt-deepseek');
const askBtn = document.getElementById('ask');
const lowEl = document.getElementById('low');
const highEl = document.getElementById('high');
const saveBtn = document.getElementById('save');

// 移除视频控制元素（摄像头自动运行）

async function saveThresholds() {
  const low = parseFloat(lowEl.value || '0');
  const high = parseFloat(highEl.value || '100');
  
  // 添加加载状态
  saveBtn.disabled = true;
  const originalText = saveBtn.textContent;
  saveBtn.textContent = '保存中...';
  
  try {
    const response = await fetch('/api/thresholds', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ low, high })
    });
    
    if (response.ok) {
      // 成功提示
      saveBtn.textContent = '已应用';
      saveBtn.style.background = 'linear-gradient(135deg, #10b981, #059669)';
      
      // 2秒后恢复
      setTimeout(() => {
        saveBtn.textContent = originalText;
        saveBtn.style.background = '';
        saveBtn.disabled = false;
      }, 2000);
    } else {
      saveBtn.textContent = '保存失败';
      saveBtn.style.background = 'linear-gradient(135deg, #ef4444, #dc2626)';
      setTimeout(() => {
        saveBtn.textContent = originalText;
        saveBtn.style.background = '';
        saveBtn.disabled = false;
      }, 2000);
    }
  } catch (err) {
    console.error('保存阈值失败:', err);
    saveBtn.textContent = '网络错误';
    saveBtn.style.background = 'linear-gradient(135deg, #ef4444, #dc2626)';
    setTimeout(() => {
      saveBtn.textContent = originalText;
      saveBtn.style.background = '';
      saveBtn.disabled = false;
    }, 2000);
  }
}

saveBtn.addEventListener('click', saveThresholds);

function renderMetrics(metrics) {
  metricsEl.innerHTML = '';
  const entries = [
    ['实际 FPS', metrics.fps?.toFixed(1) ?? '--'],
    ['推理耗时', metrics.latency_ms ? `${metrics.latency_ms.toFixed(1)} ms` : '--'],
    ['当前帧号', metrics.frame_id ?? '-'],
  ];
  for (const [label, value] of entries) {
    const dt = document.createElement('dt');
    const dd = document.createElement('dd');
    dt.textContent = label;
    dd.textContent = value;
    metricsEl.appendChild(dt);
    metricsEl.appendChild(dd);
  }
}

function pushTimeline(evt) {
  const li = document.createElement('li');
  const timeStr = new Date(evt.ts * 1000).toLocaleTimeString('zh-CN', { 
    hour: '2-digit', 
    minute: '2-digit', 
    second: '2-digit' 
  });
  li.textContent = `[${timeStr}] ${evt.event} → ${evt.value ?? ''}`;
  timelineEl.prepend(li);
  while (timelineEl.children.length > 20) {
    timelineEl.removeChild(timelineEl.lastChild);
  }
}

async function pollSummary() {
  try {
    const r = await fetch('/api/summary');
    const j = await r.json();
    renderMetrics(j);
    
    // 支持多个仪表的读数显示
    if (j.readings && j.readings.length > 0) {
      if (j.readings.length === 1) {
        // 单个仪表
        const reading = j.readings[0];
        readingEl.textContent = `${reading.value.toFixed(2)} ${reading.unit}`;
        scaleEl.textContent = reading.label;
        statusEl.textContent = reading.status;
        statusEl.classList.toggle('alert', reading.status === 'ALARM');
      } else {
        // 多个仪表
        const readingsText = j.readings.map(r => 
          `#${r.gauge_id}: ${r.value.toFixed(1)}%`
        ).join(' | ');
        readingEl.textContent = readingsText;
        
        // 状态：如果有任何一个报警，显示 ALARM
        const hasAlarm = j.readings.some(r => r.status === 'ALARM');
        const alarmCount = j.readings.filter(r => r.status === 'ALARM').length;
        
        if (hasAlarm) {
          statusEl.textContent = `ALARM (${alarmCount}/${j.readings.length})`;
          statusEl.classList.add('alert');
        } else {
          statusEl.textContent = `NORMAL (${j.readings.length})`;
          statusEl.classList.remove('alert');
        }
        
        scaleEl.textContent = `${j.readings.length} 个仪表`;
      }
    } else {
      // 无仪表
      readingEl.textContent = '--';
      scaleEl.textContent = '';
      statusEl.textContent = 'NO DATA';
      statusEl.classList.remove('alert');
    }
  } catch (e) {}
  setTimeout(pollSummary, 1000);
}

pollSummary();

const conversation = [];

// Timeline 防抖和去重
let lastGaugeReading = null;
let lastTimelineUpdate = 0;
const TIMELINE_UPDATE_INTERVAL = 3000; // 3秒内不重复添加

function appendMessage(role, text) {
  const div = document.createElement('div');
  div.className = `msg ${role}`;
  div.textContent = text;
  dialogEl.appendChild(div);
  dialogEl.scrollTop = dialogEl.scrollHeight;
}

async function refreshLLMState() {
  try {
    const r = await fetch('/api/llm/state');
    const j = await r.json();
    if (!j.available) {
      llmStatusEl.textContent = 'DeepSeek 不可用';
      askBtn.disabled = true;
    } else {
      llmStatusEl.textContent = `${j.model} · ${j.status}`;
      askBtn.disabled = false;
    }
  } catch (err) {
    llmStatusEl.textContent = '状态获取失败';
  }
}

refreshLLMState();

async function sendPrompt() {
  const text = promptEl.value.trim();
  if (!text) return;
  promptEl.value = '';
  appendMessage('user', text);
  
  // 创建一个占位div用于流式更新
  const assistantDiv = document.createElement('div');
  assistantDiv.className = 'msg assistant';
  assistantDiv.textContent = '思考中...';
  dialogEl.appendChild(assistantDiv);
  dialogEl.scrollTop = dialogEl.scrollHeight;
  
  askBtn.disabled = true;
  const originalBtnText = askBtn.textContent;
  askBtn.textContent = '生成中...';
  llmStatusEl.textContent = '生成中...';
  
  try {
    const response = await fetch('/api/llm/query', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ message: text, history: conversation })
    });
    
    if (!response.ok) {
      assistantDiv.textContent = '服务暂时不可用，请稍后重试。';
      llmStatusEl.textContent = '错误';
      askBtn.disabled = false;
      askBtn.textContent = originalBtnText;
      return;
    }
    
    // SSE流式处理
    const reader = response.body.getReader();
    const decoder = new TextDecoder();
    let buffer = '';
    let fullReply = '';
    let tokenCount = 0;
    
    while (true) {
      const {done, value} = await reader.read();
      if (done) break;
      
      buffer += decoder.decode(value, {stream: true});
      const lines = buffer.split('\n');
      buffer = lines.pop(); // 保留不完整的行
      
      for (const line of lines) {
        if (line.startsWith('data: ')) {
          try {
            const jsonStr = line.substring(6).trim();
            if (!jsonStr) {
              continue; // 跳过空数据
            }
            
            const data = JSON.parse(jsonStr);
            
            if (data.done) {
              // 生成完成
              fullReply = data.reply || fullReply;
              updateAssistantMessage(fullReply, true);
              conversation.push({ user: text, assistant: fullReply });
              llmStatusEl.textContent = `完成 (${data.tokens_generated || tokenCount} tokens, 平均 ${(data.avg_time_per_token || 0).toFixed(2)}s/token)`;
            } else {
              // 流式更新
              fullReply = data.reply || '';
              updateAssistantMessage(fullReply, false);
              tokenCount++;
              
              // 更新状态显示
              llmStatusEl.textContent = `生成中... (${tokenCount} tokens)`;
              
              // 自动滚动到底部
              dialogEl.scrollTop = dialogEl.scrollHeight;
            }
          } catch (e) {
            console.error('Parse SSE error:', e, 'Line:', line);
            // 显示更详细的错误信息
            if (line.length > 0) {
              console.error('Failed to parse:', line.substring(0, 100));
            }
          }
        }
      }
    }
    
    // 处理消息显示的函数（支持 think 标签折叠）
    function updateAssistantMessage(text, isDone) {
      // 调试：显示收到的文本前100个字符
      console.log('[Think Debug] 收到文本:', text.substring(0, 100), '...');
      
      // DeepSeek-R1 特性：只有 </think> 结束标签，没有 <think> 开始标签
      // 思考内容从开头开始，到 </think> 结束
      const thinkEndIndex = text.indexOf('</think>');
      
      console.log('[Think Debug] </think> 位置:', thinkEndIndex);
      
      if (thinkEndIndex !== -1) {
        // 找到 </think> 标签
        const thinkText = text.substring(0, thinkEndIndex).trim();
        const afterThink = text.substring(thinkEndIndex + 8).trim(); // 8 = '</think>'.length
        
        console.log('[Think Debug] 思考内容长度:', thinkText.length, '回答内容长度:', afterThink.length);
        
        // 清空并重建内容
        assistantDiv.innerHTML = '';
        
        // 创建 think 折叠容器
        const thinkContainer = document.createElement('div');
        thinkContainer.className = 'think-container';
        
        const thinkHeader = document.createElement('div');
        thinkHeader.className = 'think-header';
        
        const toggle = document.createElement('span');
        toggle.className = 'think-toggle collapsed';  // 默认折叠
        toggle.textContent = '▶';
        
        const label = document.createElement('span');
        label.textContent = '思考过程';
        
        thinkHeader.appendChild(toggle);
        thinkHeader.appendChild(label);
        
        const thinkContentDiv = document.createElement('div');
        thinkContentDiv.className = 'think-content collapsed';  // 默认折叠
        thinkContentDiv.textContent = thinkText;
        
        thinkContainer.appendChild(thinkHeader);
        thinkContainer.appendChild(thinkContentDiv);
        
        // 添加点击事件
        thinkHeader.addEventListener('click', () => {
          const isCollapsed = thinkContentDiv.classList.toggle('collapsed');
          toggle.classList.toggle('collapsed');
          toggle.textContent = isCollapsed ? '▶' : '▼';
        });
        
        assistantDiv.appendChild(thinkContainer);
        
        // 添加真正的回答内容
        if (afterThink) {
          const answerDiv = document.createElement('div');
          answerDiv.className = 'answer-content';
          answerDiv.textContent = afterThink;
          assistantDiv.appendChild(answerDiv);
        }
      } else {
        // 还没有 </think> 标签，显示正在思考或直接显示内容
        if (isDone) {
          // 已完成但没有 think 标签，直接显示
          assistantDiv.textContent = text || '(无响应)';
        } else {
          // 生成中，显示当前内容
          assistantDiv.textContent = text + '...';
        }
      }
    }
  } catch (err) {
    console.error('SSE stream error:', err);
    assistantDiv.textContent = '请求失败: ' + err.message;
    llmStatusEl.textContent = '错误';
  }
  
  askBtn.disabled = false;
  askBtn.textContent = originalBtnText;
}

askBtn.addEventListener('click', sendPrompt);
promptEl.addEventListener('keydown', (ev) => {
  if (ev.key === 'Enter' && (ev.metaKey || ev.ctrlKey)) {
    sendPrompt();
  }
});

// 摄像头视频流（自动连接）
function connectCamera() {
  const ws = new WebSocket(`ws://${location.host}/ws/video`);
  ws.binaryType = 'arraybuffer';

  ws.onmessage = (ev) => {
    if (typeof ev.data === 'string') {
      try {
        const j = JSON.parse(ev.data);
        
        if (j.type === 'frame_meta') {
          // 不在视频流上方显示 FPS/Frame（改由右侧 Metrics 面板显示）
          if (hudA) hudA.textContent = '';
          
          renderMetrics({
            fps: j.fps,
            latency_ms: j.inference_time_ms,
            frame_id: j.frame_id,
            over_limit: false
          });

          // Timeline 更新逻辑：智能防抖 + 多仪表支持
          if (j.num_detections > 0 && j.gauge_angles && j.gauge_angles.length > 0) {
            const currentTime = Date.now();
            
            // 收集所有有效读数
            const validReadings = [];
            for (let i = 0; i < j.gauge_angles.length; i++) {
              const reading = j.gauge_angles[i];
              if (reading !== null && reading !== undefined && !isNaN(reading)) {
                validReadings.push({index: i + 1, value: reading});
              }
            }
            
            if (validReadings.length > 0) {
              let shouldUpdate = false;
              
              // 使用第一个仪表的读数作为防抖基准
              const firstReading = validReadings[0].value;
              
              if (lastGaugeReading === null) {
                // 首次检测
                shouldUpdate = true;
              } else {
                const changeDiff = Math.abs(firstReading - lastGaugeReading);
                const timeDiff = currentTime - lastTimelineUpdate;
                
                if (changeDiff >= 5.0) {
                  shouldUpdate = true;
                } else if (timeDiff >= TIMELINE_UPDATE_INTERVAL) {
                  shouldUpdate = true;
                }
              }
              
              if (shouldUpdate) {
                // 构建显示文本
                let displayText;
                if (validReadings.length === 1) {
                  displayText = `${validReadings[0].value.toFixed(1)}%`;
                } else {
                  // 多个仪表：显示所有读数
                  const readingTexts = validReadings.map(r => 
                    `#${r.index}: ${r.value.toFixed(1)}%`
                  ).join(' | ');
                  displayText = readingTexts;
                }
                
                pushTimeline({
                  ts: j.timestamp,
                  event: validReadings.length > 1 ? `${validReadings.length} 个仪表` : '仪表读数',
                  value: displayText
                });
                
                lastGaugeReading = firstReading;
                lastTimelineUpdate = currentTime;
              }
            }
          }
        } else if (j.type === 'error') {
          console.error('摄像头错误:', j.message);
        }
      } catch (err) {
        console.error('WebSocket 消息解析失败:', err);
      }
      return;
    }

    const blob = new Blob([ev.data], { type: 'image/jpeg' });
    const url = URL.createObjectURL(blob);
    imgA.src = url;
  };

  ws.onclose = () => {
    console.log('摄像头断开，2秒后重连...');
    setTimeout(connectCamera, 2000);
  };

  ws.onerror = (err) => {
    console.error('WebSocket 错误:', err);
  };
}

// 页面加载时自动连接
connectCamera();

// ==================== 数据统计与分析功能 ====================

// 刷新统计数据（动态卡片版本）
async function refreshStats() {
  try {
    const hours = document.getElementById('time-range').value;
    const response = await fetch(`/api/data/stats?hours=${hours}`);
    const data = await response.json();
    
    if (data.success) {
      const stats = data.statistics;
      const statsContent = document.getElementById('stats-content');
      
      // 清空现有卡片（保留总读数）
      statsContent.innerHTML = `
        <div class="stat-card">
          <h3>总读数</h3>
          <p class="stat-value">${stats.total_readings || 0}</p>
          <p class="stat-label">数据点</p>
        </div>
      `;
      
      // 动态添加每个仪表类型的卡片
      for (const [type, typeStats] of Object.entries(stats.by_type || {})) {
        const displayName = typeStats.display_name || type;
        const unit = typeStats.unit || '%';
        const isConfigured = unit !== '%';
        
        // 使用实际值或百分比
        const avgValue = isConfigured ? typeStats.avg_actual : typeStats.avg;
        const minValue = isConfigured ? typeStats.min_actual : typeStats.min;
        const maxValue = isConfigured ? typeStats.max_actual : typeStats.max;
        
        const hintHtml = !isConfigured ? '<p class="stat-hint">点击 ⚙️ 配置单位和量程</p>' : '';
        
        const cardHtml = `
          <div class="stat-card" data-gauge-type="${type}">
            <div class="stat-card-header">
              <h3>${displayName}</h3>
              <button class="config-btn" onclick="showGaugeConfig('${type}')" title="配置">⚙️</button>
            </div>
            <p class="stat-value">${avgValue.toFixed(2)} ${unit}</p>
            <p class="stat-trend">
              趋势: ${typeStats.trend} | 
              范围: ${minValue.toFixed(1)}-${maxValue.toFixed(1)} ${unit}
            </p>
            ${hintHtml}
          </div>
        `;
        
        statsContent.innerHTML += cardHtml;
      }
      
      console.log('[Stats] 统计数据已更新（动态卡片）');
    } else {
      console.error('[Stats] 获取统计数据失败:', data.error);
    }
  } catch (error) {
    console.error('[Stats] 获取统计数据失败:', error);
  }
}

// 生成分析报告
async function generateReport() {
  const btn = document.getElementById('generate-report-btn');
  const overlay = document.getElementById('loading-overlay');
  
  try {
    btn.disabled = true;
    overlay.style.display = 'flex';
    
    const hours = document.getElementById('time-range').value;
    const response = await fetch(`/api/data/report?hours=${hours}`, {
      method: 'POST'
    });
    
    const data = await response.json();
    
    if (data.success) {
      const reportId = data.report.report_id;
      overlay.style.display = 'none';
      
      // 显示成功提示
      showToast('报告生成成功！', 'success');
      
      // 立即显示报告详情
      setTimeout(() => showReportDetail(reportId), 500);
    } else {
      overlay.style.display = 'none';
      showToast('报告生成失败: ' + data.error, 'error');
    }
  } catch (error) {
    console.error('[Report] 生成报告失败:', error);
    overlay.style.display = 'none';
    showToast('生成报告失败: ' + error.message, 'error');
  } finally {
    btn.disabled = false;
  }
}

// 显示报告列表
async function showReportList() {
  try {
    const response = await fetch('/api/data/reports?limit=20');
    const data = await response.json();
    
    if (data.success) {
      const reports = data.reports;
      
      if (reports.length === 0) {
        showModal('历史报告', '<p class="empty-state">暂无历史报告，点击"生成分析报告"创建第一份报告。</p>');
        return;
      }
      
      // 创建报告列表 HTML
      let html = '<div class="report-list">';
      reports.forEach(report => {
        const createdAt = new Date(report.created_at).toLocaleString('zh-CN');
        const periodStart = new Date(report.period_start).toLocaleDateString('zh-CN');
        const periodEnd = new Date(report.period_end).toLocaleDateString('zh-CN');
        
        html += `
          <div class="report-item" onclick="showReportDetail(${report.id})">
            <div class="report-header">
              <h3>报告 #${report.id}</h3>
              <span class="report-date">${createdAt}</span>
            </div>
            <p class="report-summary">${report.summary || '暂无摘要'}</p>
            <div class="report-meta">
              <span>数据周期: ${periodStart} - ${periodEnd}</span>
              <span>${report.data_points} 条数据</span>
            </div>
          </div>
        `;
      });
      html += '</div>';
      
      showModal('历史报告', html);
    } else {
      showToast('获取报告列表失败: ' + data.error, 'error');
    }
  } catch (error) {
    console.error('[Report] 获取报告列表失败:', error);
    showToast('获取报告列表失败: ' + error.message, 'error');
  }
}

// 显示报告详情
async function showReportDetail(reportId) {
  try {
    const response = await fetch(`/api/data/reports/${reportId}`);
    const data = await response.json();
    
    if (data.success) {
      const report = data.report;
      
      const createdAt = new Date(report.created_at).toLocaleString('zh-CN');
      const periodStart = new Date(report.period_start).toLocaleString('zh-CN');
      const periodEnd = new Date(report.period_end).toLocaleString('zh-CN');
      
      // 使用 marked.js 渲染 Markdown
      const reportHtml = marked.parse(report.report_text || '暂无报告内容');
      
      const html = `
        <div class="report-detail">
          <div class="report-detail-header">
            <div class="report-detail-info">
              <p><strong>报告编号:</strong> #${report.id}</p>
              <p><strong>生成时间:</strong> ${createdAt}</p>
              <p><strong>数据周期:</strong> ${periodStart} 至 ${periodEnd}</p>
              <p><strong>数据点数:</strong> ${report.data_points}</p>
            </div>
            <div class="report-detail-actions">
              <button onclick="exportReport(${report.id})" class="btn-secondary">导出报告</button>
              <button onclick="deleteReport(${report.id})" class="btn-danger">删除报告</button>
            </div>
          </div>
          <hr>
          <div class="report-content markdown-body">
            ${reportHtml}
          </div>
        </div>
      `;
      
      showModal(`报告详情 - #${report.id}`, html);
    } else {
      showToast('获取报告详情失败: ' + data.error, 'error');
    }
  } catch (error) {
    console.error('[Report] 获取报告详情失败:', error);
    showToast('获取报告详情失败: ' + error.message, 'error');
  }
}

// 删除报告
async function deleteReport(reportId) {
  if (!confirm('确定要删除这份报告吗？此操作不可恢复。')) {
    return;
  }
  
  try {
    const response = await fetch(`/api/data/reports/${reportId}`, {
      method: 'DELETE'
    });
    
    const data = await response.json();
    
    if (data.success) {
      showToast('报告已删除', 'success');
      closeModal();
      // 刷新报告列表
      setTimeout(() => showReportList(), 500);
    } else {
      showToast('删除报告失败: ' + data.error, 'error');
    }
  } catch (error) {
    console.error('[Report] 删除报告失败:', error);
    showToast('删除报告失败: ' + error.message, 'error');
  }
}

// 导出报告（下载为 Markdown 文件）
function exportReport(reportId) {
  // 触发下载
  window.open(`/api/data/reports/${reportId}/export?format=md`, '_blank');
  showToast('正在下载报告...', 'info');
}

// Modal 对话框控制
function showModal(title, content) {
  document.getElementById('modal-title').textContent = title;
  document.getElementById('modal-body').innerHTML = content;
  document.getElementById('modal-overlay').style.display = 'flex';
}

function closeModal() {
  document.getElementById('modal-overlay').style.display = 'none';
}

// 点击遮罩层关闭 Modal
document.getElementById('modal-overlay').addEventListener('click', (e) => {
  if (e.target.id === 'modal-overlay') {
    closeModal();
  }
});

// Toast 提示
function showToast(message, type = 'info') {
  // 创建 toast 元素
  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;
  toast.textContent = message;
  
  document.body.appendChild(toast);
  
  // 触发显示动画
  setTimeout(() => toast.classList.add('show'), 10);
  
  // 3秒后移除
  setTimeout(() => {
    toast.classList.remove('show');
    setTimeout(() => toast.remove(), 300);
  }, 3000);
}

// 页面加载时刷新统计数据
document.addEventListener('DOMContentLoaded', () => {
  refreshStats();
  // 每30秒自动刷新统计
  setInterval(refreshStats, 30000);
});

// ESC 键关闭 Modal
document.addEventListener('keydown', (e) => {
  if (e.key === 'Escape') {
    closeModal();
    closeGaugeConfig();
  }
});

// ==================== 仪表配置功能 ====================

// 当前配置的仪表类型
let currentConfigGaugeType = null;

// 显示配置对话框
async function showGaugeConfig(gaugeType) {
  currentConfigGaugeType = gaugeType;
  
  try {
    // 获取当前配置
    const response = await fetch(`/api/gauge/configs/${encodeURIComponent(gaugeType)}`);
    const data = await response.json();
    
    if (data.success) {
      const config = data.config;
      
      // 填充表单
      document.getElementById('gauge-config-title').textContent = `配置 - ${gaugeType}`;
      document.getElementById('config-display-name').value = config.display_name || gaugeType;
      document.getElementById('config-min-range').value = config.min_range || 0;
      document.getElementById('config-max-range').value = config.max_range || 100;
      document.getElementById('config-unit').value = config.unit || '%';
      document.getElementById('config-low-threshold').value = config.low_threshold || 0;
      document.getElementById('config-high-threshold').value = config.high_threshold || 100;
      
      // 显示对话框
      document.getElementById('gauge-config-overlay').style.display = 'flex';
    }
  } catch (error) {
    console.error('[Config] 获取配置失败:', error);
    showToast('获取配置失败', 'error');
  }
}

// 保存配置
async function saveGaugeConfig() {
  if (!currentConfigGaugeType) return;
  
  const config = {
    display_name: document.getElementById('config-display-name').value,
    min_range: parseFloat(document.getElementById('config-min-range').value),
    max_range: parseFloat(document.getElementById('config-max-range').value),
    unit: document.getElementById('config-unit').value,
    low_threshold: parseFloat(document.getElementById('config-low-threshold').value),
    high_threshold: parseFloat(document.getElementById('config-high-threshold').value)
  };
  
  try {
    const response = await fetch(`/api/gauge/configs/${encodeURIComponent(currentConfigGaugeType)}`, {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(config)
    });
    
    const data = await response.json();
    
    if (data.success) {
      showToast('配置已保存！', 'success');
      closeGaugeConfig();
      refreshStats();  // 刷新统计显示
    } else {
      showToast('保存失败: ' + data.error, 'error');
    }
  } catch (error) {
    console.error('[Config] 保存配置失败:', error);
    showToast('保存失败: ' + error.message, 'error');
  }
}

// 重置为默认
async function resetGaugeConfig() {
  if (!currentConfigGaugeType) return;
  
  if (!confirm('确定要重置为默认配置吗？')) return;
  
  try {
    const response = await fetch(`/api/gauge/configs/${encodeURIComponent(currentConfigGaugeType)}`, {
      method: 'DELETE'
    });
    
    const data = await response.json();
    
    if (data.success) {
      showToast('已重置为默认', 'success');
      closeGaugeConfig();
      refreshStats();
    }
  } catch (error) {
    console.error('[Config] 重置配置失败:', error);
    showToast('重置失败: ' + error.message, 'error');
  }
}

// 关闭配置对话框
function closeGaugeConfig() {
  document.getElementById('gauge-config-overlay').style.display = 'none';
  currentConfigGaugeType = null;
}

// ==================== 数据删除功能 ====================

// 删除指定仪表的所有数据
async function deleteGaugeData() {
  if (!currentConfigGaugeType) return;
  
  const confirmation = prompt(
    `⚠️ 危险操作！\n\n` +
    `这将删除"${currentConfigGaugeType}"的所有历史数据，此操作不可恢复。\n\n` +
    `如果确定要删除，请输入仪表名称确认：`,
    ''
  );
  
  if (confirmation !== currentConfigGaugeType) {
    if (confirmation !== null) {
      showToast('输入不匹配，已取消删除', 'info');
    }
    return;
  }
  
  try {
    const response = await fetch(`/api/data/readings/${encodeURIComponent(currentConfigGaugeType)}`, {
      method: 'DELETE'
    });
    
    const data = await response.json();
    
    if (data.success) {
      showToast(`已删除 ${data.deleted_count} 条数据`, 'success');
      closeGaugeConfig();
      refreshStats();  // 刷新统计（卡片会消失）
    } else {
      showToast('删除失败: ' + data.error, 'error');
    }
  } catch (error) {
    console.error('[Data] 删除数据失败:', error);
    showToast('删除失败: ' + error.message, 'error');
  }
}

// 清空所有数据（确认对话框）
function confirmClearAllData() {
  const confirmation = prompt(
    `⚠️⚠️⚠️ 危险操作！⚠️⚠️⚠️\n\n` +
    `这将删除所有仪表的所有历史数据和报告，此操作不可恢复！\n\n` +
    `如果确定要清空，请输入"清空所有数据"确认：`,
    ''
  );
  
  if (confirmation === '清空所有数据') {
    clearAllData();
  } else if (confirmation !== null) {
    showToast('输入不匹配，已取消清空', 'info');
  }
}

// 清空所有数据（实际执行）
async function clearAllData() {
  try {
    const response = await fetch('/api/data/clear-all', {
      method: 'POST'
    });
    
    const data = await response.json();
    
    if (data.success) {
      const deleted = data.deleted;
      showToast(
        `已清空 ${deleted.readings} 条读数和 ${deleted.reports} 份报告`, 
        'success'
      );
      refreshStats();  // 刷新统计（所有卡片消失）
    } else {
      showToast('清空失败: ' + data.error, 'error');
    }
  } catch (error) {
    console.error('[Data] 清空数据失败:', error);
    showToast('清空失败: ' + error.message, 'error');
  }
}





