<script setup lang="ts">
import { RouterView } from 'vue-router'
</script>

<template>
  <RouterView />
</template>

<style>
* {
  box-sizing: border-box;
}

body {
  margin: 0;
  font-family: 'Microsoft YaHei', 'PingFang SC', 'Segoe UI', sans-serif;
  color: #e4f1ff;
  background:
    radial-gradient(circle at top left, rgba(24, 189, 255, 0.22), transparent 26%),
    radial-gradient(circle at top right, rgba(49, 255, 169, 0.12), transparent 20%),
    linear-gradient(180deg, #050b14 0%, #07111d 38%, #091724 100%);
}

body::before {
  content: '';
  position: fixed;
  inset: 0;
  pointer-events: none;
  background-image:
    linear-gradient(rgba(69, 105, 136, 0.08) 1px, transparent 1px),
    linear-gradient(90deg, rgba(69, 105, 136, 0.08) 1px, transparent 1px);
  background-size: 48px 48px;
  mask-image: linear-gradient(180deg, rgba(0, 0, 0, 0.58), transparent);
}

#app {
  min-height: 100vh;
}
</style>

<style>

.page-shell {
  width: min(1440px, calc(100% - 32px));
  margin: 0 auto;
  padding: 24px 0 40px;
  display: grid;
  gap: 18px;
}

.hero-panel,
.panel,
.banner,
.dialog-card {
  border: 1px solid rgba(102, 154, 198, 0.16);
  background: linear-gradient(180deg, rgba(9, 23, 40, 0.92), rgba(7, 16, 31, 0.92));
  box-shadow:
    0 0 0 1px rgba(42, 95, 142, 0.08) inset,
    0 20px 50px rgba(0, 0, 0, 0.34);
  backdrop-filter: blur(14px);
}

.hero-panel {
  border-radius: 32px;
  padding: 30px;
  display: grid;
  grid-template-columns: 1.15fr 0.85fr;
  gap: 24px;
  align-items: start;
}

.hero-copy,
.hero-aside,
.mqtt-panel,
.time-card,
.status-card,
.metric-card,
.stats-card,
.log-card {
  position: relative;
  overflow: hidden;
}

.hero-copy::after,
.panel::before,
.status-card::before,
.metric-card::before,
.log-card::before {
  content: '';
  position: absolute;
  inset: 0;
  pointer-events: none;
  background: linear-gradient(
    135deg,
    rgba(119, 214, 255, 0.08),
    transparent 34%,
    transparent 66%,
    rgba(120, 255, 218, 0.05)
  );
}

.eyebrow,
.section-tag {
  margin: 0 0 10px;
  letter-spacing: 0.24em;
  font-size: 12px;
  font-weight: 700;
  color: #78ceff;
}

h1,
h2 {
  margin: 0;
  font-weight: 700;
}

h1 {
  font-size: clamp(40px, 5vw, 76px);
  line-height: 0.98;
  text-shadow: 0 0 24px rgba(82, 193, 255, 0.18);
}

h2 {
  font-size: clamp(24px, 2vw, 34px);
}

.hero-text {
  max-width: 720px;
  margin: 16px 0 26px;
  color: #90aeca;
  line-height: 1.8;
  font-size: 15px;
}

.hero-actions,
.controls,
.mqtt-actions,
.dialog-footer,
.inline-grid {
  display: flex;
  gap: 12px;
  align-items: center;
}

button,
select,
input {
  border-radius: 16px;
  border: 1px solid rgba(102, 154, 198, 0.18);
  padding: 12px 16px;
  font: inherit;
}

button {
  cursor: pointer;
}

button:focus-visible,
select:focus-visible,
input:focus-visible {
  outline: 2px solid rgba(82, 216, 255, 0.55);
  outline-offset: 2px;
}

select,
input {
  color: #dbeeff;
  background: rgba(8, 17, 31, 0.88);
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.68;
}

.primary-btn,
.secondary-btn,
.ghost-btn,
.icon-btn {
  transition:
    transform 0.2s ease,
    box-shadow 0.2s ease,
    border-color 0.2s ease,
    background 0.2s ease;
}

.primary-btn:hover,
.secondary-btn:hover,
.ghost-btn:hover,
.icon-btn:hover {
  transform: translateY(-1px);
}

