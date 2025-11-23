#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H

#include <Arduino.h>

// Modern CSS styles embedded as string constants
const char* CSS_STYLES = R"(
<style>
:root {
  --primary-color: #1976d2;
  --primary-dark: #1565c0;
  --secondary-color: #03dac6;
  --background: #fafafa;
  --surface: #ffffff;
  --error: #b00020;
  --warning: #ff9800;
  --success: #4caf50;
  --on-surface: #000000;
  --on-primary: #ffffff;
  --border-radius: 8px;
  --shadow: 0 2px 4px rgba(0,0,0,0.1);
  --transition: all 0.3s ease;
}

* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: var(--background);
  color: var(--on-surface);
  line-height: 1.6;
  padding: 20px;
}

.container {
  max-width: 1200px;
  margin: 0 auto;
  background: var(--surface);
  border-radius: var(--border-radius);
  box-shadow: var(--shadow);
  overflow: hidden;
}

.header {
  background: linear-gradient(135deg, var(--primary-color), var(--primary-dark));
  color: var(--on-primary);
  padding: 24px;
  text-align: center;
}

.header h1 {
  font-size: 2rem;
  font-weight: 300;
  margin-bottom: 8px;
}

.header .version {
  opacity: 0.8;
  font-size: 0.9rem;
}

.nav-tabs {
  display: flex;
  background: var(--surface);
  border-bottom: 1px solid #e0e0e0;
}

.nav-tab {
  flex: 1;
  padding: 16px 24px;
  background: none;
  border: none;
  cursor: pointer;
  font-size: 1rem;
  color: var(--on-surface);
  transition: var(--transition);
  border-bottom: 3px solid transparent;
}

.nav-tab:hover {
  background: #f5f5f5;
}

.nav-tab.active {
  color: var(--primary-color);
  border-bottom-color: var(--primary-color);
}

.content {
  padding: 24px;
}

.tab-content {
  display: none !important;
}

.tab-content.active {
  display: block !important;
}

.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 20px;
  margin-bottom: 24px;
}

.status-card {
  background: var(--surface);
  border: 1px solid #e0e0e0;
  border-radius: var(--border-radius);
  padding: 20px;
  box-shadow: var(--shadow);
  transition: var(--transition);
}

.status-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0,0,0,0.15);
}

.card-header {
  display: flex;
  align-items: center;
  margin-bottom: 16px;
}

.card-icon {
  width: 24px;
  height: 24px;
  margin-right: 12px;
  color: var(--primary-color);
}

.card-title {
  font-size: 1.1rem;
  font-weight: 500;
  color: var(--on-surface);
}

.temp-display {
  font-size: 3rem;
  font-weight: 300;
  color: var(--primary-color);
  text-align: center;
  margin: 16px 0;
}

.temp-unit {
  font-size: 1.5rem;
  opacity: 0.7;
}

.status-indicator {
  display: inline-block;
  padding: 4px 12px;
  border-radius: 16px;
  font-size: 0.8rem;
  font-weight: 500;
  text-transform: uppercase;
}

.status-on {
  background: var(--success);
  color: white;
}

.status-off {
  background: #757575;
  color: white;
}

.status-auto {
  background: var(--warning);
  color: white;
}

.form-group {
  margin-bottom: 20px;
}

.form-label {
  display: block;
  margin-bottom: 8px;
  font-weight: 500;
  color: var(--on-surface);
}

.form-input, .form-select {
  width: 100%;
  padding: 12px 16px;
  border: 2px solid #e0e0e0;
  border-radius: var(--border-radius);
  font-size: 1rem;
  transition: var(--transition);
  background: var(--surface);
}

.form-input:focus, .form-select:focus {
  outline: none;
  border-color: var(--primary-color);
  box-shadow: 0 0 0 3px rgba(25, 118, 210, 0.1);
}

.form-checkbox {
  display: flex;
  align-items: center;
  margin-bottom: 16px;
}

