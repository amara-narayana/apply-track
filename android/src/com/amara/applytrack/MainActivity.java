package com.amara.applytrack;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ShortcutInfo;
import android.content.pm.ShortcutManager;
import android.graphics.Color;
import android.graphics.drawable.Icon;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;

public class MainActivity extends Activity {

    private DatabaseHelper dbHelper;
    private static final String PREFS_NAME = "applytrack_native_prefs";
    private static final String KEY_PINNED = "shortcut_pinned_prompt";

    // State
    private String currentFilter = "ALL";
    private String currentSearch = "";
    private int currentTab = 0; // 0: Applications, 1: Kanban, 2: Vault
    private boolean isDarkMode = true;

    // Views
    private View rootLayout;
    private TextView tvAppTitle;
    private TextView tvMetricTotal, tvMetricApplied, tvMetricInterviews, tvMetricOffers;
    private LinearLayout llCardList, llEmptyState, llKanbanColumns;
    private ScrollView viewApplications, viewKanban, viewVault;
    private TextView tabApplications, tabKanban, tabVault;
    private EditText etSearch;
    private Button btnNewApp, btnExportCsv, btnClearDb, btnPinHome, btnThemeToggle;
    private TextView tvVaultFullPath, tvVaultDbSize;
    private Button[] filterChips;

    private static final String[] STAGES = {
        "Wishlist", "Applied", "Screening", "Technical Interview", "Final Round", "Offer", "Rejected"
    };
    private static final String[] WORK_MODES = { "Remote", "Hybrid", "On-site" };
    private static final String[] JOB_TYPES = { "Full-time", "Part-time", "Contract", "Internship" };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        dbHelper = new DatabaseHelper(this);

        initViews();
        setupListeners();
        applyFilterChip("ALL");
        switchTab(0);
        refreshAllData();