.primary-btn {
  color: #051118;
  border: none;
  background: linear-gradient(135deg, #52d8ff, #73ffbf);
  box-shadow: 0 10px 30px rgba(83, 216, 255, 0.28);
}

.secondary-btn {
  color: #d4efff;
  background: rgba(18, 35, 57, 0.94);
}

.ghost-btn,
.icon-btn {
  color: #9ecdf6;
  background: transparent;
}

.small-btn {
  padding: 10px 14px;
}

.hero-aside {
  display: grid;
  gap: 16px;
}

.time-card {
  padding: 20px 22px;
  border-radius: 24px;
  border: 1px solid rgba(97, 167, 214, 0.18);
  background: linear-gradient(180deg, rgba(8, 19, 35, 0.98), rgba(8, 19, 33, 0.9));
}

.time-label,
.sample-chip,
.info-label {
  display: block;
  color: #7da2c3;
  font-size: 13px;
  letter-spacing: 0.08em;
}

.time-card strong {
  display: block;
  margin-top: 10px;
  font-size: clamp(24px, 2.4vw, 38px);
  color: #ecf8ff;
}

.time-card small {
  display: block;
  margin-top: 10px;
  color: #72b8e4;
  overflow-wrap: anywhere;
}

.hero-status,
.content-grid,
.metric-grid,
.stats-grid {
  display: grid;
  gap: 16px;
}

.hero-status {
  grid-template-columns: repeat(2, minmax(0, 1fr));
}

.status-card,
.metric-card,
.stats-card,
.log-card {
  border-radius: 22px;
  border: 1px solid rgba(103, 152, 190, 0.14);
  background: rgba(8, 17, 31, 0.92);
}

.status-card {
  padding: 18px;
  min-height: 108px;
}

.status-card span,
.metric-label,
.stats-card span {
  display: block;
  color: #82a8c6;
}

.status-card strong {
  display: block;
  margin-top: 12px;
  font-size: 24px;
  line-height: 1.3;
}

.status-card[data-tone='ok'] strong {
  color: #83ffbf;
}

.status-card[data-tone='danger'] strong {
  color: #ff8f8f;
}

.banner {
  border-radius: 22px;
  padding: 16px 20px;
}

.error-banner {
  color: #ffb5b5;
  background: rgba(62, 16, 21, 0.88);
}

.info-banner {
  color: #a8c9e6;
}

.panel {
  border-radius: 28px;
  padding: 24px;
}

.panel-header,
.subpanel-header,
.dialog-header {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  align-items: end;
}

.panel-header {
  margin-bottom: 18px;
}

.metrics-panel {
  animation: rise 0.45s ease;
}

.metric-grid {
  grid-template-columns: repeat(6, minmax(0, 1fr));
}

.metric-card {
  padding: 18px;
  min-height: 176px;
}

.metric-card strong {
  display: block;
  margin-top: 18px;
  font-size: clamp(28px, 2vw, 42px);
  line-height: 1;
  color: #f4fbff;
}

.metric-card small {
  display: block;
  margin-top: 8px;
  color: #6d9fc5;
}

.metric-track {
  height: 8px;
  margin-top: 22px;
  border-radius: 999px;
  overflow: hidden;
  background: rgba(91, 122, 150, 0.18);
}

.metric-fill {
  height: 100%;
  border-radius: inherit;
  background: linear-gradient(90deg, rgba(82, 216, 255, 0.4), #52d8ff);
  box-shadow: 0 0 18px rgba(82, 216, 255, 0.22);
}

.metric-card[data-tone='warm'] strong,
.metric-card[data-tone='warm'] .metric-fill {
  color: #ffb27a;
  background-image: linear-gradient(90deg, rgba(255, 140, 92, 0.32), #ff8c5c);
}

.metric-card[data-tone='cool'] strong,
.metric-card[data-tone='cool'] .metric-fill {
  color: #6de6ff;
  background-image: linear-gradient(90deg, rgba(93, 226, 255, 0.3), #5de2ff);
}

.metric-card[data-tone='green'] strong,
.metric-card[data-tone='green'] .metric-fill {
  color: #78f7a8;
  background-image: linear-gradient(90deg, rgba(120, 247, 168, 0.3), #78f7a8);
}

.metric-card[data-tone='gold'] strong,
.metric-card[data-tone='gold'] .metric-fill {
  color: #f5de6f;
  background-image: linear-gradient(90deg, rgba(245, 222, 111, 0.3), #f5de6f);
}

.metric-card[data-tone='violet'] strong,
.metric-card[data-tone='violet'] .metric-fill {
  color: #c29bff;
  background-image: linear-gradient(90deg, rgba(184, 144, 255, 0.3), #b890ff);
}

.metric-card[data-tone='cyan'] strong,
.metric-card[data-tone='cyan'] .metric-fill {
  color: #7de8e6;
  background-image: linear-gradient(90deg, rgba(125, 232, 230, 0.3), #7de8e6);
}

.content-grid {
  grid-template-columns: 1fr 1fr;
  align-items: start;
}

.history-layout {
  grid-template-columns: 1.2fr 0.8fr;
}

.mqtt-layout {
  grid-template-columns: 1fr;
}

.decision-list,
.presence-list {
  margin: 0;
  padding: 0;
  list-style: none;
}

.decision-list div,
.presence-list li,
.stats-card dl div,
.mqtt-info-strip > div {
  display: flex;
  justify-content: space-between;
  gap: 12px;
  padding: 14px 0;
  border-bottom: 1px solid rgba(74, 112, 146, 0.14);
}

.decision-list div:last-child,
.presence-list li:last-child,
.stats-card dl div:last-child,
.mqtt-info-strip > div:last-child {
  border-bottom: none;
}

.decision-list dt,
.presence-list span,
.stats-card dt {
  color: #7da2c3;
}

.decision-list dd,
.presence-list strong,
.stats-card dd {
  margin: 0;
  color: #eff8ff;
  font-weight: 700;
  text-align: right;
}

.table-wrap {
  overflow-x: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
}

th,
td {
  padding: 12px 10px;
  border-bottom: 1px solid rgba(74, 112, 146, 0.14);
  text-align: left;
  white-space: nowrap;
}

th {
  color: #7da2c3;
  font-size: 13px;
}

.empty-state {
  padding: 34px 0 8px;
  text-align: center;
  color: #7d9dbb;
}

.stats-grid {
  grid-template-columns: 1fr;
}

.stats-card {
  padding: 18px;
}

.stats-card strong {
  display: block;
  margin: 10px 0 16px;
  font-size: 26px;
  color: #f3f8ff;
}

.stats-card dl {
  margin: 0;
}

.mqtt-panel {
  display: grid;
  gap: 18px;
}

.mqtt-badge {
  padding: 8px 14px;
  border-radius: 999px;
  font-size: 13px;
  font-weight: 700;
  letter-spacing: 0.08em;
}

.mqtt-badge[data-state='idle'],
.mqtt-badge[data-state='disconnected'] {
  color: #9cb4c9;
  background: rgba(66, 86, 104, 0.34);
}

.mqtt-badge[data-state='connecting'] {
  color: #ffd57f;
  background: rgba(142, 110, 32, 0.3);
}

.mqtt-badge[data-state='connected'] {
  color: #87ffc2;
  background: rgba(21, 108, 74, 0.36);
  box-shadow: 0 0 18px rgba(91, 255, 179, 0.18);
}

.mqtt-badge[data-state='error'] {
  color: #ff9d9d;
  background: rgba(115, 31, 45, 0.36);
}

.mqtt-info-strip {
  border-radius: 20px;
  padding: 4px 18px;
  background: rgba(6, 14, 27, 0.84);
  border: 1px solid rgba(103, 152, 190, 0.12);
}

.mqtt-info-strip strong {
  color: #e8f6ff;
}

.mqtt-log-layout {
  display: grid;
  grid-template-columns: 1fr;
  gap: 18px;
}

.log-card {
  padding: 18px;
  background:
    linear-gradient(180deg, rgba(8, 19, 35, 0.98), rgba(7, 17, 31, 0.94)),
    rgba(8, 19, 35, 0.92);
}

.subpanel-header {
  margin-bottom: 14px;
}

.subpanel-header span {
  font-weight: 700;
  color: #d9f0ff;
}

.subpanel-header small {
  color: #81a4c4;
}

.log-list {
  height: 280px;
  overflow: auto;
  display: grid;
  gap: 10px;
}

.log-item {
  padding: 12px 14px;
  border-radius: 16px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(10, 20, 34, 0.94);
}

.log-item[data-level='success'] {
  border-color: rgba(120, 247, 168, 0.18);
  background: rgba(10, 34, 28, 0.8);
}

.log-item[data-level='warning'] {
  border-color: rgba(245, 222, 111, 0.18);
  background: rgba(36, 30, 10, 0.78);
}

.log-item[data-level='error'] {
  border-color: rgba(255, 122, 122, 0.18);
  background: rgba(38, 14, 18, 0.8);
}

.log-item span {
  display: block;
  margin-bottom: 6px;
  color: #80a3c0;
  font-size: 13px;
}

.log-item p {
  margin: 0;
  line-height: 1.5;
  color: #eaf7ff;
  overflow-wrap: anywhere;
}

.message-preview {
  margin: 0 0 10px;
  min-height: 160px;
  max-height: 220px;
  overflow: auto;
  padding: 16px 18px;
  border-radius: 18px;
  background: linear-gradient(180deg, rgba(3, 9, 18, 0.98), rgba(6, 15, 26, 0.98));
  color: #dff7df;
  font-family: Consolas, 'SFMono-Regular', monospace;
  white-space: pre-wrap;
  word-break: break-word;
  border: 1px solid rgba(90, 145, 182, 0.14);
}

.message-time {
  margin: 0;
  color: #7fb8d6;
  font-size: 13px;
}

.dialog-backdrop {
  position: fixed;
  inset: 0;
  z-index: 20;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(1, 6, 12, 0.62);
}

.dialog-card {
  width: min(760px, 100%);
  border-radius: 28px;
  padding: 26px;
}

.dialog-header {
  margin-bottom: 20px;
}

.dialog-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.field {
  display: grid;
  gap: 8px;
}

.field span {
  color: #8eb3d1;
  font-size: 14px;
  font-weight: 600;
}

.span-2 {
  grid-column: 1 / -1;
}

.protocol-grid select {
  width: 140px;
}

.protocol-grid input,
.client-grid input {
  flex: 1;
}

.switch-field {
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-radius: 18px;
  padding: 16px 18px;
  border: 1px solid rgba(102, 154, 198, 0.18);
  background: rgba(8, 17, 31, 0.88);
}

.switch-field input {
  width: 20px;
  height: 20px;
  padding: 0;
}

.dialog-note {
  margin-top: 16px;
  color: #86abc9;
  line-height: 1.7;
}

.dialog-footer {
  justify-content: flex-end;
  margin-top: 20px;
}

.icon-btn {
  width: 42px;
  height: 42px;
  padding: 0;
  border-radius: 50%;
  font-size: 28px;
  line-height: 1;
}

.charts-wrap {
  display: grid;
  gap: 18px;
  margin-top: 20px;
}

.chart-block {
  height: 260px;
  padding: 16px;
  border-radius: 22px;
  border: 1px solid rgba(103, 152, 190, 0.12);
  background: rgba(6, 14, 27, 0.82);
}

.chart-title {
  margin: 0 0 8px;
  color: #80a8c8;
  font-size: 13px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

@keyframes rise {
  from {
    opacity: 0;
    transform: translateY(10px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 1200px) {
  .metric-grid {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }

  .history-layout {
    grid-template-columns: 1fr;
  }
}

@media (max-width: 960px) {
  .hero-panel,
  .content-grid,
  .dialog-grid {
    grid-template-columns: 1fr;
  }

  .hero-status {
    grid-template-columns: 1fr;
  }

  .metric-grid {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }

  .page-shell {
    width: min(1440px, calc(100% - 20px));
  }

  .hero-panel,
  .panel,
  .dialog-card {
    padding: 20px;
  }

  .hero-actions,
  .mqtt-actions,
  .dialog-footer,
  .controls {
    flex-wrap: wrap;
  }
}

@media (max-width: 640px) {
  .metric-grid {
    grid-template-columns: 1fr;
  }

  h1 {
    font-size: 34px;
  }
}
</style>
