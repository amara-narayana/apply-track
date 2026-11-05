package com.amara.applytrack;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class DatabaseHelper extends SQLiteOpenHelper {

    private static final String DATABASE_NAME = "jobtracker.db";
    private static final int DATABASE_VERSION = 1;
    private final Context context;

    public DatabaseHelper(Context context) {
        super(context, DATABASE_NAME, null, DATABASE_VERSION);
        this.context = context;
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(
            "CREATE TABLE IF NOT EXISTS applications (" +
            "  id TEXT PRIMARY KEY," +
            "  company TEXT NOT NULL," +
            "  role TEXT NOT NULL," +
            "  status TEXT NOT NULL DEFAULT 'Applied'," +
            "  work_mode TEXT DEFAULT 'Remote'," +
            "  job_type TEXT DEFAULT 'Full-time'," +
            "  location TEXT DEFAULT ''," +
            "  job_url TEXT DEFAULT ''," +
            "  salary_min REAL DEFAULT 0," +
            "  salary_max REAL DEFAULT 0," +
            "  currency TEXT DEFAULT '$'," +
            "  salary_period TEXT DEFAULT 'Yearly'," +
            "  date_applied TEXT DEFAULT ''," +
            "  next_interview TEXT DEFAULT ''," +
            "  follow_up_date TEXT DEFAULT ''," +
            "  contact_name TEXT DEFAULT ''," +
            "  contact_email TEXT DEFAULT ''," +
            "  contact_phone TEXT DEFAULT ''," +
            "  notes TEXT DEFAULT ''," +
            "  tags TEXT DEFAULT ''," +
            "  created_at TEXT DEFAULT (datetime('now', 'localtime'))," +
            "  updated_at TEXT DEFAULT (datetime('now', 'localtime'))" +
            ");"
        );

        db.execSQL(
            "CREATE TABLE IF NOT EXISTS activity_log (" +
            "  id INTEGER PRIMARY KEY AUTOINCREMENT," +
            "  action_type TEXT NOT NULL," +
            "  message TEXT NOT NULL," +
            "  application_id TEXT," +
            "  created_at TEXT DEFAULT (datetime('now', 'localtime'))" +
            ");"
        );
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        // Migration if needed
    }

    private String getCurrentDateTime() {
        return new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(new Date());
    }

    public void logActivity(String type, String message, String appId) {
        SQLiteDatabase db = getWritableDatabase();
        ContentValues cv = new ContentValues();
        cv.put("action_type", type);
        cv.put("message", message);
        cv.put("application_id", appId != null ? appId : "");
        cv.put("created_at", getCurrentDateTime());
        db.insert("activity_log", null, cv);
    }

    public String getApplicationsJson() {
        JSONArray arr = new JSONArray();
        SQLiteDatabase db = getReadableDatabase();
        Cursor c = db.rawQuery("SELECT id, company, role, status, work_mode, job_type, location, job_url, salary_min, salary_max, currency, salary_period, date_applied, next_interview, follow_up_date, contact_name, contact_email, contact_phone, notes, tags, created_at, updated_at FROM applications ORDER BY date_applied DESC, created_at DESC", null);
        try {
            while (c.moveToNext()) {
                JSONObject obj = new JSONObject();
                obj.put("id", c.getString(0));
                obj.put("company", c.getString(1));
                obj.put("role", c.getString(2));
                obj.put("status", c.getString(3));
                obj.put("work_mode", c.getString(4));
                obj.put("job_type", c.getString(5));
                obj.put("location", c.getString(6));
                obj.put("job_url", c.getString(7));
                obj.put("salary_min", c.getDouble(8));
                obj.put("salary_max", c.getDouble(9));
                obj.put("currency", c.getString(10));
                obj.put("salary_period", c.getString(11));
                obj.put("date_applied", c.getString(12));
                obj.put("next_interview", c.getString(13));
                obj.put("follow_up_date", c.getString(14));
                obj.put("contact_name", c.getString(15));
                obj.put("contact_email", c.getString(16));
                obj.put("contact_phone", c.getString(17));
                obj.put("notes", c.getString(18));
                obj.put("tags", c.getString(19));
                obj.put("created_at", c.getString(20));
                obj.put("updated_at", c.getString(21));
                arr.put(obj);
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            c.close();
        }
        return arr.toString();
    }

    public String getActivityJson() {
        JSONArray arr = new JSONArray();
        SQLiteDatabase db = getReadableDatabase();
        Cursor c = db.rawQuery("SELECT id, action_type, message, application_id, created_at FROM activity_log ORDER BY id DESC LIMIT 25", null);
        try {
            while (c.moveToNext()) {
                JSONObject obj = new JSONObject();
                obj.put("id", c.getInt(0));
                obj.put("action_type", c.getString(1));
                obj.put("message", c.getString(2));
                obj.put("application_id", c.getString(3));
                obj.put("created_at", c.getString(4));
                arr.put(obj);
            }
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            c.close();
        }
        return arr.toString();
    }

    public String saveApplication(String payload) {
        try {
            JSONObject obj = new JSONObject(payload);
            String id = obj.optString("id", "").trim();
            boolean isNew = id.isEmpty();
            if (isNew) {
                id = "app_" + (System.currentTimeMillis() / 1000) + "_" + (int)(Math.random() * 9000 + 1000);
            }

            SQLiteDatabase db = getWritableDatabase();
            ContentValues cv = new ContentValues();
            cv.put("company", obj.optString("company", ""));
            cv.put("role", obj.optString("role", ""));
            cv.put("status", obj.optString("status", "Applied"));
            cv.put("work_mode", obj.optString("work_mode", "Remote"));
            cv.put("job_type", obj.optString("job_type", "Full-time"));
            cv.put("location", obj.optString("location", ""));
            cv.put("job_url", obj.optString("job_url", ""));
            cv.put("salary_min", obj.optDouble("salary_min", 0));
            cv.put("salary_max", obj.optDouble("salary_max", 0));
            cv.put("currency", obj.optString("currency", "$"));
            cv.put("salary_period", obj.optString("salary_period", "Yearly"));
            cv.put("date_applied", obj.optString("date_applied", ""));
            cv.put("next_interview", obj.optString("next_interview", ""));
            cv.put("follow_up_date", obj.optString("follow_up_date", ""));
            cv.put("contact_name", obj.optString("contact_name", ""));
            cv.put("contact_email", obj.optString("contact_email", ""));
            cv.put("contact_phone", obj.optString("contact_phone", ""));
            cv.put("notes", obj.optString("notes", ""));
            cv.put("tags", obj.optString("tags", ""));
            cv.put("updated_at", getCurrentDateTime());

            if (isNew) {
                cv.put("id", id);
                cv.put("created_at", getCurrentDateTime());
                db.insert("applications", null, cv);
                logActivity("CREATED", "Added application for " + obj.optString("role") + " at " + obj.optString("company"), id);
            } else {
                db.update("applications", cv, "id = ?", new String[]{ id });
                logActivity("UPDATED", "Updated application details for " + obj.optString("company"), id);
            }

            JSONObject res = new JSONObject();
            res.put("success", true);
            res.put("id", id);
            return res.toString();
        } catch (Exception e) {
            return "{\"success\":false,\"error\":\"" + e.getMessage() + "\"}";
        }
    }

    public String updateStage(String id, String newStage) {
        try {
            SQLiteDatabase db = getWritableDatabase();
            ContentValues cv = new ContentValues();
            cv.put("status", newStage);
            cv.put("updated_at", getCurrentDateTime());
            db.update("applications", cv, "id = ?", new String[]{ id });
            logActivity("STAGE_CHANGE", "Moved application to " + newStage, id);
            return "{\"success\":true}";
        } catch (Exception e) {
            return "{\"success\":false,\"error\":\"" + e.getMessage() + "\"}";
        }
    }

    public String deleteApplication(String id) {
        try {
            SQLiteDatabase db = getWritableDatabase();
            db.delete("applications", "id = ?", new String[]{ id });
            logActivity("DELETED", "Deleted application " + id, id);
            return "{\"success\":true}";
        } catch (Exception e) {
            return "{\"success\":false,\"error\":\"" + e.getMessage() + "\"}";
        }
    }

    public String clearData() {
        try {
            SQLiteDatabase db = getWritableDatabase();
            db.execSQL("DELETE FROM applications");
            db.execSQL("DELETE FROM activity_log");
            return "{\"success\":true}";
        } catch (Exception e) {
            return "{\"success\":false,\"error\":\"" + e.getMessage() + "\"}";
        }
    }

    public String exportCsv() {
        StringBuilder csv = new StringBuilder();
        csv.append("ID,Company,Role,Status,WorkMode,JobType,Location,JobURL,SalaryMin,SalaryMax,Currency,Period,DateApplied,NextInterview,FollowUpDate,ContactName,ContactEmail,ContactPhone,Notes,Tags\r\n");
        SQLiteDatabase db = getReadableDatabase();
        Cursor c = db.rawQuery("SELECT id, company, role, status, work_mode, job_type, location, job_url, salary_min, salary_max, currency, salary_period, date_applied, next_interview, follow_up_date, contact_name, contact_email, contact_phone, notes, tags FROM applications ORDER BY date_applied DESC", null);
        try {
            while (c.moveToNext()) {
                for (int i = 0; i < 20; i++) {
                    if (i > 0) csv.append(",");
                    String s = c.getString(i);
                    if (s == null) s = "";
                    s = s.replace("\"", "\"\"").replace("\r", " ").replace("\n", " ");
                    csv.append("\"").append(s).append("\"");
                }
                csv.append("\r\n");
            }
        } finally {
            c.close();
        }
        return csv.toString();
    }

    public String getStorageInfoJson() {
        try {
            File dbFile = context.getDatabasePath(DATABASE_NAME);
            long size = dbFile.exists() ? dbFile.length() : 0;
            JSONObject obj = new JSONObject();
            obj.put("storageDir", dbFile.getParent());
            obj.put("dbPath", dbFile.getAbsolutePath());
            obj.put("dbSize", size);
            obj.put("defaultDir", dbFile.getParent());
            obj.put("isCustom", false);
            return obj.toString();
        } catch (Exception e) {
            return "{\"storageDir\":\"\",\"dbPath\":\"\",\"dbSize\":0}";
        }
    }
}