.form-checkbox input {
  margin-right: 12px;
  transform: scale(1.2);
}

.btn {
  display: inline-block;
  padding: 12px 24px;
  border: none;
  border-radius: var(--border-radius);
  font-size: 1rem;
  font-weight: 500;
  cursor: pointer;
  text-decoration: none;
  text-align: center;
  transition: var(--transition);
  margin: 4px;
}

.btn-primary {
  background: var(--primary-color);
  color: var(--on-primary);
}

.btn-primary:hover {
  background: var(--primary-dark);
  transform: translateY(-1px);
}

.btn-secondary {
  background: #6c757d;
  color: white;
}

.btn-secondary:hover {
  background: #545b62;
}

.btn-warning {
  background: var(--warning);
  color: white;
}

.btn-warning:hover {
  background: #e68900;
}

.btn-danger {
  background: var(--error);
  color: white;
}

.btn-danger:hover {
  background: #8e0000;
}

.progress-bar {
  width: 100%;
  height: 8px;
  background: #e0e0e0;
  border-radius: 4px;
  overflow: hidden;
  margin: 16px 0;
}

.progress-fill {
  height: 100%;
  background: var(--primary-color);
  transition: width 0.3s ease;
}

.alert {
  padding: 16px;
  border-radius: var(--border-radius);
  margin-bottom: 20px;
  border-left: 4px solid;
}

.alert-success {
  background: #d4edda;
  color: #155724;
  border-color: var(--success);
}

.alert-warning {
  background: #fff3cd;
  color: #856404;
  border-color: var(--warning);
}

.alert-error {
  background: #f8d7da;
  color: #721c24;
  border-color: var(--error);
}

.settings-section {
  margin-bottom: 32px;
  padding: 24px;
  border: 1px solid #e0e0e0;
  border-radius: var(--border-radius);
}

.settings-section h3 {
  margin-bottom: 20px;
  color: var(--primary-color);
  border-bottom: 2px solid #e0e0e0;
  padding-bottom: 8px;
}

.info-card {
  background: #f5f5f5;
  border-radius: var(--border-radius);
  padding: 16px;
  margin-top: 16px;
}

.info-card h4 {
  margin-bottom: 12px;
  color: var(--primary-color);
}

.info-card p {
  margin-bottom: 8px;
  color: var(--on-surface);
}

.button-group {
  display: flex;
  gap: 12px;
  margin-top: 24px;
  flex-wrap: wrap;
}

.system-status {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 16px;
  margin-bottom: 24px;
}

.relay-status {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: #f8f9fa;
  border-radius: var(--border-radius);
  border-left: 4px solid #dee2e6;
}

.relay-status.active {
  background: #e8f5e8;
  border-left-color: var(--success);
}

@media (max-width: 768px) {
  body {
    padding: 10px;
  }
  
  .header {
    padding: 16px;
  }
  
  .header h1 {
    font-size: 1.5rem;
  }
  
  .nav-tab {
    padding: 12px 16px;
    font-size: 0.9rem;
  }
  
  .content {
    padding: 16px;
  }
  
  .temp-display {
    font-size: 2.5rem;
  }
  
  .status-grid {
    grid-template-columns: 1fr;
  }
  
  .button-group {
    flex-direction: column;
  }
  
  .btn {
    width: 100%;
  }
}

