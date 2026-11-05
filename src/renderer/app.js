// APPLY TRACK - Glassmorphic UI & Standard SQL Client

(function () {
  'use strict';

  // State
  let applications = [];
  let storageInfo = null;
  let currentView = 'dashboard';
  let searchQuery = '';
  let statusFilter = 'ALL';
  let workModeFilter = 'ALL';
  let sortOption = 'date-newest';
  let currentTheme = localStorage.getItem('applytrack_theme') || 'dark';

  const STAGES = [
    { id: 'Wishlist', label: 'Wishlist', color: '#94a3b8', pillClass: 'glass-pill' },
    { id: 'Applied', label: 'Applied', color: '#2dd4bf', pillClass: 'pill-teal' },
    { id: 'Screening', label: 'Screening / HR', color: '#f59e0b', pillClass: 'pill-orange' },
    { id: 'Technical Interview', label: 'Tech Interview', color: '#a855f7', pillClass: 'pill-purple' },
    { id: 'Final Round', label: 'Final Round', color: '#6366f1', pillClass: 'pill-purple' },
    { id: 'Offer', label: 'Offer Received', color: '#10b981', pillClass: 'pill-emerald' },
    { id: 'Rejected', label: 'Rejected', color: '#f43f5e', pillClass: 'pill-rose' }
  ];

  // Pure C++ Winsock REST API Client
  const API = {
    async getApplications() {
      try {
        const res = await fetch('/api/applications');
        return await res.json();
      } catch (e) {
        console.error('Fetch applications error:', e);
        return [];
      }
    },
    async saveApplication(appData) {
      const res = await fetch('/api/applications', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(appData)
      });
      return await res.json();
    },
    async updateStage(id, stage) {
      const res = await fetch('/api/stage', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id, stage })
      });
      return await res.json();
    },
    async deleteApplication(id) {
      const res = await fetch('/api/delete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ id })
      });
      return await res.json();
    },
    async getStorageInfo() {
      try {
        const res = await fetch('/api/storage');
        return await res.json();
      } catch (e) {
        return { storageDir: '', dbPath: '', dbSize: 0 };
      }
    },
    async openStorageFolder() {
      await fetch('/api/storage/open', { method: 'POST' });
    },
    async clearData() {
      await fetch('/api/clear', { method: 'POST' });
    }
  };

  function escapeHtml(str) {
    if (!str) return '';
    return String(str)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;');
  }

  function formatBytes(bytes) {
    if (!bytes || bytes === 0) return '0 KB';
    return (bytes / 1024).toFixed(1) + ' KB';
  }

  function formatSalary(min, max, currency = '$', period = 'Yearly') {
    const minN = Number(min || 0);
    const maxN = Number(max || 0);
    if (!minN && !maxN) return '';
    const p = period === 'Yearly' ? '/yr' : (period === 'Monthly' ? '/mo' : '/hr');
    if (minN && maxN) return `${currency}${minN.toLocaleString()} - ${currency}${maxN.toLocaleString()}${p}`;
    if (minN) return `${currency}${minN.toLocaleString()}+${p}`;
    return `Up to ${currency}${maxN.toLocaleString()}${p}`;
  }

  // Toast Notifications
  function showToast(msg, type = 'success') {
    const shelf = document.getElementById('toast-shelf');
    const toast = document.createElement('div');
    toast.className = 'glass-toast';
    toast.innerHTML = `
      <span style="color: ${type === 'success' ? '#2dd4bf' : '#f43f5e'};">
        ${type === 'success' ? Icons.check : Icons.close}
      </span>
      <span>${escapeHtml(msg)}</span>
    `;
    shelf.appendChild(toast);
    setTimeout(() => {
      toast.style.opacity = '0';
      toast.style.transform = 'translateY(10px)';
      toast.style.transition = 'all 0.2s';
      setTimeout(() => toast.remove(), 200);
    }, 3000);
  }

  // Initialize Icons
  function initIcons() {
    const map = {
      'icon-nav-dash': Icons.dashboard,
      'icon-nav-board': Icons.kanban,
      'icon-nav-list': Icons.list,
      'icon-nav-analytics': Icons.analytics,
      'icon-nav-storage': Icons.storage,
      'icon-storage-disk': Icons.hardDrive,
      'icon-quick-folder': Icons.folder,
      'icon-search-bar': Icons.search,
      'icon-plus-btn': Icons.plus,
      'icon-download-csv': Icons.download,
      'icon-folder-storage-btn': Icons.folder
    };
    for (const [id, svg] of Object.entries(map)) {
      const el = document.getElementById(id);
      if (el) el.innerHTML = svg;
    }
  }

  // Theme Management (Light and Dark Mode)
  function initTheme() {
    applyTheme(currentTheme, false);

    const themeBtn = document.getElementById('theme-toggle-btn');
    themeBtn.addEventListener('click', () => {
      const nextTheme = currentTheme === 'dark' ? 'light' : 'dark';
      applyTheme(nextTheme, true);
    });
  }

  function applyTheme(theme, showNotice = false) {
    currentTheme = theme;
    localStorage.setItem('applytrack_theme', theme);
    document.documentElement.setAttribute('data-theme', theme);

    const iconEl = document.getElementById('theme-toggle-icon');
    const labelEl = document.getElementById('theme-toggle-label');

    if (theme === 'light') {
      iconEl.textContent = '🌙';
      labelEl.textContent = 'Light';
      if (showNotice) showToast('Switched to Frosted Light Mode');
    } else {
      iconEl.textContent = '☀️';
      labelEl.textContent = 'Dark';
      if (showNotice) showToast('Switched to Glassmorphic Dark Mode');
    }
  }

  // Navigation
  function setupNavigation() {
    document.querySelectorAll('.glass-nav-item').forEach(item => {
      item.addEventListener('click', () => {
        const view = item.getAttribute('data-view');
        switchView(view);
      });
    });

    document.getElementById('btn-quick-explorer').addEventListener('click', () => API.openStorageFolder());
    document.getElementById('btn-open-explorer-storage').addEventListener('click', () => API.openStorageFolder());

    const searchInput = document.getElementById('global-search-input');
    searchInput.addEventListener('input', (e) => {
      searchQuery = e.target.value.toLowerCase().trim();
      renderCurrentView();
    });

    window.addEventListener('keydown', (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'f') {
        e.preventDefault();
        searchInput.focus();
        searchInput.select();
      }
    });
  }

  function switchView(viewName) {
    currentView = viewName;
    document.querySelectorAll('.glass-nav-item').forEach(el => {
      el.classList.toggle('active', el.getAttribute('data-view') === viewName);
    });
    document.querySelectorAll('.glass-view-body').forEach(el => {
      el.classList.toggle('active', el.id === `view-${viewName}`);
    });

    const headings = {
      dashboard: 'Overview Dashboard',
      board: 'Kanban Application Pipeline',
      list: 'All Job Applications',
      analytics: 'Salary & Conversion Insights',
      storage: 'Windows Internal SQLite Storage'
    };
    document.getElementById('page-headline').textContent = headings[viewName] || 'Overview';
    renderCurrentView();
  }

  function renderCurrentView() {
    updateCounts();
    if (currentView === 'dashboard') renderDashboard();
    else if (currentView === 'board') renderBoard();
    else if (currentView === 'list') renderList();
    else if (currentView === 'analytics') renderAnalytics();
    else if (currentView === 'storage') renderStorage();
  }

  function updateCounts() {
    document.getElementById('badge-board-count').textContent = applications.length;
    document.getElementById('badge-list-count').textContent = applications.length;
  }

  // --- EMPTY STATE GENERATOR (No Fake Data) ---
  function getEmptyStateHtml(subMessage = 'Start building your pipeline by adding your first real job application.') {
    return `
      <div class="glass-empty-state">
        <div class="empty-state-icon-wrap">
          <img src="app_icon.png" style="width: 44px; height: 44px; border-radius: 10px;" alt="Empty Icon">
        </div>
        <h2 class="empty-state-title">No Applications Tracked Yet</h2>
        <p class="empty-state-desc">
          ${escapeHtml(subMessage)} Every application is saved locally in your internal Windows SQLite database with zero cloud tracking.
        </p>
        <button class="glass-btn glass-btn-primary" onclick="window.openNewModal()">
          ${Icons.plus} Add First Application
        </button>
      </div>
    `;
  }

  // --- VIEW 1: DASHBOARD ---
  function renderDashboard() {
    const container = document.getElementById('dashboard-content-area');
    if (applications.length === 0) {
      container.innerHTML = getEmptyStateHtml();
      return;
    }

    const total = applications.length;
    const active = applications.filter(a => ['Applied', 'Screening', 'Technical Interview', 'Final Round'].includes(a.status)).length;
    const interviews = applications.filter(a => (a.status || '').includes('Interview') || a.next_interview).length;
    const offers = applications.filter(a => a.status === 'Offer').length;
    const rate = total > 0 ? Math.round((interviews / total) * 100) : 0;

    container.innerHTML = `
      <div class="glass-metrics-grid">
        <div class="glass-metric-card">
          <div class="metric-card-header">
            <span class="metric-card-label">Total Applications</span>
            <span style="color: var(--brand-teal);">${Icons.briefcase}</span>
          </div>
          <div class="metric-card-val">${total}</div>
          <span class="metric-card-sub">Recorded in SQLite</span>
        </div>

        <div class="glass-metric-card">
          <div class="metric-card-header">
            <span class="metric-card-label">In Active Pipeline</span>
            <span style="color: var(--brand-orange);">${Icons.kanban}</span>
          </div>
          <div class="metric-card-val">${active}</div>
          <span class="metric-card-sub">Applied through Final Round</span>
        </div>

        <div class="glass-metric-card">
          <div class="metric-card-header">
            <span class="metric-card-label">Interviews</span>
            <span style="color: #c084fc;">${Icons.calendar}</span>
          </div>
          <div class="metric-card-val">${interviews}</div>
          <span class="metric-card-sub">${rate}% interview conversion</span>
        </div>

        <div class="glass-metric-card">
          <div class="metric-card-header">
            <span class="metric-card-label">Offers Received</span>
            <span style="color: #10b981;">${Icons.check}</span>
          </div>
          <div class="metric-card-val" style="color: #10b981;">${offers}</div>
          <span class="metric-card-sub">Decision pending / accepted</span>
        </div>
      </div>

      <!-- Upcoming Events / Interviews -->
      <div class="glass-metric-card" style="margin-top: 10px;">
        <div class="metric-card-header">
          <span class="metric-card-label" style="font-size: 14px; font-weight: 700; color: var(--text-main);">
            ${Icons.clock} Upcoming Scheduled Interviews & Deadlines
          </span>
        </div>
        <div id="dashboard-interview-list" style="display: flex; flex-direction: column; gap: 10px; margin-top: 8px;">
          ${renderUpcomingInterviewItems()}
        </div>
      </div>
    `;
  }

  function renderUpcomingInterviewItems() {
    const upcoming = applications.filter(a => a.next_interview);
    if (upcoming.length === 0) {
      return `<div style="font-size: 13px; color: var(--text-muted); padding: 12px 0;">No upcoming interviews scheduled yet. Set a "Next Interview" date on any application to see it here.</div>`;
    }
    return upcoming.map(app => `
      <div style="background: var(--glass-bg-elevated); border-left: 3px solid var(--brand-teal); border-radius: var(--radius-sm); padding: 12px 14px; display: flex; justify-content: space-between; align-items: center;">
        <div>
          <div style="font-weight: 700; color: var(--text-main);">${escapeHtml(app.company)} - <span style="font-weight: 400; color: var(--text-dim);">${escapeHtml(app.role)}</span></div>
          <div style="font-size: 12px; color: var(--text-muted); margin-top: 2px;">
            ${Icons.clock} ${escapeHtml(app.next_interview)} • Stage: ${escapeHtml(app.status)}
          </div>
        </div>
        <button class="glass-btn glass-btn-secondary" style="font-size: 11px; padding: 4px 10px;" onclick="window.editApp('${app.id}')">View</button>
      </div>
    `).join('');
  }

  // --- VIEW 2: KANBAN BOARD ---
  function renderBoard() {
    const container = document.getElementById('kanban-columns-container');
    container.innerHTML = '';

    const filtered = filterApps(applications);

    STAGES.forEach(stage => {
      const colApps = filtered.filter(a => matchStage(a.status, stage.id));
      const col = document.createElement('div');
      col.className = 'glass-column';
      col.dataset.stageId = stage.id;

      col.innerHTML = `
        <div class="glass-column-header">
          <div class="column-title-pill">
            <span class="column-dot" style="color: ${stage.color}; background: ${stage.color};"></span>
            <span class="column-heading">${escapeHtml(stage.label)}</span>
            <span class="column-badge">${colApps.length}</span>
          </div>
          <button class="glass-mini-btn" title="Add to ${stage.label}" onclick="window.openNewModal('${stage.id}')">
            ${Icons.plus}
          </button>
        </div>
        <div class="glass-cards-dropzone" id="zone-${stage.id.replace(/\s+/g, '-')}"></div>
      `;

      const zone = col.querySelector('.glass-cards-dropzone');

      // Drag and drop listeners
      zone.addEventListener('dragover', (e) => {
        e.preventDefault();
        zone.classList.add('drag-active');
      });

      zone.addEventListener('dragleave', () => {
        zone.classList.remove('drag-active');
      });

      zone.addEventListener('drop', async (e) => {
        e.preventDefault();
        zone.classList.remove('drag-active');
        const id = e.dataTransfer.getData('text/plain');
        if (!id) return;

        const app = applications.find(a => a.id === id);
        if (app && !matchStage(app.status, stage.id)) {
          app.status = stage.id;
          await API.updateStage(id, stage.id);
          await refreshData();
          showToast(`Updated "${app.company}" to ${stage.label}`);
        }
      });

      if (colApps.length === 0) {
        zone.innerHTML = `
          <div style="flex: 1; display: flex; align-items: center; justify-content: center; font-size: 11.5px; color: var(--text-muted); opacity: 0.6; min-height: 80px;">
            Drop here
          </div>
        `;
      } else {
        colApps.forEach(app => {
          const card = createCard(app, stage);
          zone.appendChild(card);
        });
      }

      container.appendChild(col);
    });
  }

  function matchStage(appStatus, stageId) {
    if (!appStatus) return false;
    const a = appStatus.toLowerCase().replace(/[^a-z]/g, '');
    const b = stageId.toLowerCase().replace(/[^a-z]/g, '');
    return a === b || a.startsWith(b) || b.startsWith(a);
  }

  function createCard(app, stage) {
    const card = document.createElement('div');
    card.className = 'glass-app-card';
    card.draggable = true;
    card.dataset.id = app.id;

    card.addEventListener('dragstart', (e) => {
      card.classList.add('dragging');
      e.dataTransfer.setData('text/plain', app.id);
    });

    card.addEventListener('dragend', () => {
      card.classList.remove('dragging');
    });

    const salary = formatSalary(app.salary_min, app.salary_max, app.currency, app.salary_period);
    const tags = (app.tags || '').split(',').map(t => t.trim()).filter(t => t.length > 0);

    card.innerHTML = `
      <div class="app-card-title-row">
        <div>
          <div class="app-card-company">${escapeHtml(app.company)}</div>
          <div class="app-card-role">${escapeHtml(app.role)}</div>
        </div>
        <div class="card-foot-tools">
          ${app.job_url ? `<button class="glass-mini-btn" title="Open Link" onclick="window.openUrl('${escapeHtml(app.job_url)}')">${Icons.externalLink}</button>` : ''}
          <button class="glass-mini-btn" title="Edit" onclick="window.editApp('${app.id}')">${Icons.edit}</button>
          <button class="glass-mini-btn" title="Delete" onclick="window.deleteApp('${app.id}')">${Icons.trash}</button>
        </div>
      </div>

      ${salary ? `<div class="app-card-salary">${salary}</div>` : ''}

      <div class="app-card-tags">
        ${app.work_mode ? `<span class="glass-pill pill-teal">${escapeHtml(app.work_mode)}</span>` : ''}
        ${app.next_interview ? `<span class="glass-pill pill-purple">${Icons.clock} ${escapeHtml(app.next_interview)}</span>` : ''}
        ${tags.slice(0, 2).map(t => `<span class="glass-pill">${escapeHtml(t)}</span>`).join('')}
      </div>

      <div class="app-card-footer">
        <span class="card-foot-text">${app.date_applied ? 'Applied: ' + escapeHtml(app.date_applied) : 'Draft'}</span>
        ${app.location ? `<span class="card-foot-text" style="max-width: 120px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap;">${escapeHtml(app.location)}</span>` : ''}
      </div>
    `;

    return card;
  }

  // --- VIEW 3: APPLICATIONS LIST ---
  function renderList() {
    const tbody = document.getElementById('table-body-rows');
    let filtered = filterApps(applications);
    filtered = sortApps(filtered);

    if (filtered.length === 0) {
      tbody.innerHTML = `
        <tr>
          <td colspan="7" style="text-align: center; padding: 48px; color: var(--text-muted);">
            No matching job applications found. Click "+ New Application" to add one.
          </td>
        </tr>
      `;
      return;
    }

    tbody.innerHTML = filtered.map(app => {
      const stage = STAGES.find(s => matchStage(app.status, s.id)) || { label: app.status, color: '#94a3b8' };
      const salary = formatSalary(app.salary_min, app.salary_max, app.currency, app.salary_period) || '-';

      return `
        <tr>
          <td>
            <div style="font-weight: 700; color: var(--text-main);">${escapeHtml(app.company)}</div>
            <div style="font-size: 12px; color: var(--text-dim);">${escapeHtml(app.role)}</div>
          </td>
          <td>
            <span class="glass-pill" style="border-color: ${stage.color}44; color: ${stage.color}; background: ${stage.color}18;">
              <span style="width: 6px; height: 6px; border-radius: 50%; background: ${stage.color};"></span>
              ${escapeHtml(stage.label)}
            </span>
          </td>
          <td style="color: #10b981; font-weight: 600;">${salary}</td>
          <td><span class="glass-pill pill-teal">${escapeHtml(app.work_mode || 'Remote')}</span></td>
          <td style="font-size: 12px; color: var(--text-dim);">${escapeHtml(app.date_applied || '-')}</td>
          <td style="font-size: 12px; color: ${app.next_interview ? '#c084fc' : 'var(--text-muted)'};">
            ${app.next_interview ? `${Icons.clock} ${escapeHtml(app.next_interview)}` : '-'}
          </td>
          <td style="text-align: right;">
            <div style="display: flex; gap: 6px; justify-content: flex-end;">
              ${app.job_url ? `<button class="glass-mini-btn" title="Open Link" onclick="window.openUrl('${escapeHtml(app.job_url)}')">${Icons.externalLink}</button>` : ''}
              <button class="glass-mini-btn" title="Edit" onclick="window.editApp('${app.id}')">${Icons.edit}</button>
              <button class="glass-mini-btn" title="Delete" onclick="window.deleteApp('${app.id}')">${Icons.trash}</button>
            </div>
          </td>
        </tr>
      `;
    }).join('');
  }

  function filterApps(apps) {
    return apps.filter(a => {
      if (searchQuery) {
        const text = `${a.company} ${a.role} ${a.location} ${a.tags} ${a.notes}`.toLowerCase();
        if (!text.includes(searchQuery)) return false;
      }
      if (statusFilter !== 'ALL' && !matchStage(a.status, statusFilter)) return false;
      if (workModeFilter !== 'ALL' && (a.work_mode || '').toLowerCase() !== workModeFilter.toLowerCase()) return false;
      return true;
    });
  }

  function sortApps(apps) {
    const copy = [...apps];
    if (sortOption === 'date-newest') {
      return copy.sort((a, b) => (b.date_applied || b.created_at || '').localeCompare(a.date_applied || a.created_at || ''));
    } else if (sortOption === 'date-oldest') {
      return copy.sort((a, b) => (a.date_applied || a.created_at || '').localeCompare(b.date_applied || b.created_at || ''));
    } else if (sortOption === 'company-az') {
      return copy.sort((a, b) => (a.company || '').localeCompare(b.company || ''));
    } else if (sortOption === 'salary-high') {
      return copy.sort((a, b) => (Number(b.salary_max || 0) - Number(a.salary_max || 0)));
    } else if (sortOption === 'interview-soonest') {
      return copy.sort((a, b) => {
        if (!a.next_interview) return 1;
        if (!b.next_interview) return -1;
        return a.next_interview.localeCompare(b.next_interview);
      });
    }
    return copy;
  }

  // --- VIEW 4: ANALYTICS ---
  function renderAnalytics() {
    const container = document.getElementById('analytics-content-area');
    if (applications.length === 0) {
      container.innerHTML = getEmptyStateHtml('Add your applications to see salary targets, workplace ratios, and conversion trends.');
      return;
    }

    const total = applications.length;
    const mins = applications.map(a => Number(a.salary_min)).filter(n => n > 0);
    const maxs = applications.map(a => Number(a.salary_max)).filter(n => n > 0);

    const avgMin = mins.length > 0 ? Math.round(mins.reduce((a, b) => a + b, 0) / mins.length) : 0;
    const avgMax = maxs.length > 0 ? Math.round(maxs.reduce((a, b) => a + b, 0) / maxs.length) : 0;

    const remoteCount = applications.filter(a => (a.work_mode || '').toLowerCase().includes('remote')).length;
    const remotePct = Math.round((remoteCount / total) * 100);

    container.innerHTML = `
      <div class="glass-metrics-grid">
        <div class="glass-metric-card">
          <div class="metric-card-header"><span class="metric-card-label">Average Min Salary</span></div>
          <div class="metric-card-val">$${avgMin.toLocaleString()}</div>
          <span class="metric-card-sub">Based on ${mins.length} documented ranges</span>
        </div>
        <div class="glass-metric-card">
          <div class="metric-card-header"><span class="metric-card-label">Average Max Target</span></div>
          <div class="metric-card-val" style="color: #10b981;">$${avgMax.toLocaleString()}</div>
          <span class="metric-card-sub">Top-end target pay</span>
        </div>
        <div class="glass-metric-card">
          <div class="metric-card-header"><span class="metric-card-label">Remote Share</span></div>
          <div class="metric-card-val" style="color: var(--brand-teal);">${remotePct}%</div>
          <span class="metric-card-sub">${remoteCount} of ${total} applications remote</span>
        </div>
      </div>
    `;
  }

  // --- VIEW 5: STORAGE ---
  async function renderStorage() {
    storageInfo = await API.getStorageInfo();
    if (storageInfo) {
      if (storageInfo.dbPath) {
        document.getElementById('storage-full-path').textContent = storageInfo.dbPath;
        document.getElementById('sidebar-db-path').textContent = storageInfo.dbPath;
      }
      document.getElementById('storage-db-size').textContent = formatBytes(storageInfo.dbSize);
      document.getElementById('storage-total-records').textContent = applications.length;
    }
  }

  // --- MODAL: CREATE / EDIT ---
  function openModal(app = null, defaultStage = 'Applied') {
    const modal = document.getElementById('app-modal');
    if (app) {
      document.getElementById('modal-title-text').textContent = `Edit Application: ${app.company}`;
      document.getElementById('form-id').value = app.id || '';
      document.getElementById('form-company').value = app.company || '';
      document.getElementById('form-role').value = app.role || '';
      
      // Match Stage in dropdown
      const statusSelect = document.getElementById('form-status');
      const matchedOption = Array.from(statusSelect.options).find(o => matchStage(app.status, o.value));
      statusSelect.value = matchedOption ? matchedOption.value : (app.status || defaultStage);

      // Match Work Mode
      const workmodeSelect = document.getElementById('form-workmode');
      const matchedMode = Array.from(workmodeSelect.options).find(o => o.value.toLowerCase() === (app.work_mode || '').toLowerCase());
      workmodeSelect.value = matchedMode ? matchedMode.value : 'Remote';

      // Match Job Type
      const jobtypeSelect = document.getElementById('form-jobtype');
      const matchedType = Array.from(jobtypeSelect.options).find(o => o.value.toLowerCase() === (app.job_type || '').toLowerCase());
      jobtypeSelect.value = matchedType ? matchedType.value : 'Full-time';

      document.getElementById('form-location').value = app.location || '';
      document.getElementById('form-joburl').value = app.job_url || '';
      document.getElementById('form-salary-min').value = app.salary_min || '';
      document.getElementById('form-salary-max').value = app.salary_max || '';

      // Match Currency
      const currSelect = document.getElementById('form-currency');
      currSelect.value = app.currency || '$';

      // Match Salary Period
      const periodSelect = document.getElementById('form-salary-period');
      periodSelect.value = app.salary_period || 'Yearly';

      document.getElementById('form-date-applied').value = app.date_applied || '';
      document.getElementById('form-next-interview').value = app.next_interview || '';
      document.getElementById('form-followup-date').value = app.follow_up_date || '';
      document.getElementById('form-contact-name').value = app.contact_name || '';
      document.getElementById('form-contact-email').value = app.contact_email || '';
      document.getElementById('form-contact-phone').value = app.contact_phone || '';
      document.getElementById('form-tags').value = app.tags || '';
      document.getElementById('form-notes').value = app.notes || '';
    } else {
      document.getElementById('modal-title-text').textContent = 'Add Job Application';
      document.getElementById('form-id').value = '';
      document.getElementById('form-company').value = '';
      document.getElementById('form-role').value = '';
      document.getElementById('form-status').value = defaultStage;
      document.getElementById('form-workmode').value = 'Remote';
      document.getElementById('form-jobtype').value = 'Full-time';
      document.getElementById('form-location').value = '';
      document.getElementById('form-joburl').value = '';
      document.getElementById('form-salary-min').value = '';
      document.getElementById('form-salary-max').value = '';
      document.getElementById('form-currency').value = '$';
      document.getElementById('form-salary-period').value = 'Yearly';
      document.getElementById('form-date-applied').value = new Date().toISOString().split('T')[0];
      document.getElementById('form-next-interview').value = '';
      document.getElementById('form-followup-date').value = '';
      document.getElementById('form-contact-name').value = '';
      document.getElementById('form-contact-email').value = '';
      document.getElementById('form-contact-phone').value = '';
      document.getElementById('form-tags').value = '';
      document.getElementById('form-notes').value = '';
    }
    modal.classList.add('open');
  }

  function closeModal() {
    document.getElementById('app-modal').classList.remove('open');
  }

  async function saveFromModal() {
    const company = document.getElementById('form-company').value.trim();
    const role = document.getElementById('form-role').value.trim();
    if (!company) {
      showToast('Please enter Company name', 'error');
      document.getElementById('form-company').focus();
      return;
    }
    if (!role) {
      showToast('Please enter Role / Job Title', 'error');
      document.getElementById('form-role').focus();
      return;
    }

    const payload = {
      id: document.getElementById('form-id').value || '',
      company,
      role,
      status: document.getElementById('form-status').value,
      work_mode: document.getElementById('form-workmode').value,
      job_type: document.getElementById('form-jobtype').value,
      location: document.getElementById('form-location').value.trim(),
      job_url: document.getElementById('form-joburl').value.trim(),
      salary_min: document.getElementById('form-salary-min').value,
      salary_max: document.getElementById('form-salary-max').value,
      currency: document.getElementById('form-currency').value,
      salary_period: document.getElementById('form-salary-period').value,
      date_applied: document.getElementById('form-date-applied').value,
      next_interview: document.getElementById('form-next-interview').value.trim(),
      follow_up_date: document.getElementById('form-followup-date').value,
      contact_name: document.getElementById('form-contact-name').value.trim(),
      contact_email: document.getElementById('form-contact-email').value.trim(),
      contact_phone: document.getElementById('form-contact-phone').value.trim(),
      tags: document.getElementById('form-tags').value.trim(),
      notes: document.getElementById('form-notes').value.trim()
    };

    await API.saveApplication(payload);
    closeModal();
    await refreshData();
    showToast(`Saved application for ${company}!`);
  }

  async function refreshData() {
    applications = await API.getApplications();
    renderCurrentView();
  }

  // Window globals
  window.openNewModal = (stage = 'Applied') => openModal(null, stage);
  window.editApp = (id) => {
    const a = applications.find(x => x.id === id);
    if (a) openModal(a);
  };
  window.deleteApp = async (id) => {
    const a = applications.find(x => x.id === id);
    if (!a) return;
    if (confirm(`Delete application for "${a.role}" at ${a.company}?`)) {
      await API.deleteApplication(id);
      await refreshData();
      showToast(`Deleted ${a.company}`);
    }
  };
  window.openUrl = (url) => {
    if (!/^https?:\/\//i.test(url)) url = 'https://' + url;
    window.open(url, '_blank');
  };

  // Setup Listeners
  function setupListeners() {
    document.getElementById('btn-add-application').addEventListener('click', () => openModal());
    document.getElementById('btn-close-modal').addEventListener('click', closeModal);
    document.getElementById('btn-cancel-modal').addEventListener('click', closeModal);
    document.getElementById('btn-save-modal').addEventListener('click', saveFromModal);

    // Dropdown Change Handlers
    document.getElementById('filter-status-select').addEventListener('change', (e) => {
      statusFilter = e.target.value;
      renderList();
    });
    document.getElementById('filter-workmode-select').addEventListener('change', (e) => {
      workModeFilter = e.target.value;
      renderList();
    });
    document.getElementById('sort-select').addEventListener('change', (e) => {
      sortOption = e.target.value;
      renderList();
    });

    // CSV Export
    const exportCsvFn = () => {
      if (applications.length === 0) {
        showToast('No applications to export', 'error');
        return;
      }
      window.location.href = '/api/export/csv';
      showToast('Downloading CSV from SQLite...');
    };
    document.getElementById('btn-export-csv-table').addEventListener('click', exportCsvFn);
    document.getElementById('btn-export-csv-action').addEventListener('click', exportCsvFn);

    // Database Reset
    document.getElementById('btn-clear-db-action').addEventListener('click', async () => {
      if (confirm('Are you sure you want to reset the SQLite database and wipe all records?')) {
        await API.clearData();
        await refreshData();
        showToast('Database reset: all records wiped');
      }
    });
  }

  // Initialize
  async function init() {
    initTheme();
    initIcons();
    setupNavigation();
    setupListeners();

    // Load real data from SQLite (ZERO fake/synthetic data)
    applications = await API.getApplications();
    storageInfo = await API.getStorageInfo();

    if (storageInfo && storageInfo.dbPath) {
      document.getElementById('sidebar-db-path').textContent = storageInfo.dbPath;
    }

    renderCurrentView();
  }

  document.addEventListener('DOMContentLoaded', init);
})();
