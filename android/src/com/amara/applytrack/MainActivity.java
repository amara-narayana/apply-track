package com.amara.applytrack;

import android.app.Activity;
import android.content.ComponentName;
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
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;

public class MainActivity extends Activity {

    private WebView webView;
    private DatabaseHelper dbHelper;
    private static final String PREFS_NAME = "applytrack_prefs";
    private static final String KEY_SHORTCUT_REQUESTED = "shortcut_requested";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        dbHelper = new DatabaseHelper(this);

        webView = new WebView(this);
        webView.setBackgroundColor(Color.parseColor("#0a0f1d"));
        setContentView(webView);

        WebSettings settings = webView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setDatabaseEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);
        settings.setLoadWithOverviewMode(true);
        settings.setUseWideViewPort(true);
        settings.setCacheMode(WebSettings.LOAD_DEFAULT);

        webView.setWebChromeClient(new WebChromeClient());
        webView.setWebViewClient(new WebViewClient());

        webView.addJavascriptInterface(new WebAppInterface(this, dbHelper), "AndroidBridge");

        // Load the local embedded glassmorphic web application
        webView.loadUrl("file:///android_asset/www/index.html");

        // Prompt user to add shortcut to home screen after launch
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override
            public void run() {
                addShortcutToHomeScreen(false);
            }
        }, 1200);
    }

    public void addShortcutToHomeScreen(boolean force) {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        boolean alreadyRequested = prefs.getBoolean(KEY_SHORTCUT_REQUESTED, false);

        if (!force && alreadyRequested) {
            return;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            ShortcutManager shortcutManager = getSystemService(ShortcutManager.class);
            if (shortcutManager != null && shortcutManager.isRequestPinShortcutSupported()) {
                Intent shortcutIntent = new Intent(this, MainActivity.class);
                shortcutIntent.setAction(Intent.ACTION_MAIN);
                shortcutIntent.addCategory(Intent.CATEGORY_LAUNCHER);

                ShortcutInfo pinShortcutInfo = new ShortcutInfo.Builder(this, "applytrack_homescreen_shortcut")
                    .setShortLabel(getString(R.string.shortcut_short_label))
                    .setLongLabel(getString(R.string.shortcut_long_label))
                    .setIcon(Icon.createWithResource(this, R.mipmap.ic_launcher))
                    .setIntent(shortcutIntent)
                    .build();

                boolean result = shortcutManager.requestPinShortcut(pinShortcutInfo, null);
                if (result) {
                    prefs.edit().putBoolean(KEY_SHORTCUT_REQUESTED, true).apply();
                    if (force) {
                        Toast.makeText(this, "Home screen shortcut requested!", Toast.LENGTH_SHORT).show();
                    }
                }
            }
        } else {
            // Legacy Android Shortcut Broadcast (Android 7 and below)
            Intent shortcutIntent = new Intent(getApplicationContext(), MainActivity.class);
            shortcutIntent.setAction(Intent.ACTION_MAIN);

            Intent addIntent = new Intent();
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, getString(R.string.app_name));
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE,
                Intent.ShortcutIconResource.fromContext(getApplicationContext(), R.mipmap.ic_launcher));
            addIntent.setAction("com.android.launcher.action.INSTALL_SHORTCUT");
            addIntent.putExtra("duplicate", false);
            sendBroadcast(addIntent);

            prefs.edit().putBoolean(KEY_SHORTCUT_REQUESTED, true).apply();
            if (force) {
                Toast.makeText(this, "Added to Home screen!", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Override
    public void onBackPressed() {
        if (webView != null && webView.canGoBack()) {
            webView.goBack();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    protected void onDestroy() {
        if (dbHelper != null) {
            dbHelper.close();
        }
        super.onDestroy();
    }
}