.loading {
  display: inline-block;
  width: 20px;
  height: 20px;
  border: 3px solid #f3f3f3;
  border-top: 3px solid var(--primary-color);
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.fade-in {
  animation: fadeIn 0.5s ease-in;
}

@keyframes fadeIn {
  0% { opacity: 0; transform: translateY(10px); }
  100% { opacity: 1; transform: translateY(0); }
}

/* Schedule table styles */
.schedule-table {
  display: flex;
  flex-direction: column;
  border: 1px solid var(--border-color);
  border-radius: 8px;
  overflow: hidden;
  background: white;
  margin: 16px 0;
}

.schedule-row {
  display: grid;
  grid-template-columns: 1fr auto 1fr 2fr 1fr 2fr;
  gap: 8px;
  padding: 12px 16px;
  border-bottom: 1px solid var(--border-color);
  align-items: center;
}

.schedule-row:last-child {
  border-bottom: none;
}

.schedule-header {
  background: var(--primary-color);
  color: white;
  font-weight: 600;
  font-size: 0.9rem;
}

.schedule-cell {
  display: flex;
  align-items: center;
  justify-content: center;
  text-align: center;
  min-height: 40px;
}

.schedule-cell:first-child {
  justify-content: flex-start;
  text-align: left;
}

.temp-inputs {
  display: flex;
  gap: 4px;
  flex-direction: column;
}

.temp-input {
  width: 70px !important;
  min-width: 70px;
  font-size: 0.85rem;
  padding: 4px 6px;
}

.time-input {
  width: 90px !important;
  min-width: 90px;
  font-size: 0.85rem;
  padding: 4px 6px;
}

.toggle-switch.small {
  transform: scale(0.8);
}

/* Responsive schedule table */
@media (max-width: 768px) {
  .schedule-row {
    grid-template-columns: 1fr;
    gap: 8px;
    text-align: left;
  }
  
  .schedule-cell {
    justify-content: flex-start;
    text-align: left;
    padding: 4px 0;
  }
  
  .schedule-header .schedule-cell {
    display: none;
  }
  
  .schedule-header::before {
    content: "Schedule Configuration";
    font-weight: 600;
  }
  
  .temp-inputs {
    flex-direction: row;
    gap: 8px;
  }
  
  .temp-input, .time-input {
    width: auto !important;
    min-width: 60px;
    flex: 1;
  }
}

.temp-label {
  font-size: 0.75rem;
  font-weight: 600;
  color: var(--text-color);
  margin-bottom: 2px;
  display: block;
}
</style>
)";

// SVG Icons as string constants
const char* ICON_TEMPERATURE = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M15 13V5a3 3 0 0 0-6 0v8a5 5 0 1 0 6 0zm-3 4a1 1 0 1 1 0-2 1 1 0 0 1 0 2zm0-4a1 1 0 0 0-1 1v.5L9 16a3 3 0 1 0 6 0l-2-1.5V14a1 1 0 0 0-1-1z"/></svg>)";

const char* ICON_HUMIDITY = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,2C13.09,2 14.07,2.37 14.84,3.16L22.07,10.39C22.86,11.16 23.23,12.14 23.23,13.23C23.23,15.5 21.43,17.3 19.16,17.3C18.07,17.3 17.09,16.93 16.32,16.16L12,11.84L7.68,16.16C6.91,16.93 5.93,17.3 4.84,17.3C2.57,17.3 0.77,15.5 0.77,13.23C0.77,12.14 1.14,11.16 1.93,10.39L9.16,3.16C9.93,2.37 10.91,2 12,2M12,4.89L6.5,10.39C6.06,10.83 5.82,11.42 5.82,12.04C5.82,13.32 6.85,14.35 8.13,14.35C8.75,14.35 9.34,14.11 9.78,13.67L12,11.45L14.22,13.67C14.66,14.11 15.25,14.35 15.87,14.35C17.15,14.35 18.18,13.32 18.18,12.04C18.18,11.42 17.94,10.83 17.5,10.39L12,4.89Z"/></svg>)";

const char* ICON_THERMOSTAT = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M16 12a4 4 0 0 1-4 4 4 4 0 0 1-4-4 4 4 0 0 1 4-4 4 4 0 0 1 4 4m4 0a8 8 0 0 1-8 8 8 8 0 0 1-8-8 8 8 0 0 1 8-8 8 8 0 0 1 8 8M12 2l1.09 2.41L16 5.91l-2.91 1.5L12 10l-1.09-2.59L8 5.91l2.91-1.5L12 2m-8 10l1.09 2.41L8 15.91l-2.91 1.5L4 20l-1.09-2.59L0 15.91l2.91-1.5L4 12m16 0l1.09 2.41L24 15.91l-2.91 1.5L20 20l-1.09-2.59L16 15.91l2.91-1.5L20 12z"/></svg>)";