        // Automatically prompt to add to Home Screen after first open
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                addShortcutToHomeScreen(false);
            }
        }, 1200);
    }

    private void initViews() {
        rootLayout = findViewById(R.id.root_layout);
        tvAppTitle = findViewById(R.id.tv_app_title);

        tvMetricTotal = findViewById(R.id.tv_metric_total);
        tvMetricApplied = findViewById(R.id.tv_metric_applied);
        tvMetricInterviews = findViewById(R.id.tv_metric_interviews);
        tvMetricOffers = findViewById(R.id.tv_metric_offers);

        etSearch = findViewById(R.id.et_search);

        viewApplications = findViewById(R.id.view_applications);
        viewKanban = findViewById(R.id.view_kanban);
        viewVault = findViewById(R.id.view_vault);

        llCardList = findViewById(R.id.ll_card_list);
        llEmptyState = findViewById(R.id.ll_empty_state);
        llKanbanColumns = findViewById(R.id.ll_kanban_columns);

        tabApplications = findViewById(R.id.tab_applications);
        tabKanban = findViewById(R.id.tab_kanban);
        tabVault = findViewById(R.id.tab_vault);

        btnNewApp = findViewById(R.id.btn_new_app);
        btnPinHome = findViewById(R.id.btn_pin_home);
        btnThemeToggle = findViewById(R.id.btn_theme_toggle);
        btnExportCsv = findViewById(R.id.btn_export_csv);
        btnClearDb = findViewById(R.id.btn_clear_db);

        tvVaultFullPath = findViewById(R.id.tv_vault_full_path);
        tvVaultDbSize = findViewById(R.id.tv_vault_db_size);

        filterChips = new Button[]{
            findViewById(R.id.chip_all),
            findViewById(R.id.chip_applied),
            findViewById(R.id.chip_screening),
            findViewById(R.id.chip_interview),
            findViewById(R.id.chip_offer),
            findViewById(R.id.chip_rejected)
        };
    }

    private void setupListeners() {
        // Tab switching
        tabApplications.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { switchTab(0); }
        });
        tabKanban.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { switchTab(1); }
        });
        tabVault.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { switchTab(2); }
        });

        // Filter chips
        findViewById(R.id.chip_all).setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { applyFilterChip("ALL"); }
        });
        findViewById(R.id.chip_applied).setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { applyFilterChip("Applied"); }
        });
        findViewById(R.id.chip_screening).setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { applyFilterChip("Screening"); }
        });
        findViewById(R.id.chip_interview).setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { applyFilterChip("Technical Interview"); }
        });
        findViewById(R.id.chip_offer).setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { applyFilterChip("Offer"); }
        });
        findViewById(R.id.chip_rejected).setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { applyFilterChip("Rejected"); }
        });

        // Search text watcher
        etSearch.addTextChangedListener(new TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) {}
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {
                currentSearch = s.toString();
                refreshApplicationsList();
            }
            @Override public void afterTextChanged(Editable s) {}
        });

        // Add application
        btnNewApp.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { showJobFormDialog(null, currentFilter.equals("ALL") ? "Applied" : currentFilter); }
        });

        // Pin to home
        btnPinHome.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { addShortcutToHomeScreen(true); }
        });

        // Theme toggle
        btnThemeToggle.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { toggleTheme(); }
        });

        // Export CSV
        btnExportCsv.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { shareCsvExport(); }
        });

        // Clear DB
        btnClearDb.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { confirmClearDatabase(); }
        });
    }

    private void switchTab(int tabIndex) {
        currentTab = tabIndex;
        viewApplications.setVisibility(tabIndex == 0 ? View.VISIBLE : View.GONE);
        viewKanban.setVisibility(tabIndex == 1 ? View.VISIBLE : View.GONE);
        viewVault.setVisibility(tabIndex == 2 ? View.VISIBLE : View.GONE);

        int activeColor = Color.parseColor("#2dd4bf");
        int inactiveColor = Color.parseColor("#94a3b8");

        tabApplications.setTextColor(tabIndex == 0 ? activeColor : inactiveColor);
        tabKanban.setTextColor(tabIndex == 1 ? activeColor : inactiveColor);
        tabVault.setTextColor(tabIndex == 2 ? activeColor : inactiveColor);

        if (tabIndex == 0) refreshApplicationsList();
        else if (tabIndex == 1) refreshKanbanBoard();
        else if (tabIndex == 2) refreshVaultInfo();
    }

    private void applyFilterChip(String filter) {
        currentFilter = filter;
        for (Button chip : filterChips) {
            String text = chip.getText().toString();
            boolean isActive = false;
            if (filter.equalsIgnoreCase("ALL") && text.equalsIgnoreCase("All")) isActive = true;
            else if (filter.equalsIgnoreCase("Technical Interview") && text.equalsIgnoreCase("Interview")) isActive = true;
            else if (filter.equalsIgnoreCase(text)) isActive = true;

            if (isActive) {
                chip.setBackgroundResource(R.drawable.chip_active);
                chip.setTextColor(Color.parseColor("#0a0f1d"));
            } else {
                chip.setBackgroundResource(R.drawable.chip_inactive);
                chip.setTextColor(Color.parseColor("#ffffff"));
            }
        }
        refreshApplicationsList();
    }

    private void refreshAllData() {
        refreshMetrics();
        refreshApplicationsList();
        refreshKanbanBoard();
        refreshVaultInfo();
    }

    private void refreshMetrics() {
        int[] counts = dbHelper.getMetricsCounts();
        tvMetricTotal.setText(String.valueOf(counts[0]));
        tvMetricApplied.setText(String.valueOf(counts[1]));
        tvMetricInterviews.setText(String.valueOf(counts[2]));
        tvMetricOffers.setText(String.valueOf(counts[3]));
    }

    private void refreshApplicationsList() {
        llCardList.removeAllViews();
        List<DatabaseHelper.JobApplication> apps = dbHelper.queryApplications(currentFilter, currentSearch);

        if (apps.isEmpty()) {
            llEmptyState.setVisibility(View.VISIBLE);
        } else {
            llEmptyState.setVisibility(View.GONE);
            LayoutInflater inflater = LayoutInflater.from(this);

            for (final DatabaseHelper.JobApplication app : apps) {
                View card = inflater.inflate(R.layout.item_job_card, llCardList, false);

                TextView tvRole = card.findViewById(R.id.tv_card_role);
                TextView tvCompany = card.findViewById(R.id.tv_card_company);
                TextView tvStatus = card.findViewById(R.id.tv_card_status);
                TextView tvMeta = card.findViewById(R.id.tv_card_meta);
                TextView tvSalary = card.findViewById(R.id.tv_card_salary);
                TextView tvDateApplied = card.findViewById(R.id.tv_card_date_applied);
                TextView tvNextInterview = card.findViewById(R.id.tv_card_next_interview);
                Button btnStage = card.findViewById(R.id.btn_card_stage);
                Button btnEdit = card.findViewById(R.id.btn_card_edit);
                Button btnDelete = card.findViewById(R.id.btn_card_delete);

                tvRole.setText(app.role);
                tvCompany.setText(app.company);
                tvStatus.setText(app.status);
                bindStatusBadge(tvStatus, app.status);

                StringBuilder meta = new StringBuilder();
                if (!app.workMode.isEmpty()) meta.append(app.workMode);
                if (!app.jobType.isEmpty()) {
                    if (meta.length() > 0) meta.append(" • ");
                    meta.append(app.jobType);
                }
                if (!app.location.isEmpty()) {
                    if (meta.length() > 0) meta.append(" • ");
                    meta.append(app.location);
                }
                tvMeta.setText(meta.toString());

                if (app.salaryMin > 0 || app.salaryMax > 0) {
                    tvSalary.setVisibility(View.VISIBLE);
                    String period = app.salaryPeriod.equalsIgnoreCase("Yearly") ? "/yr" : "/mo";
                    if (app.salaryMin > 0 && app.salaryMax > 0) {
                        tvSalary.setText(String.format(Locale.US, "%s%,.0f - %s%,.0f%s", app.currency, app.salaryMin, app.currency, app.salaryMax, period));
                    } else if (app.salaryMin > 0) {
                        tvSalary.setText(String.format(Locale.US, "%s%,.0f+%s", app.currency, app.salaryMin, period));
                    } else {
                        tvSalary.setText(String.format(Locale.US, "Up to %s%,.0f%s", app.currency, app.salaryMax, period));
                    }
                } else {
                    tvSalary.setVisibility(View.GONE);
                }

                if (!app.dateApplied.isEmpty()) {
                    tvDateApplied.setText("Applied: " + app.dateApplied);
                } else {
                    tvDateApplied.setText("");
                }

                if (!app.nextInterview.isEmpty()) {
                    tvNextInterview.setVisibility(View.VISIBLE);
                    tvNextInterview.setText("Interview: " + app.nextInterview);
                } else {
                    tvNextInterview.setVisibility(View.GONE);
                }

                // Stage change button
                btnStage.setOnClickListener(new View.OnClickListener() {
                    @Override public void onClick(View v) { showStageSelectorDialog(app); }
                });

                // Edit button
                btnEdit.setOnClickListener(new View.OnClickListener() {
                    @Override public void onClick(View v) { showJobFormDialog(app, app.status); }
                });

                // Delete button
                btnDelete.setOnClickListener(new View.OnClickListener() {
                    @Override public void onClick(View v) { confirmDelete(app); }
                });

                llCardList.addView(card);
            }
        }
    }

    private void bindStatusBadge(TextView tv, String status) {
        if ("Applied".equalsIgnoreCase(status)) {
            tv.setBackgroundResource(R.drawable.pill_status_applied);
            tv.setTextColor(Color.parseColor("#2dd4bf"));
        } else if ("Screening".equalsIgnoreCase(status)) {
            tv.setBackgroundResource(R.drawable.pill_status_screening);
            tv.setTextColor(Color.parseColor("#f59e0b"));
        } else if (status != null && status.toLowerCase(Locale.ROOT).contains("interview")) {
            tv.setBackgroundResource(R.drawable.pill_status_interview);
            tv.setTextColor(Color.parseColor("#a855f7"));
        } else if ("Offer".equalsIgnoreCase(status)) {
            tv.setBackgroundResource(R.drawable.pill_status_offer);
            tv.setTextColor(Color.parseColor("#10b981"));
        } else if ("Rejected".equalsIgnoreCase(status)) {
            tv.setBackgroundResource(R.drawable.pill_status_rejected);
            tv.setTextColor(Color.parseColor("#f43f5e"));
        } else {
            tv.setBackgroundResource(R.drawable.pill_status_wishlist);
            tv.setTextColor(Color.parseColor("#94a3b8"));
        }
    }

    private void refreshKanbanBoard() {
        llKanbanColumns.removeAllViews();
        LayoutInflater inflater = LayoutInflater.from(this);

        for (final String stageName : STAGES) {
            List<DatabaseHelper.JobApplication> stageApps = dbHelper.queryApplications(stageName, currentSearch);

            LinearLayout col = new LinearLayout(this);
            col.setOrientation(LinearLayout.VERTICAL);
            col.setLayoutParams(new LinearLayout.LayoutParams(dpToPx(280), ViewGroup.LayoutParams.WRAP_CONTENT));
            col.setPadding(dpToPx(8), dpToPx(8), dpToPx(8), dpToPx(8));
            col.setBackgroundResource(isDarkMode ? R.drawable.glass_card_dark : R.drawable.glass_card_light);

            LinearLayout.LayoutParams colLp = (LinearLayout.LayoutParams) col.getLayoutParams();
            colLp.setMargins(0, 0, dpToPx(12), 0);
            col.setLayoutParams(colLp);

            // Column Header
            LinearLayout header = new LinearLayout(this);
            header.setOrientation(LinearLayout.HORIZONTAL);
            header.setPadding(dpToPx(4), dpToPx(4), dpToPx(4), dpToPx(8));

            TextView tvTitle = new TextView(this);
            tvTitle.setText(stageName);
            tvTitle.setTextColor(isDarkMode ? Color.WHITE : Color.parseColor("#0f172a"));
            tvTitle.setTextSize(14);
            tvTitle.setLayoutParams(new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

            TextView tvCount = new TextView(this);
            tvCount.setText(String.valueOf(stageApps.size()));
            tvCount.setTextSize(12);
            tvCount.setTextColor(Color.parseColor("#2dd4bf"));
            tvCount.setBackgroundResource(R.drawable.chip_inactive);
            tvCount.setPadding(dpToPx(8), dpToPx(2), dpToPx(8), dpToPx(2));

            header.addView(tvTitle);
            header.addView(tvCount);
            col.addView(header);

            if (stageApps.isEmpty()) {
                TextView empty = new TextView(this);
                empty.setText("No jobs in this stage");
                empty.setTextSize(12);
                empty.setTextColor(Color.parseColor("#64748b"));
                empty.setPadding(dpToPx(8), dpToPx(24), dpToPx(8), dpToPx(24));
                col.addView(empty);
            } else {
                for (final DatabaseHelper.JobApplication app : stageApps) {
                    View item = inflater.inflate(R.layout.item_job_card, col, false);
                    TextView tvRole = item.findViewById(R.id.tv_card_role);
                    TextView tvCompany = item.findViewById(R.id.tv_card_company);
                    TextView tvStatus = item.findViewById(R.id.tv_card_status);
                    TextView tvMeta = item.findViewById(R.id.tv_card_meta);
                    TextView tvSalary = item.findViewById(R.id.tv_card_salary);
                    item.findViewById(R.id.btn_card_stage).setOnClickListener(new View.OnClickListener() {
                        @Override public void onClick(View v) { showStageSelectorDialog(app); }
                    });
                    item.findViewById(R.id.btn_card_edit).setOnClickListener(new View.OnClickListener() {
                        @Override public void onClick(View v) { showJobFormDialog(app, app.status); }
                    });
                    item.findViewById(R.id.btn_card_delete).setOnClickListener(new View.OnClickListener() {
                        @Override public void onClick(View v) { confirmDelete(app); }
                    });

                    tvRole.setText(app.role);
                    tvCompany.setText(app.company);
                    tvStatus.setText(app.status);
                    bindStatusBadge(tvStatus, app.status);
                    tvMeta.setText(app.workMode + " • " + app.location);
                    if (app.salaryMax > 0) {
                        tvSalary.setText(String.format(Locale.US, "$%,.0f", app.salaryMax));
                    } else {
                        tvSalary.setVisibility(View.GONE);
                    }
                    col.addView(item);
                }
            }

            llKanbanColumns.addView(col);
        }
    }

    private void refreshVaultInfo() {
        tvVaultFullPath.setText(dbHelper.getDbPath());
        long size = dbHelper.getDbFileSize();
        tvVaultDbSize.setText(String.format(Locale.US, "Database File Size: %.1f KB (%d bytes)", (size / 1024.0), size));
    }

    private void showJobFormDialog(final DatabaseHelper.JobApplication existingApp, String defaultStage) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = LayoutInflater.from(this);
        View dialogView = inflater.inflate(R.layout.dialog_job_form, null);
        builder.setView(dialogView);

        final AlertDialog dialog = builder.create();

        final TextView tvTitle = dialogView.findViewById(R.id.tv_dialog_title);
        final EditText etCompany = dialogView.findViewById(R.id.dialog_et_company);
        final EditText etRole = dialogView.findViewById(R.id.dialog_et_role);
        final Spinner spStage = dialogView.findViewById(R.id.dialog_sp_stage);
        final Spinner spWorkMode = dialogView.findViewById(R.id.dialog_sp_workmode);
        final Spinner spJobType = dialogView.findViewById(R.id.dialog_sp_jobtype);
        final EditText etLocation = dialogView.findViewById(R.id.dialog_et_location);
        final EditText etJobUrl = dialogView.findViewById(R.id.dialog_et_joburl);
        final EditText etSalaryMin = dialogView.findViewById(R.id.dialog_et_salary_min);
        final EditText etSalaryMax = dialogView.findViewById(R.id.dialog_et_salary_max);
        final EditText etDateApplied = dialogView.findViewById(R.id.dialog_et_date_applied);
        final EditText etNextInterview = dialogView.findViewById(R.id.dialog_et_next_interview);
        final EditText etNotes = dialogView.findViewById(R.id.dialog_et_notes);
        Button btnCancel = dialogView.findViewById(R.id.dialog_btn_cancel);
        Button btnSave = dialogView.findViewById(R.id.dialog_btn_save);

        // Spinners
        ArrayAdapter<String> stageAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, STAGES);
        spStage.setAdapter(stageAdapter);

        ArrayAdapter<String> modeAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, WORK_MODES);
        spWorkMode.setAdapter(modeAdapter);

        ArrayAdapter<String> typeAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, JOB_TYPES);
        spJobType.setAdapter(typeAdapter);

        // Fill existing data if edit mode
        if (existingApp != null) {
            tvTitle.setText("Edit: " + existingApp.company);
            etCompany.setText(existingApp.company);
            etRole.setText(existingApp.role);
            etLocation.setText(existingApp.location);
            etJobUrl.setText(existingApp.jobUrl);
            if (existingApp.salaryMin > 0) etSalaryMin.setText(String.valueOf((int)existingApp.salaryMin));
            if (existingApp.salaryMax > 0) etSalaryMax.setText(String.valueOf((int)existingApp.salaryMax));
            etDateApplied.setText(existingApp.dateApplied);
            etNextInterview.setText(existingApp.nextInterview);
            etNotes.setText(existingApp.notes);

            spStage.setSelection(findItemIndex(STAGES, existingApp.status));
            spWorkMode.setSelection(findItemIndex(WORK_MODES, existingApp.workMode));
            spJobType.setSelection(findItemIndex(JOB_TYPES, existingApp.jobType));
        } else {
            tvTitle.setText("Add Job Application");
            spStage.setSelection(findItemIndex(STAGES, defaultStage));
        }

        btnCancel.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { dialog.dismiss(); }
        });

        btnSave.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                String company = etCompany.getText().toString().trim();
                String role = etRole.getText().toString().trim();

                if (company.isEmpty() || role.isEmpty()) {
                    Toast.makeText(MainActivity.this, "Company and Role are required!", Toast.LENGTH_SHORT).show();
                    return;
                }

                DatabaseHelper.JobApplication app = (existingApp != null) ? existingApp : new DatabaseHelper.JobApplication();
                app.company = company;
                app.role = role;
                app.status = spStage.getSelectedItem().toString();
                app.workMode = spWorkMode.getSelectedItem().toString();
                app.jobType = spJobType.getSelectedItem().toString();
                app.location = etLocation.getText().toString().trim();
                app.jobUrl = etJobUrl.getText().toString().trim();

                try {
                    String minS = etSalaryMin.getText().toString().trim();
                    app.salaryMin = minS.isEmpty() ? 0 : Double.parseDouble(minS);
                } catch (Exception ignored) { app.salaryMin = 0; }

                try {
                    String maxS = etSalaryMax.getText().toString().trim();
                    app.salaryMax = maxS.isEmpty() ? 0 : Double.parseDouble(maxS);
                } catch (Exception ignored) { app.salaryMax = 0; }

                app.dateApplied = etDateApplied.getText().toString().trim();
                app.nextInterview = etNextInterview.getText().toString().trim();
                app.notes = etNotes.getText().toString().trim();

                boolean success = dbHelper.saveJobApplication(app);
                if (success) {
                    Toast.makeText(MainActivity.this, "Application saved successfully!", Toast.LENGTH_SHORT).show();
                    dialog.dismiss();
                    refreshAllData();
                } else {
                    Toast.makeText(MainActivity.this, "Failed to save application.", Toast.LENGTH_SHORT).show();
                }
            }
        });

        dialog.show();
    }

    private void showStageSelectorDialog(final DatabaseHelper.JobApplication app) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Move Stage: " + app.company);
        builder.setItems(STAGES, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                String selectedStage = STAGES[which];
                dbHelper.updateJobStage(app.id, selectedStage);
                Toast.makeText(MainActivity.this, "Moved to " + selectedStage, Toast.LENGTH_SHORT).show();
                refreshAllData();
            }
        });
        builder.show();
    }

    private void confirmDelete(final DatabaseHelper.JobApplication app) {
        new AlertDialog.Builder(this)
            .setTitle("Delete Application")
            .setMessage("Are you sure you want to delete application for " + app.role + " at " + app.company + "?")
            .setPositiveButton("Delete", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dbHelper.deleteJobApplication(app.id);
                    Toast.makeText(MainActivity.this, "Application deleted.", Toast.LENGTH_SHORT).show();
                    refreshAllData();
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void confirmClearDatabase() {
        new AlertDialog.Builder(this)
            .setTitle("Reset Database")
            .setMessage("Wipe all saved job applications and activity history? This cannot be undone.")
            .setPositiveButton("Reset Everything", new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dbHelper.resetDatabase();
                    Toast.makeText(MainActivity.this, "Database cleared.", Toast.LENGTH_SHORT).show();
                    refreshAllData();
                }
            })
            .setNegativeButton("Cancel", null)
            .show();
    }

    private void shareCsvExport() {
        try {
            String csv = dbHelper.exportCsv();
            Intent sendIntent = new Intent();
            sendIntent.setAction(Intent.ACTION_SEND);
            sendIntent.putExtra(Intent.EXTRA_TEXT, csv);
            sendIntent.setType("text/csv");
            startActivity(Intent.createChooser(sendIntent, "Share Job Applications CSV"));
        } catch (Exception e) {
            Toast.makeText(this, "Error sharing CSV: " + e.getMessage(), Toast.LENGTH_SHORT).show();
        }
    }

    public void addShortcutToHomeScreen(boolean force) {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        boolean alreadyPrompted = prefs.getBoolean(KEY_PINNED, false);

        if (!force && alreadyPrompted) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            ShortcutManager shortcutManager = getSystemService(ShortcutManager.class);
            if (shortcutManager != null && shortcutManager.isRequestPinShortcutSupported()) {
                Intent shortcutIntent = new Intent(this, MainActivity.class);
                shortcutIntent.setAction(Intent.ACTION_MAIN);
                shortcutIntent.addCategory(Intent.CATEGORY_LAUNCHER);

                ShortcutInfo pinShortcutInfo = new ShortcutInfo.Builder(this, "applytrack_native_shortcut")
                    .setShortLabel(getString(R.string.shortcut_short_label))
                    .setLongLabel(getString(R.string.shortcut_long_label))
                    .setIcon(Icon.createWithResource(this, R.mipmap.ic_launcher))
                    .setIntent(shortcutIntent)
                    .build();

                boolean success = shortcutManager.requestPinShortcut(pinShortcutInfo, null);
                if (success) {
                    prefs.edit().putBoolean(KEY_PINNED, true).apply();
                    if (force) {
                        Toast.makeText(this, "Home screen shortcut prompt displayed!", Toast.LENGTH_SHORT).show();
                    }
                }
            }
        }
    }

    private void toggleTheme() {
        isDarkMode = !isDarkMode;
        rootLayout.setBackgroundResource(isDarkMode ? R.drawable.bg_dark : R.drawable.bg_light);
        btnThemeToggle.setText(isDarkMode ? "☀️" : "🌙");
        tvAppTitle.setTextColor(isDarkMode ? Color.WHITE : Color.parseColor("#0f172a"));
        refreshAllData();
        Toast.makeText(this, isDarkMode ? "Switched to Dark Glass" : "Switched to Light Glass", Toast.LENGTH_SHORT).show();
    }

    private int findItemIndex(String[] array, String value) {
        if (value == null) return 0;
        for (int i = 0; i < array.length; i++) {
            if (array[i].equalsIgnoreCase(value.trim())) return i;
        }
        return 0;
    }

    private int dpToPx(int dp) {
        return (int) (dp * getResources().getDisplayMetrics().density + 0.5f);
    }

    @Override
    protected void onDestroy() {
        if (dbHelper != null) dbHelper.close();
        super.onDestroy();
    }
}
