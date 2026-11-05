package com.amara.applytrack;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class DatabaseHelper extends SQLiteOpenHelper {

    private static final String DATABASE_NAME = "jobtracker.db";
    private static final int DATABASE_VERSION = 1;
    private final Context context;

    public static class JobApplication {
        public String id = "";
        public String company = "";
        public String role = "";
        public String status = "Applied";
        public String workMode = "Remote";
        public String jobType = "Full-time";
        public String location = "";
        public String jobUrl = "";
        public double salaryMin = 0;
        public double salaryMax = 0;
        public String currency = "$";
        public String salaryPeriod = "Yearly";
        public String dateApplied = "";
        public String nextInterview = "";
        public String followUpDate = "";
        public String contactName = "";
        public String contactEmail = "";
        public String contactPhone = "";
        public String notes = "";
        public String tags = "";
        public String createdAt = "";
        public String updatedAt = "";
    }

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
        // Upgrade migrations if needed
    }

    private String getCurrentDateTime() {
        return new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault()).format(new Date());
    }

    public void logActivity(String type, String message, String appId) {
        try {
            SQLiteDatabase db = getWritableDatabase();
            ContentValues cv = new ContentValues();
            cv.put("action_type", type);
            cv.put("message", message);
            cv.put("application_id", appId != null ? appId : "");
            cv.put("created_at", getCurrentDateTime());
            db.insert("activity_log", null, cv);
        } catch (Exception ignored) {}
    }

    public List<JobApplication> queryApplications(String filterStatus, String searchKeyword) {
        List<JobApplication> list = new ArrayList<JobApplication>();
        SQLiteDatabase db = getReadableDatabase();

        StringBuilder sql = new StringBuilder("SELECT id, company, role, status, work_mode, job_type, location, job_url, salary_min, salary_max, currency, salary_period, date_applied, next_interview, follow_up_date, contact_name, contact_email, contact_phone, notes, tags, created_at, updated_at FROM applications WHERE 1=1 ");
        List<String> args = new ArrayList<String>();

        if (filterStatus != null && !filterStatus.equalsIgnoreCase("ALL") && !filterStatus.isEmpty()) {
            sql.append(" AND status = ? ");
            args.add(filterStatus);
        }

        if (searchKeyword != null && !searchKeyword.trim().isEmpty()) {
            String kw = "%" + searchKeyword.trim().toLowerCase(Locale.ROOT) + "%";
            sql.append(" AND (LOWER(company) LIKE ? OR LOWER(role) LIKE ? OR LOWER(location) LIKE ? OR LOWER(tags) LIKE ? OR LOWER(notes) LIKE ?) ");
            args.add(kw);
            args.add(kw);
            args.add(kw);
            args.add(kw);
            args.add(kw);
        }

        sql.append(" ORDER BY date_applied DESC, created_at DESC ");

        Cursor c = db.rawQuery(sql.toString(), args.toArray(new String[0]));
        try {
            while (c.moveToNext()) {
                JobApplication app = new JobApplication();
                app.id = c.getString(0);
                app.company = c.getString(1);
                app.role = c.getString(2);
                app.status = c.getString(3);
                app.workMode = c.getString(4);
                app.jobType = c.getString(5);
                app.location = c.getString(6);
                app.jobUrl = c.getString(7);
                app.salaryMin = c.getDouble(8);
                app.salaryMax = c.getDouble(9);
                app.currency = c.getString(10);
                app.salaryPeriod = c.getString(11);
                app.dateApplied = c.getString(12);
                app.nextInterview = c.getString(13);
                app.followUpDate = c.getString(14);
                app.contactName = c.getString(15);
                app.contactEmail = c.getString(16);
                app.contactPhone = c.getString(17);
                app.notes = c.getString(18);
                app.tags = c.getString(19);
                app.createdAt = c.getString(20);
                app.updatedAt = c.getString(21);
                list.add(app);
            }
        } finally {
            c.close();
        }
        return list;
    }

    public JobApplication getApplication(String id) {
        if (id == null || id.isEmpty()) return null;
        SQLiteDatabase db = getReadableDatabase();
        Cursor c = db.rawQuery("SELECT id, company, role, status, work_mode, job_type, location, job_url, salary_min, salary_max, currency, salary_period, date_applied, next_interview, follow_up_date, contact_name, contact_email, contact_phone, notes, tags, created_at, updated_at FROM applications WHERE id = ?", new String[]{ id });
        try {
            if (c.moveToFirst()) {
                JobApplication app = new JobApplication();
                app.id = c.getString(0);
                app.company = c.getString(1);
                app.role = c.getString(2);
                app.status = c.getString(3);
                app.workMode = c.getString(4);
                app.jobType = c.getString(5);
                app.location = c.getString(6);
                app.jobUrl = c.getString(7);
                app.salaryMin = c.getDouble(8);
                app.salaryMax = c.getDouble(9);
                app.currency = c.getString(10);
                app.salaryPeriod = c.getString(11);
                app.dateApplied = c.getString(12);
                app.nextInterview = c.getString(13);
                app.followUpDate = c.getString(14);
                app.contactName = c.getString(15);
                app.contactEmail = c.getString(16);
                app.contactPhone = c.getString(17);
                app.notes = c.getString(18);
                app.tags = c.getString(19);
                app.createdAt = c.getString(20);
                app.updatedAt = c.getString(21);
                return app;
            }
        } finally {
            c.close();
        }
        return null;
    }

    public boolean saveJobApplication(JobApplication app) {
        try {
            SQLiteDatabase db = getWritableDatabase();
            ContentValues cv = new ContentValues();
            cv.put("company", app.company != null ? app.company : "");
            cv.put("role", app.role != null ? app.role : "");
            cv.put("status", app.status != null ? app.status : "Applied");
            cv.put("work_mode", app.workMode != null ? app.workMode : "Remote");
            cv.put("job_type", app.jobType != null ? app.jobType : "Full-time");
            cv.put("location", app.location != null ? app.location : "");
            cv.put("job_url", app.jobUrl != null ? app.jobUrl : "");
            cv.put("salary_min", app.salaryMin);
            cv.put("salary_max", app.salaryMax);
            cv.put("currency", app.currency != null ? app.currency : "$");
            cv.put("salary_period", app.salaryPeriod != null ? app.salaryPeriod : "Yearly");
            cv.put("date_applied", app.dateApplied != null ? app.dateApplied : "");
            cv.put("next_interview", app.nextInterview != null ? app.nextInterview : "");
            cv.put("follow_up_date", app.followUpDate != null ? app.followUpDate : "");
            cv.put("contact_name", app.contactName != null ? app.contactName : "");
            cv.put("contact_email", app.contactEmail != null ? app.contactEmail : "");
            cv.put("contact_phone", app.contactPhone != null ? app.contactPhone : "");
            cv.put("notes", app.notes != null ? app.notes : "");
            cv.put("tags", app.tags != null ? app.tags : "");
            cv.put("updated_at", getCurrentDateTime());

            boolean isNew = (app.id == null || app.id.trim().isEmpty());
            if (isNew) {
                app.id = "app_" + (System.currentTimeMillis() / 1000) + "_" + (int)(Math.random() * 9000 + 1000);
                cv.put("id", app.id);
                cv.put("created_at", getCurrentDateTime());
                db.insert("applications", null, cv);
                logActivity("CREATED", "Added application for " + app.role + " at " + app.company, app.id);
            } else {
                db.update("applications", cv, "id = ?", new String[]{ app.id });
                logActivity("UPDATED", "Updated application details for " + app.company, app.id);
            }
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean updateJobStage(String id, String newStage) {
        try {
            SQLiteDatabase db = getWritableDatabase();
            ContentValues cv = new ContentValues();
            cv.put("status", newStage);
            cv.put("updated_at", getCurrentDateTime());
            db.update("applications", cv, "id = ?", new String[]{ id });
            logActivity("STAGE_CHANGE", "Moved application to " + newStage, id);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public boolean deleteJobApplication(String id) {
        try {
            SQLiteDatabase db = getWritableDatabase();
            db.delete("applications", "id = ?", new String[]{ id });
            logActivity("DELETED", "Deleted application " + id, id);
            return true;
        } catch (Exception e) {
            return false;
        }
    }

    public void resetDatabase() {
        try {
            SQLiteDatabase db = getWritableDatabase();
            db.execSQL("DELETE FROM applications");
            db.execSQL("DELETE FROM activity_log");
        } catch (Exception ignored) {}
    }

    public int[] getMetricsCounts() {
        int total = 0, applied = 0, interview = 0, offer = 0;
        SQLiteDatabase db = getReadableDatabase();
        Cursor c = db.rawQuery("SELECT status, count(*) FROM applications GROUP BY status", null);
        try {
            while (c.moveToNext()) {
                String st = c.getString(0);
                int count = c.getInt(1);
                total += count;
                if ("Applied".equalsIgnoreCase(st)) applied += count;
                else if (st != null && st.toLowerCase(Locale.ROOT).contains("interview")) interview += count;
                else if ("Offer".equalsIgnoreCase(st)) offer += count;
            }
        } finally {
            c.close();
        }
        return new int[]{ total, applied, interview, offer };
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

    public String getDbPath() {
        return context.getDatabasePath(DATABASE_NAME).getAbsolutePath();
    }

    public long getDbFileSize() {
        File file = context.getDatabasePath(DATABASE_NAME);
        return file.exists() ? file.length() : 0;
    }
}