const char* ICON_RELAY = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,2A2,2 0 0,1 14,4C14,4.74 13.6,5.39 13,5.73V7H14A7,7 0 0,1 21,14H22A1,1 0 0,1 23,15V18A1,1 0 0,1 22,19H21A7,7 0 0,1 14,26H10A7,7 0 0,1 3,19H2A1,1 0 0,1 1,18V15A1,1 0 0,1 2,14H3A7,7 0 0,1 10,7H11V5.73C10.4,5.39 10,4.74 10,4A2,2 0 0,1 12,2M12,4.5A0.5,0.5 0 0,0 11.5,4A0.5,0.5 0 0,0 12,3.5A0.5,0.5 0 0,0 12.5,4A0.5,0.5 0 0,0 12,4.5M10,9A5,5 0 0,0 5,14V17H7V14A3,3 0 0,1 10,11H14A3,3 0 0,1 17,14V17H19V14A5,5 0 0,0 14,9H10Z"/></svg>)";

const char* ICON_SETTINGS = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,15.5A3.5,3.5 0 0,1 8.5,12A3.5,3.5 0 0,1 12,8.5A3.5,3.5 0 0,1 15.5,12A3.5,3.5 0 0,1 12,15.5M19.43,12.97C19.47,12.65 19.5,12.33 19.5,12C19.5,11.67 19.47,11.34 19.43,11L21.54,9.37C21.73,9.22 21.78,8.95 21.66,8.73L19.66,5.27C19.54,5.05 19.27,4.96 19.05,5.05L16.56,6.05C16.04,5.66 15.5,5.32 14.87,5.07L14.5,2.42C14.46,2.18 14.25,2 14,2H10C9.75,2 9.54,2.18 9.5,2.42L9.13,5.07C8.5,5.32 7.96,5.66 7.44,6.05L4.95,5.05C4.73,4.96 4.46,5.05 4.34,5.27L2.34,8.73C2.22,8.95 2.27,9.22 2.46,9.37L4.57,11C4.53,11.34 4.5,11.67 4.5,12C4.5,12.33 4.53,12.65 4.57,12.97L2.46,14.63C2.27,14.78 2.22,15.05 2.34,15.27L4.34,18.73C4.46,18.95 4.73,19.03 4.95,18.95L7.44,17.94C7.96,18.34 8.5,18.68 9.13,18.93L9.5,21.58C9.54,21.82 9.75,22 10,22H14C14.25,22 14.46,21.82 14.5,21.58L14.87,18.93C15.5,18.68 16.04,18.34 16.56,17.94L19.05,18.95C19.27,19.03 19.54,18.95 19.66,18.73L21.66,15.27C21.78,15.05 21.73,14.78 21.54,14.63L19.43,12.97Z"/></svg>)";

const char* ICON_WIFI = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,21L15.6,17.42C14.63,16.44 13.38,15.9 12,15.9C10.62,15.9 9.37,16.44 8.4,17.42L12,21M12,3C7.95,3 4.21,4.34 1.2,6.6L3,8.4C5.5,6.63 8.62,5.5 12,5.5C15.38,5.5 18.5,6.63 21,8.4L22.8,6.6C19.79,4.34 16.05,3 12,3M12,9C9.3,9 6.81,9.89 4.8,11.4L6.6,13.2C8.1,12.05 9.97,11.4 12,11.4C14.03,11.4 15.9,12.05 17.4,13.2L19.2,11.4C17.19,9.89 14.7,9 12,9Z"/></svg>)";

const char* ICON_MODE = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20A8,8 0 0,1 4,12A8,8 0 0,1 12,4M12,6A6,6 0 0,0 6,12A6,6 0 0,0 12,18A6,6 0 0,0 18,12A6,6 0 0,0 12,6Z"/></svg>)";

