package com.amara.applytrack;

import android.content.Intent;
import android.net.Uri;
import android.webkit.JavascriptInterface;
import android.widget.Toast;

public class WebAppInterface {

    private final MainActivity activity;
    private final DatabaseHelper dbHelper;

    public WebAppInterface(MainActivity activity, DatabaseHelper dbHelper) {
        this.activity = activity;
        this.dbHelper = dbHelper;
    }

    @JavascriptInterface
    public String getApplications() {
        return dbHelper.getApplicationsJson();
    }

    @JavascriptInterface
    public String getActivity() {
        return dbHelper.getActivityJson();
    }

    @JavascriptInterface
    public String saveApplication(String payload) {
        return dbHelper.saveApplication(payload);
    }

    @JavascriptInterface
    public String updateStage(String id, String stage) {
        return dbHelper.updateStage(id, stage);
    }

    @JavascriptInterface
    public String deleteApplication(String id) {
        return dbHelper.deleteApplication(id);
    }

    @JavascriptInterface
    public String getStorageInfo() {
        return dbHelper.getStorageInfoJson();
    }

    @JavascriptInterface
    public String exportCsv() {
        return dbHelper.exportCsv();
    }

    @JavascriptInterface
    public String clearData() {
        return dbHelper.clearData();
    }

    @JavascriptInterface
    public void pinToHomeScreen() {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                activity.addShortcutToHomeScreen(true);
            }
        });
    }

    @JavascriptInterface
    public void shareCsv(String csvData) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Intent sendIntent = new Intent();
                    sendIntent.setAction(Intent.ACTION_SEND);
                    sendIntent.putExtra(Intent.EXTRA_TEXT, csvData);
                    sendIntent.setType("text/csv");
                    activity.startActivity(Intent.createChooser(sendIntent, "Share Job Applications CSV"));
                } catch (Exception e) {
                    Toast.makeText(activity, "Error sharing CSV: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                }
            }
        });
    }

    @JavascriptInterface
    public void openUrl(String url) {
        if (url == null || url.trim().isEmpty()) return;
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    String target = url.trim();
                    if (!target.startsWith("http://") && !target.startsWith("https://")) {
                        target = "https://" + target;
                    }
                    Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(target));
                    activity.startActivity(intent);
                } catch (Exception e) {
                    Toast.makeText(activity, "Cannot open URL: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                }
            }
        });
    }

    @JavascriptInterface
    public void showToast(String message) {
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(activity, message, Toast.LENGTH_SHORT).show();
            }
        });
    }
}