const char* ICON_FAN = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,11A1,1 0 0,0 11,12A1,1 0 0,0 12,13A1,1 0 0,0 13,12A1,1 0 0,0 12,11M12.5,2C13,2 13.5,2.19 13.9,2.6L22.4,11.1C22.8,11.5 23,12 23,12.5C23,13 22.8,13.5 22.4,13.9L13.9,22.4C13.5,22.8 13,23 12.5,23C12,23 11.5,22.8 11.1,22.4L2.6,13.9C2.2,13.5 2,13 2,12.5C2,12 2.2,11.5 2.6,11.1L11.1,2.6C11.5,2.19 12,2 12.5,2Z"/></svg>)";

const char* ICON_UPDATE = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M14,2H6A2,2 0 0,0 4,4V20A2,2 0 0,0 6,22H18A2,2 0 0,0 20,20V8L14,2M18,20H6V4H13V9H18V20Z"/></svg>)";

const char* ICON_RESET = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,2C13.1,2 14,2.9 14,4C14,5.1 13.1,6 12,6C10.9,6 10,5.1 10,4C10,2.9 10.9,2 12,2M21,9V7L15,13L21,19V17H23V9H21M1,9V17H3V19L9,13L3,7V9H1Z"/></svg>)";

const char* ICON_CLOCK = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M12,2A10,10 0 0,0 2,12A10,10 0 0,0 12,22A10,10 0 0,0 22,12A10,10 0 0,0 12,2M12,4A8,8 0 0,1 20,12A8,8 0 0,1 12,20A8,8 0 0,1 4,12A8,8 0 0,1 12,4M12.5,7V12.25L17,14.92L16.25,16.15L11,13V7H12.5Z"/></svg>)";

const char* ICON_CALENDAR = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M19,3H18V1H16V3H8V1H6V3H5A2,2 0 0,0 3,5V19A2,2 0 0,0 5,21H19A2,2 0 0,0 21,19V5A2,2 0 0,0 19,3M19,19H5V8H19V19M5,6V5H6V6H8V5H16V6H18V5H19V6H19V8H5V6Z"/></svg>)";

const char* ICON_SCHEDULE = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M14,12H15.5V14.82L17.94,16.23L17.19,17.53L14,15.69V12M4,2V4H5V20A2,2 0 0,0 7,22H11.4C11,21.4 10.6,20.73 10.33,20H7V4H8V2H10V4H14V2H16V4H17A2,2 0 0,1 19,6V10.1C19.74,10.36 20.42,10.73 21,11.19V6A2,2 0 0,0 19,4H18V2H16M18.5,13.5A6.5,6.5 0 0,1 12,20A6.5,6.5 0 0,1 5.5,13.5A6.5,6.5 0 0,1 12,7A6.5,6.5 0 0,1 18.5,13.5Z"/></svg>)";

const char* ICON_BACK = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M20,11V13H8L13.5,18.5L12.08,19.92L4.16,12L12.08,4.08L13.5,5.5L8,11H20Z"/></svg>)";

const char* ICON_SAVE = R"(<svg class="card-icon" viewBox="0 0 24 24" fill="currentColor"><path d="M15,9H5V5H15M12,19A3,3 0 0,1 9,16A3,3 0 0,1 12,13A3,3 0 0,1 15,16A3,3 0 0,1 12,19M17,3H5C3.89,3 3,3.9 3,5V19A2,2 0 0,0 5,21H19A2,2 0 0,0 21,19V7L17,3Z"/></svg>)";

// JavaScript for dynamic functionality
const char* JAVASCRIPT_CODE = R"(
<script>
let currentTab = 'status';
let updateInterval;

function showTab(tabName) {
    // Hide all tab contents
    const contents = document.querySelectorAll('.tab-content');
    contents.forEach(content => {
        content.classList.remove('active');
        content.classList.remove('fade-in');
    });
    
    // Remove active class from all tabs
    const tabs = document.querySelectorAll('.nav-tab');
    tabs.forEach(tab => tab.classList.remove('active'));
    
    // Show selected tab content
    const selectedContent = document.getElementById(tabName + '-content');
    if (selectedContent) {
        selectedContent.classList.add('active');
        selectedContent.classList.add('fade-in');
    }
    
    // Add active class to selected tab
    const selectedTab = document.querySelector("[onclick=\"showTab('" + tabName + "')\"]");
    if (selectedTab) {
        selectedTab.classList.add('active');
    }
    
    currentTab = tabName;
    
    // Handle auto-refresh for status tab
    if (tabName === 'status') {
        startAutoRefresh();
    } else {
        stopAutoRefresh();
    }
}

function startAutoRefresh() {
    stopAutoRefresh();
    updateInterval = setInterval(() => {
        if (currentTab === 'status') {
            refreshStatus();
        }
    }, 10000);
}

function stopAutoRefresh() {
    if (updateInterval) {
        clearInterval(updateInterval);
    }
}

function refreshStatus() {
    // Add subtle loading indicator
    const statusCards = document.querySelectorAll('.status-card');
    statusCards.forEach(card => card.style.opacity = '0.7');
    
    // Reload the page to get fresh data
    // In a real implementation, this would be an AJAX call
    setTimeout(() => {
        if (currentTab === 'status') {
            location.reload();
        }
    }, 500);
}

function confirmAction(actionName, actionUrl) {
    if (confirm("Are you sure you want to " + actionName + "?")) {
        window.location.href = actionUrl;
    }
}

function showAlert(message, type) {
    if (typeof type === 'undefined') type = 'success';
    const alertDiv = document.createElement('div');
    alertDiv.className = 'alert alert-' + type;
    alertDiv.textContent = message;
    
    const container = document.querySelector('.container');
    container.insertBefore(alertDiv, container.firstChild);
    
    setTimeout(() => {
        alertDiv.remove();
    }, 5000);
}

function handleSettingsSubmit(event) {
    event.preventDefault();
    
    const form = event.target;
    const formData = new FormData(form);
    const submitBtn = form.querySelector('input[type="submit"]');
    
    // Show loading state
    const originalValue = submitBtn.value;
    submitBtn.value = 'Saving...';
    submitBtn.disabled = true;
    
    fetch('/set', {
        method: 'POST',
        body: formData
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'success') {
            showAlert(data.message, 'success');
            // Optional: refresh the page to show updated values
            setTimeout(() => {
                location.reload();
            }, 2000);
        } else {
            showAlert('Error saving settings: ' + (data.message || 'Unknown error'), 'error');
        }
    })
    .catch(error => {
        showAlert('Error saving settings: ' + error.message, 'error');
    })
    .finally(() => {
        // Restore button state
        submitBtn.value = originalValue;
        submitBtn.disabled = false;
    });
    
    return false;
}

// Initialize the interface when page loads
document.addEventListener('DOMContentLoaded', function() {
    // Check URL parameters for tab switching
    const urlParams = new URLSearchParams(window.location.search);
    const tabParam = urlParams.get('tab');
    const initialTab = tabParam && ['status', 'settings', 'system'].includes(tabParam) ? tabParam : 'status';
    showTab(initialTab);
    
    // Add form validation
    const forms = document.querySelectorAll('form');
    forms.forEach(form => {
        form.addEventListener('submit', function(e) {
            const submitBtn = form.querySelector('input[type="submit"]');
            if (submitBtn) {
                submitBtn.value = 'Saving...';
                submitBtn.disabled = true;
            }
        });
    });
});

// Handle page visibility for auto-refresh
document.addEventListener('visibilitychange', function() {
    if (document.hidden) {
        stopAutoRefresh();
    } else if (currentTab === 'status') {
        startAutoRefresh();
    }
});
</script>
)";

#endif // WEBINTERFACE_H